// amdhip64_7.dll shim for the MI50-on-Windows port (Strata):
// forwards to the 5.7-line amdhip64.dll (System32) which still enumerates gfx906,
// and converts hipGetDevicePropertiesR0600 (the only HIP-6+ symbol the 10.3 engine imports).
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dxgi1_4.h>
#include <cstring>
#include <cstdio>
#include <initializer_list>
#include <cstdlib>
#include <atomic>
extern "C" {
struct hipUUID { char bytes[16]; };
struct hipDeviceArch_t { unsigned bits; };   // 17 single-bit bitfields in the real header: 4 bytes
// Layout copied from the bundled HIP header; validated against it by tuning/layout.cpp.
struct Prop57 {
    char name[256];
    size_t totalGlobalMem;
    size_t sharedMemPerBlock;
    int regsPerBlock;
    int warpSize;
    int maxThreadsPerBlock;
    int maxThreadsDim[3];
    int maxGridSize[3];
    int clockRate;
    int memoryClockRate;
    int memoryBusWidth;
    size_t totalConstMem;
    int major;
    int minor;
    int multiProcessorCount;
    int l2CacheSize;
    int maxThreadsPerMultiProcessor;
    int computeMode;
    int clockInstructionRate;
    hipDeviceArch_t arch;
    int concurrentKernels;
    int pciDomainID;
    int pciBusID;
    int pciDeviceID;
    size_t maxSharedMemoryPerMultiProcessor;
    int isMultiGpuBoard;
    int canMapHostMemory;
    int gcnArch;
    char gcnArchName[256];
    int integrated;
    int cooperativeLaunch;
    int cooperativeMultiDeviceLaunch;
    int maxTexture1DLinear;
    int maxTexture1D;
    int maxTexture2D[2];
    int maxTexture3D[3];
    unsigned int* hdpMemFlushCntl;
    unsigned int* hdpRegFlushCntl;
    size_t memPitch;
    size_t textureAlignment;
    size_t texturePitchAlignment;
    int kernelExecTimeoutEnabled;
    int ECCEnabled;
    int tccDriver;
    int cooperativeMultiDeviceUnmatchedFunc;
    int cooperativeMultiDeviceUnmatchedGridDim;
    int cooperativeMultiDeviceUnmatchedBlockDim;
    int cooperativeMultiDeviceUnmatchedSharedMem;
    int isLargeBar;
    int asicRevision;
    int managedMemory;
    int directManagedMemAccessFromHost;
    int concurrentManagedAccess;
    int pageableMemoryAccess;
    int pageableMemoryAccessUsesHostPageTables;
};
// Layout copied from the bundled HIP header; validated against it by tuning/layout.cpp.
struct Prop63 {
  char name[256];
  hipUUID uuid;
  char luid[8];
  unsigned int luidDeviceNodeMask;
  size_t totalGlobalMem;
  size_t sharedMemPerBlock;
  int regsPerBlock;
  int warpSize;
  size_t memPitch;
  int maxThreadsPerBlock;
  int maxThreadsDim[3];
  int maxGridSize[3];
  int clockRate;
  size_t totalConstMem;
  int major;
  int minor;
  size_t textureAlignment;
  size_t texturePitchAlignment;
  int deviceOverlap;
  int multiProcessorCount;
  int kernelExecTimeoutEnabled;
  int integrated;
  int canMapHostMemory;
  int computeMode;
  int maxTexture1D;
  int maxTexture1DMipmap;
  int maxTexture1DLinear;
  int maxTexture2D[2];
  int maxTexture2DMipmap[2];
  int maxTexture2DLinear[3];
  int maxTexture2DGather[2];
  int maxTexture3D[3];
  int maxTexture3DAlt[3];
  int maxTextureCubemap;
  int maxTexture1DLayered[2];
  int maxTexture2DLayered[3];
  int maxTextureCubemapLayered[2];
  int maxSurface1D;
  int maxSurface2D[2];
  int maxSurface3D[3];
  int maxSurface1DLayered[2];
  int maxSurface2DLayered[3];
  int maxSurfaceCubemap;
  int maxSurfaceCubemapLayered[2];
  size_t surfaceAlignment;
  int concurrentKernels;
  int ECCEnabled;
  int pciBusID;
  int pciDeviceID;
  int pciDomainID;
  int tccDriver;
  int asyncEngineCount;
  int unifiedAddressing;
  int memoryClockRate;
  int memoryBusWidth;
  int l2CacheSize;
  int persistingL2CacheMaxSize;
  int maxThreadsPerMultiProcessor;
  int streamPrioritiesSupported;
  int globalL1CacheSupported;
  int localL1CacheSupported;
  size_t sharedMemPerMultiprocessor;
  int regsPerMultiprocessor;
  int managedMemory;
  int isMultiGpuBoard;
  int multiGpuBoardGroupID;
  int hostNativeAtomicSupported;
  int singleToDoublePrecisionPerfRatio;
  int pageableMemoryAccess;
  int concurrentManagedAccess;
  int computePreemptionSupported;
  int canUseHostPointerForRegisteredMem;
  int cooperativeLaunch;
  int cooperativeMultiDeviceLaunch;
  size_t sharedMemPerBlockOptin;
  int pageableMemoryAccessUsesHostPageTables;
  int directManagedMemAccessFromHost;
  int maxBlocksPerMultiProcessor;
  int accessPolicyMaxWindowSize;
  size_t reservedSharedMemPerBlock;
  int hostRegisterSupported;
  int sparseHipArraySupported;
  int hostRegisterReadOnlySupported;
  int timelineSemaphoreInteropSupported;
  int memoryPoolsSupported;
  int gpuDirectRDMASupported;
  unsigned int gpuDirectRDMAFlushWritesOptions;
  int gpuDirectRDMAWritesOrdering;
  unsigned int
      memoryPoolSupportedHandleTypes;
  int deferredMappingHipArraySupported;
  int ipcEventSupported;
  int clusterLaunch;
  int unifiedFunctionPointers;
  int reserved[63];
  int hipReserved[32];
  char gcnArchName[256];
  size_t maxSharedMemoryPerMultiProcessor;
  int clockInstructionRate;
  hipDeviceArch_t arch;
  unsigned int* hdpMemFlushCntl;
  unsigned int* hdpRegFlushCntl;
  int cooperativeMultiDeviceUnmatchedFunc;
  int cooperativeMultiDeviceUnmatchedGridDim;
  int cooperativeMultiDeviceUnmatchedBlockDim;
  int cooperativeMultiDeviceUnmatchedSharedMem;
  int isLargeBar;
  int asicRevision;
};
}
// Enable SeLockMemoryPrivilege (and the working-set one) at load: the 5.7-line HIP runtime
// calls cudaHostRegister/hipHostRegister WITHOUT enabling the privilege in the process token,
// so Windows refuses every large page-lock ("page-locking refused (out of memory)") and both
// pinned DMA and the MTP graphs' mapped-copy nodes degrade or fail. NVIDIA's runtime does this
// itself - the AMD one from the driver does not (MI50 Windows port).
static void enable_lock_memory_privileges() {
    HANDLE tok = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok)) return;
    // TOKEN_PRIVILEGES contains one entry: adjust each privilege separately.
    for (const char* name : {"SeLockMemoryPrivilege", "SeIncreaseWorkingSetPrivilege"}) {
        TOKEN_PRIVILEGES tp{};
        if (!LookupPrivilegeValueA(nullptr, name, &tp.Privileges[0].Luid)) continue;
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(tok, FALSE, &tp, 0, nullptr, nullptr);
    }
    CloseHandle(tok);
}
static const bool privileges_enabled = (enable_lock_memory_privileges(), true);

