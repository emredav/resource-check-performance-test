#pragma once

#include <chrono>
#include <iostream>
#include <psapi.h>
#include <windows.h>

namespace windows_resource_check {

inline void runResourceCheck(std::ostream &out, bool waitForEnter) {
    const auto startTime = std::chrono::steady_clock::now();

    out << "--- Sistem Bellek (RAM) Bilgileri ---\n";

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memInfo)) {
        const DWORDLONG totalPhysMem = memInfo.ullTotalPhys / (1024 * 1024);
        const DWORDLONG availPhysMem = memInfo.ullAvailPhys / (1024 * 1024);
        const DWORDLONG usedPhysMem = totalPhysMem - availPhysMem;

        out << "Toplam Fiziksel RAM : " << totalPhysMem << " MB\n";
        out << "Kullanilan RAM      : " << usedPhysMem << " MB\n";
        out << "Bos (Kullanilabilir): " << availPhysMem << " MB\n";
        out << "Sistem RAM Yuku     : %" << memInfo.dwMemoryLoad << '\n';
    } else {
        out << "Sistem RAM bilgisi alinamadi!\n";
    }

    out << "\n--- Mevcut Process (Uygulama) Bellek Bilgisi ---\n";

    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        const SIZE_T virtualMemUsedByMe = pmc.PagefileUsage / (1024 * 1024);
        const SIZE_T physMemUsedByMe = pmc.WorkingSetSize / (1024 * 1024);

        out << "Fiziksel RAM Kullanimi (Working Set) : " << physMemUsedByMe << " MB\n";
        out << "Sanal Bellek Kullanimi (Pagefile)    : " << virtualMemUsedByMe << " MB\n";
    } else {
        out << "Process RAM bilgisi alinamadi!\n";
    }

    const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - startTime);
    out << "\nSon Dongu Calisma Suresi: " << elapsedUs.count() << " us\n";

    if (waitForEnter) {
        out << "\nCikmak icin bir tusa basin...";
        std::cin.get();
    }
}

inline void runCpuCheck(std::ostream &out, bool waitForEnter) {
    const auto startTime = std::chrono::steady_clock::now();

    out << "--- Sistem Islemci (CPU) Bilgileri ---\n";

    FILETIME creationTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(GetCurrentProcess(), &creationTime, &exitTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER kReal, uReal;
        kReal.LowPart = kernelTime.dwLowDateTime;
        kReal.HighPart = kernelTime.dwHighDateTime;
        uReal.LowPart = userTime.dwLowDateTime;
        uReal.HighPart = userTime.dwHighDateTime;

        double sys_time_sec = static_cast<double>(kReal.QuadPart) * 1e-7;
        double user_time_sec = static_cast<double>(uReal.QuadPart) * 1e-7;

        out << "Mevcut Process User CPU Zamani  : " << user_time_sec << " saniye\n";
        out << "Mevcut Process Kernel CPU Zamani: " << sys_time_sec << " saniye\n";
        out << "Mevcut Process Toplam CPU Zamani: " << (user_time_sec + sys_time_sec) << " saniye\n";
    } else {
        out << "CPU zamani alinamadi!\n";
    }

    const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - startTime);
    out << "\nSon Dongu Calisma Suresi: " << elapsedUs.count() << " us\n";

    if (waitForEnter) {
        out << "\nCikmak icin bir tusa basin...";
        std::cin.get();
    }
}

inline void benchmarkCpuApiCost(std::ostream &out, int iterations, bool waitForEnter) {
    out << "--- Sistem Islemci (CPU) API Cagri Maliyeti (Windows: GetProcessTimes()) ---\n";
    
    FILETIME creationTime, exitTime, kernelTime, userTime;
    HANDLE currentProcess = GetCurrentProcess();

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        GetProcessTimes(currentProcess, &creationTime, &exitTime, &kernelTime, &userTime);
    }
    const auto end = std::chrono::steady_clock::now();
    
    long long totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double averageNs = static_cast<double>(totalNs) / iterations;

    out << "Toplam Cagirilan API Sayisi: " << iterations << "\n";
    out << "Toplam Gecen Zaman         : " << (totalNs / 1000.0) << " us\n";
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns\n";

    if (waitForEnter) {
        out << "\nCikmak icin bir tusa basin...";
        std::cin.get();
    }
}

} // namespace windows_resource_check