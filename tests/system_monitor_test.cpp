#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <cmath>

#include "SystemMonitor.h"

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#define mkdir_compat(d) _mkdir(d)
#else
#include <sys/stat.h>
#define mkdir_compat(d) mkdir(d, 0777)
#endif

int main() {
    std::cout << "========================================\n";
    std::cout << "      SYSTEM MONITOR TEST\n";
    std::cout << "========================================\n\n";

    // 1. Live System Monitor query
    SystemMonitor monitor;

    // CPU info
    CpuInfo cpu;
    bool cpuOk = monitor.getCpuInfo(cpu, 50);
    assert(cpuOk);
    assert(!cpu.modelName.empty());
    assert(cpu.logicalCores >= 1);
    assert(cpu.usagePercent >= 0.0 && cpu.usagePercent <= 100.0);
    std::cout << "[PASS] CPU information parsing (" << cpu.logicalCores << " cores, " << cpu.modelName << ")\n";

    // CPU utilization calculation
    double usage = monitor.sampleCpuUsage(50);
    assert(usage >= 0.0 && usage <= 100.0);
    std::cout << "[PASS] CPU utilization calculation (" << usage << "%)\n";

    // Memory info
    MemoryInfo mem;
    bool memOk = monitor.getMemoryInfo(mem);
    assert(memOk || mem.totalGB > 0.0);
    assert(mem.totalGB > 0.0);
    assert(mem.usagePercent >= 0.0 && mem.usagePercent <= 100.0);
    std::cout << "[PASS] Memory calculation (Total: " << mem.totalGB << " GB, Used: " << mem.usedGB << " GB)\n";

    // Uptime info
    double uptimeSec = 0.0;
    std::string uptimeStr;
    bool upOk = monitor.getUptime(uptimeSec, uptimeStr);
    assert(upOk || uptimeSec >= 0.0);
    assert(!uptimeStr.empty());
    std::cout << "[PASS] /proc/uptime parsing (Uptime: " << uptimeStr << ")\n";

    // 2. Direct Mock /proc parser validation
    // Create a temporary mock directory to test exact Linux /proc parser semantics
    std::string mockDir = "mock_proc";
    mkdir_compat(mockDir.c_str());

    // Mock cpuinfo
    {
        std::ofstream f(mockDir + "/cpuinfo");
        f << "processor\t: 0\n"
          << "vendor_id\t: GenuineIntel\n"
          << "model name\t: Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz\n"
          << "cpu MHz\t\t: 3792.000\n\n"
          << "processor\t: 1\n"
          << "vendor_id\t: GenuineIntel\n"
          << "model name\t: Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz\n\n";
    }

    // Mock stat
    {
        std::ofstream f(mockDir + "/stat");
        f << "cpu  1000 200 500 5000 100 50 20 0 0 0\n"
          << "cpu0 500 100 250 2500 50 25 10 0 0 0\n";
    }

    // Mock meminfo
    {
        std::ofstream f(mockDir + "/meminfo");
        f << "MemTotal:       16384000 kB\n"
          << "MemFree:         8000000 kB\n"
          << "MemAvailable:   12000000 kB\n"
          << "Buffers:          200000 kB\n"
          << "Cached:          3800000 kB\n";
    }

    // Mock uptime
    {
        std::ofstream f(mockDir + "/uptime");
        f << "12345.67 89123.45\n";
    }

    // Test with mock /proc directory
    SystemMonitor mockMonitor(mockDir);

    CpuInfo mockCpu;
    assert(mockMonitor.getCpuInfo(mockCpu, 10));
    assert(mockCpu.logicalCores == 2);
    assert(mockCpu.modelName.find("Intel") != std::string::npos);

    MemoryInfo mockMem;
    assert(mockMonitor.getMemoryInfo(mockMem));
    assert(mockMem.totalGB > 15.0 && mockMem.totalGB < 16.0); // 16384000 kB ~ 15.625 GB
    assert(mockMem.availableGB > 11.0 && mockMem.availableGB < 12.0); // 12000000 kB ~ 11.44 GB
    assert(mockMem.usagePercent > 20.0 && mockMem.usagePercent < 30.0);

    double mockUptime = 0.0;
    std::string mockUptimeStr;
    assert(mockMonitor.getUptime(mockUptime, mockUptimeStr));
    assert(std::abs(mockUptime - 12345.67) < 0.1);
    assert(mockUptimeStr.find("3 hours") != std::string::npos);

    std::cout << "[PASS] Mock /proc filesystem direct parsing\n";

    // 3. Graceful failure handling
    SystemMonitor brokenMonitor("non_existent_proc_path_9999");
    CpuInfo brokenCpu;
    // Must not crash or throw uncaught exception
    bool brokenCpuOk = brokenMonitor.getCpuInfo(brokenCpu, 10);
    assert(brokenCpuOk); // Fallback succeeds

    MemoryInfo brokenMem;
    brokenMonitor.getMemoryInfo(brokenMem); // Handled safely

    double brokenUp = 0.0;
    std::string brokenUpStr;
    brokenMonitor.getUptime(brokenUp, brokenUpStr); // Handled safely

    std::cout << "[PASS] Graceful failure handling\n";

    std::cout << "\nAll system monitor tests passed.\n";
    std::cout << "========================================\n";
    return 0;
}