// The engine requires a linear GPU alias for each registered host range. Separate
// registrations have independent GPU virtual addresses; they must never be combined
// by pointer arithmetic. Verified by partial-pin-probe at every GiB boundary.
// Keep a conservative 24 GiB per-range cap. Larger arenas use the engine's partial
// pinning path: a single registered prefix plus ordinary resident RAM.
static constexpr size_t kContiguousPinCap = 24ull << 30;
static FARPROC host_api(const char* name) {
    HMODULE h = GetModuleHandleW(L"amdhip64.dll");
    if (!h) h = LoadLibraryW(L"amdhip64.dll");
    return h ? GetProcAddress(h, name) : nullptr;
}
extern "C" __declspec(dllexport) int hipHostRegister(void* p, size_t n, unsigned flags) {
    if (n > kContiguousPinCap) return 2;
    using Fn = int(*)(void*, size_t, unsigned);
    static auto f = (Fn)host_api("hipHostRegister");
    return f ? f(p, n, flags) : 6000;
}
extern "C" __declspec(dllexport) int hipHostUnregister(void* p) {
    using Fn = int(*)(void*);
    static auto f = (Fn)host_api("hipHostUnregister");
    return f ? f(p) : 6000;
}
extern "C" __declspec(dllexport) int hipHostMalloc(void** p, size_t n, unsigned flags) {
    if (!p) return 1;
    if (n > kContiguousPinCap) { *p = nullptr; return 2; }
    using Fn = int(*)(void**, size_t, unsigned);
    static auto f = (Fn)host_api("hipHostMalloc");
    return f ? f(p, n, flags) : 6000;
}
extern "C" __declspec(dllexport) int hipHostFree(void* p) {
    using Fn = int(*)(void*);
    static auto f = (Fn)host_api("hipHostFree");
    return f ? f(p) : 6000;
}

