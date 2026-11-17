"""Windows AMD telemetry contract and resource lifetime, without a GPU or ADL install."""
import ctypes
import unittest
from unittest import mock

from serve import telemetry


class WindowsAmdTelemetry(unittest.TestCase):
    def library(self):
        cls = telemetry._WindowsAmdAdl
        lib = mock.Mock()

        def create(allocator, flags, context):
            allocator(128)
            context._obj.value = 123
            return 0

        def count(context, value):
            value._obj.value = 3
            return 0

        def adapters(context, rows, size):
            for i, row in enumerate(rows):
                row.adapter_index = i
                row.vendor_id = 1002  # ADL's decimal representation, not the PCI integer 0x1002
                row.bus = 3 if i < 2 else 5
                row.name = b"AMD Radeon Pro VII"
                row.pnp_string = b"PCI\\test"
            return 0

        def pmlog(context, adapter, value):
            pm = value._obj
            for key, n in [(cls.ACTIVITY, 42), (cls.EDGE_TEMP, 60), (cls.ASIC_POWER, 125)]:
                pm.sensors[key].supported = 1
                pm.sensors[key].value = n
            return 0

        def usage(context, adapter, value):
            value._obj.value = 8192 + adapter
            return 0

        def memory(context, adapter, value):
            value._obj.size_bytes = 16368 << 20
            return 0

        for name, fn in [("Main_Control_Create", create), ("Adapter_NumberOfAdapters_Get", count),
                         ("Adapter_AdapterInfo_Get", adapters), ("New_QueryPMLogData_Get", pmlog),
                         ("Adapter_DedicatedVRAMUsage_Get", usage), ("Adapter_MemoryInfo_Get", memory)]:
            getattr(lib, "ADL2_" + name).side_effect = fn
        lib.ADL2_Main_Control_Destroy.return_value = 0
        return lib

    def reader(self, lib=None, index=0):
        lib = lib or self.library()
        with mock.patch.object(telemetry.ctypes, "CDLL", return_value=lib), \
             mock.patch.object(telemetry._WindowsAmdAdl, "_pcie_link", return_value={}):
            g = telemetry._WindowsAmdAdl(index)
        self.addCleanup(g.close)
        return g, lib

    def test_physical_card_mapping_units_and_unsupported_sensors(self):
        g, lib = self.reader(index=1)
        self.assertTrue(g.ok())
        self.assertEqual(g.adapter, 2)  # two logical handles for bus 3 count as one card
        with mock.patch.object(g, "_pcie_link", return_value={}):
            r = g.read()
        self.assertEqual((r["util"], r["temp"], r["power"]), (42, 60, 125))
        self.assertEqual((r["mem_used"], r["mem_total"]), ((8192 + 2) << 20, 16368 << 20))
        self.assertEqual(r["power_kind"], "asic")
        self.assertIsNone(r["power_limit"])
        self.assertIsNone(r["pcie_rx_mb"])
        self.assertIsNone(r["fan_rpm"])
        g.close()
        g.close()
        lib.ADL2_Main_Control_Destroy.assert_called_once()
        self.assertEqual(g.keep_alive, [])
        self.assertEqual(g.read(), {})

    def test_invalid_sensor_values_and_failed_sample_are_missing(self):
        g, lib = self.reader()
        cls = telemetry._WindowsAmdAdl

        def bad_sample(context, adapter, value):
            for key, n in [(cls.ACTIVITY, 101), (cls.EDGE_TEMP, -1), (cls.HOTSPOT_TEMP, 70)]:
                value._obj.sensors[key].supported = 1
                value._obj.sensors[key].value = n
            return 0

        lib.ADL2_New_QueryPMLogData_Get.side_effect = bad_sample
        lib.ADL2_Adapter_DedicatedVRAMUsage_Get.side_effect = lambda c, a, p: setattr(p._obj, "value", 999999) or 0
        with mock.patch.object(g, "_pcie_link", return_value={}):
            r = g.read()
            self.assertIsNone(r["util"])
            self.assertEqual(r["temp"], 70)
            self.assertIsNone(r["mem_used"])
            lib.ADL2_New_QueryPMLogData_Get.side_effect = None
            lib.ADL2_New_QueryPMLogData_Get.return_value = -8
            self.assertIsNone(g.read()["power"])

    def test_failed_enumeration_destroys_context(self):
        lib = self.library()
        lib.ADL2_Adapter_NumberOfAdapters_Get.side_effect = lambda c, p: setattr(p._obj, "value", 1000000) or 0
        g, _ = self.reader(lib)
        self.assertFalse(g.ok())
        lib.ADL2_Main_Control_Destroy.assert_called_once()
        self.assertEqual(g.read(), {})

    def test_sampler_shutdown_releases_adl(self):
        g, lib = self.reader()
        with mock.patch.object(telemetry, "gpu_reader", return_value=g), \
             mock.patch.object(g, "_pcie_link", return_value={}):
            sampler = telemetry.Telemetry(amd=True)
            sampler.close()
        self.assertFalse(sampler._thread.is_alive())
        lib.ADL2_Main_Control_Destroy.assert_called_once()

    def test_free_vram_always_closes_even_when_sample_fails(self):
        g = mock.Mock()
        g.read.side_effect = OSError("driver unavailable")
        with mock.patch.object(telemetry, "gpu_reader", return_value=g):
            with self.assertRaises(OSError):
                telemetry.free_vram_mib(amd=True)
        g.close.assert_called_once()

    def test_pcie_properties_are_typed_and_endpoint_only(self):
        cm = mock.Mock()
        cm.CM_Locate_DevNodeW.side_effect = lambda p, name, flags: setattr(p._obj, "value", 17) or 0

        def prop(dev, key, kind, value, size, flags):
            kind._obj.value = 7  # DEVPROP_TYPE_UINT32
            ctypes.cast(value, ctypes.POINTER(ctypes.c_uint32))[0] = {9: 4, 10: 16}[key._obj.pid]
            return 0

        cm.CM_Get_DevNode_PropertyW.side_effect = prop
        g = telemetry._WindowsAmdAdl.__new__(telemetry._WindowsAmdAdl)
        with mock.patch.object(telemetry.os, "name", "nt"), \
             mock.patch.object(telemetry.ctypes, "windll", mock.Mock(cfgmgr32=cm), create=True):
            r = g._pcie_link("PCI\\test")
            self.assertEqual((r["pcie_gen"], r["pcie_width"], r["pcie_scope"]), (4, 16, "endpoint"))
            self.assertIsNone(r["pcie_gen_max"])
            cm.CM_Get_DevNode_PropertyW.side_effect = None
            cm.CM_Get_DevNode_PropertyW.return_value = 0  # no type/size set: invalid despite success
            self.assertIsNone(g._pcie_link("PCI\\test")["pcie_gen"])


if __name__ == "__main__":
    unittest.main()
