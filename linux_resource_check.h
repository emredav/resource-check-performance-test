#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/sysinfo.h>
#include <sys/times.h>
#include <unistd.h>

namespace linux_resource_check {

inline void runResourceCheck(std::ostream &out, bool waitForEnter) {
    const auto startTime = std::chrono::steady_clock::now();

    out << "--- Sistem Bellek (RAM) Bilgileri ---\n";

    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        const unsigned long long totalPhysMem =
            (static_cast<unsigned long long>(info.totalram) * info.mem_unit) / (1024ULL * 1024ULL);
        const unsigned long long freePhysMem =
            (static_cast<unsigned long long>(info.freeram) * info.mem_unit) / (1024ULL * 1024ULL);
        const unsigned long long usedPhysMem = totalPhysMem - freePhysMem;
        const unsigned long long memoryLoad = totalPhysMem == 0 ? 0 : (usedPhysMem * 100ULL) / totalPhysMem;

        out << "Toplam Fiziksel RAM : " << totalPhysMem << " MB\n";
        out << "Kullanilan RAM      : " << usedPhysMem << " MB\n";
        out << "Bos (Kullanilabilir): " << freePhysMem << " MB\n";
        out << "Sistem RAM Yuku     : %" << memoryLoad << '\n';
    } else {
        out << "Sistem RAM bilgisi alinamadi!\n";
    }

    out << "\n--- Mevcut Process (Uygulama) Bellek Bilgisi ---\n";

    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) {
        out << "Process RAM bilgisi alinamadi!\n";
    } else {
        unsigned long long residentKb = 0;
        unsigned long long virtualKb = 0;
        std::string line;
        while (std::getline(statusFile, line)) {
            if (line.rfind("VmRSS:", 0) == 0) {
                std::istringstream stream(line.substr(6));
                stream >> residentKb;
            } else if (line.rfind("VmSize:", 0) == 0) {
                std::istringstream stream(line.substr(7));
                stream >> virtualKb;
            }
        }

        if (residentKb == 0 && virtualKb == 0) {
            out << "Process RAM bilgisi alinamadi!\n";
        } else {
            out << "Fiziksel RAM Kullanimi (Working Set) : " << residentKb / 1024ULL << " MB\n";
            out << "Sanal Bellek Kullanimi (Pagefile)    : " << virtualKb / 1024ULL << " MB\n";
        }
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
    
    // Uygulamanin o ana kadar harcadigi CPU zamani
    clock_t tick_hz = sysconf(_SC_CLK_TCK);
    struct tms time_buf;
    if (times(&time_buf) != (clock_t)-1) {
        double user_time_sec = static_cast<double>(time_buf.tms_utime) / tick_hz;
        double sys_time_sec  = static_cast<double>(time_buf.tms_stime) / tick_hz;
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
    out << "--- Sistem Islemci (CPU) API Cagri Maliyeti (Linux: times()) ---\n";
    
    struct tms time_buf;
    
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        times(&time_buf);
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

} // namespace linux_resource_check