typedef int hipError_t;
extern "C" __declspec(dllexport) hipError_t hipGetDevicePropertiesR0600(Prop63* p, int dev);
extern "C" __declspec(dllexport) hipError_t hipExtGetLastError();
typedef hipError_t (*Props57Fn)(Prop57*, int);
static Props57Fn p_props57;
static void resolve57() { if (!p_props57) { HMODULE h = GetModuleHandleA("amdhip64.dll"); if (!h) h = LoadLibraryA("amdhip64.dll"); p_props57 = (Props57Fn)(h ? GetProcAddress(h, "hipGetDeviceProperties") : nullptr); } }
hipError_t hipGetDevicePropertiesR0600(Prop63* p, int dev) {
  std::memset(p, 0, sizeof *p); resolve57();
  if (!p_props57) return 6000;
  Prop57 s; std::memset(&s, 0, sizeof s);
  const hipError_t e = p_props57(&s, dev); if (e) return e;
  std::memcpy(p->name, s.name, sizeof p->name);
  p->totalGlobalMem = s.totalGlobalMem;
  p->sharedMemPerBlock = s.sharedMemPerBlock;
  p->regsPerBlock = s.regsPerBlock;
  p->warpSize = s.warpSize;
  p->memPitch = s.memPitch;
  p->maxThreadsPerBlock = s.maxThreadsPerBlock;
  std::memcpy(p->maxThreadsDim, s.maxThreadsDim, sizeof p->maxThreadsDim);
  std::memcpy(p->maxGridSize, s.maxGridSize, sizeof p->maxGridSize);
  p->clockRate = s.clockRate;
  p->totalConstMem = s.totalConstMem;
  p->major = s.major;
  p->minor = s.minor;
  p->textureAlignment = s.textureAlignment;
  p->texturePitchAlignment = s.texturePitchAlignment;
  p->multiProcessorCount = s.multiProcessorCount;
  p->kernelExecTimeoutEnabled = s.kernelExecTimeoutEnabled;
  p->integrated = s.integrated;
  p->canMapHostMemory = s.canMapHostMemory;
  p->computeMode = s.computeMode;
  p->maxTexture1D = s.maxTexture1D;
  p->maxTexture1DLinear = s.maxTexture1DLinear;
  std::memcpy(p->maxTexture2D, s.maxTexture2D, sizeof p->maxTexture2D);
  std::memcpy(p->maxTexture3D, s.maxTexture3D, sizeof p->maxTexture3D);
  p->concurrentKernels = s.concurrentKernels;
  p->ECCEnabled = s.ECCEnabled;
  p->pciBusID = s.pciBusID;
  p->pciDeviceID = s.pciDeviceID;
  p->pciDomainID = s.pciDomainID;
  p->tccDriver = s.tccDriver;
  p->memoryClockRate = s.memoryClockRate;
  p->memoryBusWidth = s.memoryBusWidth;
  p->l2CacheSize = s.l2CacheSize;
  p->maxThreadsPerMultiProcessor = s.maxThreadsPerMultiProcessor;
  p->managedMemory = s.managedMemory;
  p->isMultiGpuBoard = s.isMultiGpuBoard;
  p->pageableMemoryAccess = s.pageableMemoryAccess;
  p->concurrentManagedAccess = s.concurrentManagedAccess;
  p->cooperativeLaunch = s.cooperativeLaunch;
  p->cooperativeMultiDeviceLaunch = s.cooperativeMultiDeviceLaunch;
  p->pageableMemoryAccessUsesHostPageTables = s.pageableMemoryAccessUsesHostPageTables;
  p->directManagedMemAccessFromHost = s.directManagedMemAccessFromHost;
  std::memcpy(p->gcnArchName, s.gcnArchName, sizeof p->gcnArchName);
  p->maxSharedMemoryPerMultiProcessor = s.maxSharedMemoryPerMultiProcessor;
  p->clockInstructionRate = s.clockInstructionRate;
  p->arch = s.arch;
  p->hdpMemFlushCntl = s.hdpMemFlushCntl;
  p->hdpRegFlushCntl = s.hdpRegFlushCntl;
  p->cooperativeMultiDeviceUnmatchedFunc = s.cooperativeMultiDeviceUnmatchedFunc;
  p->cooperativeMultiDeviceUnmatchedGridDim = s.cooperativeMultiDeviceUnmatchedGridDim;
  p->cooperativeMultiDeviceUnmatchedBlockDim = s.cooperativeMultiDeviceUnmatchedBlockDim;
  p->cooperativeMultiDeviceUnmatchedSharedMem = s.cooperativeMultiDeviceUnmatchedSharedMem;
  p->isLargeBar = s.isLargeBar;
  p->asicRevision = s.asicRevision;
  p->luidDeviceNodeMask = 1;   // single-node LUID
  {
    IDXGIFactory4* f = nullptr;
    typedef HRESULT(WINAPI* CF)(const IID&, void**);
    if (HMODULE d = GetModuleHandleA("dxgi.dll")) {
      if (auto cf = (CF)GetProcAddress(d, "CreateDXGIFactory1")) {
        if (SUCCEEDED(cf(__uuidof(IDXGIFactory4), (void**)&f))) {
          IDXGIAdapter1* a = nullptr;
          for (UINT i = 0; f->EnumAdapters1(i, &a) != DXGI_ERROR_NOT_FOUND; i++, a->Release()) {
            DXGI_ADAPTER_DESC1 dd{}; a->GetDesc1(&dd);
            if (dd.VendorId == 0x1002 && dd.DedicatedVideoMemory + (256ull<<20) > s.totalGlobalMem &&
                dd.DedicatedVideoMemory < s.totalGlobalMem + (256ull<<20)) {
              std::memcpy(p->luid, &dd.AdapterLuid, sizeof p->luid); break; } }
          f->Release(); } } } }
  return 0; }
