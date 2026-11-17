// Native CPU pool protocol stress, including sleeping workers and empty batches.
// No GPU or model needed. Zero-scale valid IQ4_XS/IQ4_NL experts produce zero;
// sentinels detect missing writes and writes to inactive jobs/token rows.
#include "strata/kernels/cpu/pool.hpp"
#include "strata/kernels/cpu/native_expert.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace cpu = strata::kernels::cpu;
int main(int argc, char** argv) {
    const double seconds = argc > 1 ? std::atof(argv[1]) : 20.0;
    cpu::NativeFmt format;
    std::string error;
    if (!cpu::native_fmt(23, 20, cpu::H, cpu::FF, format, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    constexpr int E = 4, T = 4;
    std::vector<uint8_t> blob(format.bytes, 0);
    std::vector<std::vector<uint8_t>> act(T, std::vector<uint8_t>(cpu::kNativeActBytes, 0));
    std::vector<float> input(cpu::H, 0.f), output((size_t) E * T * cpu::H);
    for (int t = 0; t < T; ++t) cpu::native_quant_act(format, input.data(), act[t].data());
    cpu::ExpertJobMulti jobs[E];
    for (int e = 0; e < E; ++e) {
        jobs[e].blob = blob.data();
        for (int t = 0; t < T; ++t) {
            jobs[e].nact[t] = act[t].data();
            jobs[e].out[t] = output.data() + ((size_t) e * T + t) * cpu::H;
        }
    }
    std::mt19937 random(0x906);
    size_t batches = 0, sleeps = 0;
    {
        cpu::ExpertPool pool(3, false, true, cpu::PoolAffinity::All, 36);
        const auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() < seconds) {
            if ((batches % 8) == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                ++sleeps;
            }
            const int n = random() % (E + 1);
            for (int e = 0; e < E; ++e) jobs[e].nt = 1 + random() % T;
            std::fill(output.begin(), output.end(), std::numeric_limits<float>::quiet_NaN());
            pool.run_split_multi_native(format, jobs, n);
            for (int e = 0; e < E; ++e)
                for (int t = 0; t < T; ++t)
                    for (int r = 0; r < cpu::H; ++r) {
                        const float value = output[((size_t) e * T + t) * cpu::H + r];
                        const bool active = e < n && t < jobs[e].nt;
                        if ((active && value != 0.f) || (!active && !std::isnan(value))) {
                            std::fprintf(stderr, "bad output: batch %zu e=%d t=%d r=%d active=%d value=%g\n",
                                         batches, e, t, r, active, value);
                            return 2;
                        }
                    }
            ++batches;
        }
        // Stop and join after workers have genuinely entered their sleep path.
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    std::printf("native_pool_wait_stress: %zu batches, %zu sleeping gaps; active/inactive rows and shutdown passed\n", batches, sleeps);
    return batches > 0 ? 0 : 3;
}