hipError_t hipExtGetLastError() { typedef hipError_t(*Fn)(); HMODULE h = GetModuleHandleA("amdhip64.dll");
  if (h) if (auto f = (Fn)GetProcAddress(h, "hipGetLastError")) return f(); return 0; }
// ---- forwarders to amdhip64.dll ----
#pragma comment(linker, "/export:__hipPopCallConfiguration=amdhip64.__hipPopCallConfiguration")
#pragma comment(linker, "/export:__hipPushCallConfiguration=amdhip64.__hipPushCallConfiguration")
#pragma comment(linker, "/export:__hipRegisterFatBinary=amdhip64.__hipRegisterFatBinary")
#pragma comment(linker, "/export:__hipRegisterFunction=amdhip64.__hipRegisterFunction")
#pragma comment(linker, "/export:__hipRegisterVar=amdhip64.__hipRegisterVar")
#pragma comment(linker, "/export:__hipUnregisterFatBinary=amdhip64.__hipUnregisterFatBinary")
#pragma comment(linker, "/export:hipDeviceCanAccessPeer=amdhip64.hipDeviceCanAccessPeer")
#pragma comment(linker, "/export:hipDeviceDisablePeerAccess=amdhip64.hipDeviceDisablePeerAccess")
#pragma comment(linker, "/export:hipDeviceEnablePeerAccess=amdhip64.hipDeviceEnablePeerAccess")
#pragma comment(linker, "/export:hipDeviceGetAttribute=amdhip64.hipDeviceGetAttribute")
#pragma comment(linker, "/export:hipDeviceGetDefaultMemPool=amdhip64.hipDeviceGetDefaultMemPool")
#pragma comment(linker, "/export:hipDeviceGetStreamPriorityRange=amdhip64.hipDeviceGetStreamPriorityRange")
#pragma comment(linker, "/export:hipDeviceSynchronize=amdhip64.hipDeviceSynchronize")
#pragma comment(linker, "/export:hipDriverGetVersion=amdhip64.hipDriverGetVersion")
#pragma comment(linker, "/export:hipEventCreate=amdhip64.hipEventCreate")
#pragma comment(linker, "/export:hipEventCreateWithFlags=amdhip64.hipEventCreateWithFlags")
#pragma comment(linker, "/export:hipEventDestroy=amdhip64.hipEventDestroy")
#pragma comment(linker, "/export:hipEventElapsedTime=amdhip64.hipEventElapsedTime")
#pragma comment(linker, "/export:hipEventQuery=amdhip64.hipEventQuery")
#pragma comment(linker, "/export:hipEventRecord=amdhip64.hipEventRecord")
#pragma comment(linker, "/export:hipEventSynchronize=amdhip64.hipEventSynchronize")
#pragma comment(linker, "/export:hipExtModuleLaunchKernel=amdhip64.hipExtModuleLaunchKernel")
#pragma comment(linker, "/export:hipFree=amdhip64.hipFree")
#pragma comment(linker, "/export:hipFreeAsync=amdhip64.hipFreeAsync")
#pragma comment(linker, "/export:hipFuncSetAttribute=amdhip64.hipFuncSetAttribute")
#pragma comment(linker, "/export:hipGetDevice=amdhip64.hipGetDevice")
#pragma comment(linker, "/export:hipGetDeviceCount=amdhip64.hipGetDeviceCount")
#pragma comment(linker, "/export:hipGetErrorName=amdhip64.hipGetErrorName")
#pragma comment(linker, "/export:hipGetErrorString=amdhip64.hipGetErrorString")
#pragma comment(linker, "/export:hipGetLastError=amdhip64.hipGetLastError")
#pragma comment(linker, "/export:hipGraphDestroy=amdhip64.hipGraphDestroy")
#pragma comment(linker, "/export:hipGraphExecDestroy=amdhip64.hipGraphExecDestroy")
#pragma comment(linker, "/export:hipGraphGetNodes=amdhip64.hipGraphGetNodes")
// hipGraphInstantiateWithFlags: the 5.7-line runtime now and then reports a spurious
// hipErrorOutOfMemory (2) for a graph that instantiates fine a moment later (MI50 Windows
// port; MTP round capture). Retry a few times before giving the error up to the engine.
typedef int (*InstFlagsFn)(void**, void*, unsigned long long);
extern "C" __declspec(dllexport) int hipGraphInstantiateWithFlags(void** exec, void* graph, unsigned long long flags) {
    HMODULE h = GetModuleHandleA("amdhip64.dll");
    if (!h) h = LoadLibraryA("amdhip64.dll");
    if (!h) return 6000;
    auto f = (InstFlagsFn)GetProcAddress(h, "hipGraphInstantiateWithFlags");
    if (!f) return 6000;
    int e = f(exec, graph, flags);
    for (int t = 0; e == 2 && t < 4; ++t) { Sleep(120); e = f(exec, graph, flags); }
    return e;
}
#pragma comment(linker, "/export:hipGraphKernelNodeGetParams=amdhip64.hipGraphKernelNodeGetParams")
#pragma comment(linker, "/export:hipGraphLaunch=amdhip64.hipGraphLaunch")
#pragma comment(linker, "/export:hipGraphNodeGetType=amdhip64.hipGraphNodeGetType")
#pragma comment(linker, "/export:hipGraphUpload=amdhip64.hipGraphUpload")
#pragma comment(linker, "/export:hipHostGetDevicePointer=amdhip64.hipHostGetDevicePointer")
#pragma comment(linker, "/export:hipLaunchHostFunc=amdhip64.hipLaunchHostFunc")
#pragma comment(linker, "/export:hipLaunchKernel=amdhip64.hipLaunchKernel")
#pragma comment(linker, "/export:hipMalloc=amdhip64.hipMalloc")
#pragma comment(linker, "/export:hipMallocAsync=amdhip64.hipMallocAsync")
// gfx906's CUDA compatibility header calls HIP directly and bypasses device.cu's WDDM guard.
// Apply the same budget subtraction here, before the engine sizes its expert cache.
extern "C" __declspec(dllexport) int hipMemGetInfo(size_t* available, size_t* total) {
    HMODULE h = GetModuleHandleA("amdhip64.dll");
    if (!h) h = LoadLibraryA("amdhip64.dll");
    auto real = h ? (int(*)(size_t*,size_t*))GetProcAddress(h,"hipMemGetInfo") : nullptr;
    if (!real) return 6000;
    const int rc = real(available,total);
    const char* opt = std::getenv("STRATA_WDDM_BUDGET");
    if (rc || !available || !total || (opt && opt[0]=='0')) return rc;
    static IDXGIAdapter3* adapter = []() -> IDXGIAdapter3* {
        IDXGIFactory4* factory = nullptr;
        if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory4),(void**)&factory))) return nullptr;
        IDXGIAdapter3* result = nullptr;
        IDXGIAdapter1* candidate = nullptr;
        for (UINT i=0;factory->EnumAdapters1(i,&candidate)!=DXGI_ERROR_NOT_FOUND;i++) {
            DXGI_ADAPTER_DESC1 desc{}; candidate->GetDesc1(&desc);
            if (desc.VendorId==0x1002 && desc.DeviceId==0x66a1)
                candidate->QueryInterface(__uuidof(IDXGIAdapter3),(void**)&result);
            candidate->Release();
            if(result)break;
        }
        factory->Release();return result;
    }();
    DXGI_QUERY_VIDEO_MEMORY_INFO info{};
    if(!adapter || FAILED(adapter->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&info)) || !info.Budget)return rc;
    const size_t withheld = *total > info.Budget ? *total-(size_t)info.Budget : 0;
    *available = *available > withheld ? *available-withheld : 0;
    static std::atomic<bool> said{false};
    if(!said.exchange(true))std::fprintf(stderr,"MI50 WDDM: budget=%llu MiB usage=%llu MiB withheld=%zu MiB adjustedFree=%zu MiB\n",(unsigned long long)(info.Budget>>20),(unsigned long long)(info.CurrentUsage>>20),withheld>>20,*available>>20);
    return rc;
}
#pragma comment(linker, "/export:hipMemPoolTrimTo=amdhip64.hipMemPoolTrimTo")
#pragma comment(linker, "/export:hipMemcpy=amdhip64.hipMemcpy")
#pragma comment(linker, "/export:hipMemcpy2D=amdhip64.hipMemcpy2D")
#pragma comment(linker, "/export:hipMemcpy2DAsync=amdhip64.hipMemcpy2DAsync")
#pragma comment(linker, "/export:hipMemcpyAsync=amdhip64.hipMemcpyAsync")
#pragma comment(linker, "/export:hipMemcpyFromSymbol=amdhip64.hipMemcpyFromSymbol")
#pragma comment(linker, "/export:hipMemcpyPeer=amdhip64.hipMemcpyPeer")
#pragma comment(linker, "/export:hipMemcpyPeerAsync=amdhip64.hipMemcpyPeerAsync")
#pragma comment(linker, "/export:hipMemcpyToSymbol=amdhip64.hipMemcpyToSymbol")
#pragma comment(linker, "/export:hipMemset=amdhip64.hipMemset")
#pragma comment(linker, "/export:hipMemsetAsync=amdhip64.hipMemsetAsync")
#pragma comment(linker, "/export:hipModuleGetFunction=amdhip64.hipModuleGetFunction")
#pragma comment(linker, "/export:hipModuleLoadData=amdhip64.hipModuleLoadData")
#pragma comment(linker, "/export:hipModuleOccupancyMaxActiveBlocksPerMultiprocessor=amdhip64.hipModuleOccupancyMaxActiveBlocksPerMultiprocessor")
#pragma comment(linker, "/export:hipModuleUnload=amdhip64.hipModuleUnload")
#pragma comment(linker, "/export:hipPeekAtLastError=amdhip64.hipPeekAtLastError")
#pragma comment(linker, "/export:hipPointerGetAttributes=amdhip64.hipPointerGetAttributes")
#pragma comment(linker, "/export:hipRuntimeGetVersion=amdhip64.hipRuntimeGetVersion")
#pragma comment(linker, "/export:hipSetDevice=amdhip64.hipSetDevice")
#pragma comment(linker, "/export:hipSetDeviceFlags=amdhip64.hipSetDeviceFlags")
#pragma comment(linker, "/export:hipStreamBeginCapture=amdhip64.hipStreamBeginCapture")
#pragma comment(linker, "/export:hipStreamCreate=amdhip64.hipStreamCreate")
#pragma comment(linker, "/export:hipStreamCreateWithFlags=amdhip64.hipStreamCreateWithFlags")
#pragma comment(linker, "/export:hipStreamCreateWithPriority=amdhip64.hipStreamCreateWithPriority")
#pragma comment(linker, "/export:hipStreamDestroy=amdhip64.hipStreamDestroy")
#pragma comment(linker, "/export:hipStreamEndCapture=amdhip64.hipStreamEndCapture")
#pragma comment(linker, "/export:hipStreamIsCapturing=amdhip64.hipStreamIsCapturing")
#pragma comment(linker, "/export:hipStreamQuery=amdhip64.hipStreamQuery")
#pragma comment(linker, "/export:hipStreamSynchronize=amdhip64.hipStreamSynchronize")
#pragma comment(linker, "/export:hipStreamWaitEvent=amdhip64.hipStreamWaitEvent")
