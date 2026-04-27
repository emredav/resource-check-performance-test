#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

namespace linux_resource_check {

struct ProcessMemoryCountersLinux {
    unsigned long long WorkingSetSize;     // Anlık RSS (KB)
};

// Caching file descriptor for high performance
inline bool GetProcessMemoryInfoFast(ProcessMemoryCountersLinux* pmc) {
    // Bu yuzden anlik RAM'i alabilmek icin Linux'taki en hizli yontem olan /proc/self/statm kullandik.
    static int statm_fd = -1;
    static long page_size_kb = -1;

    if (statm_fd == -1) {
        statm_fd = open("/proc/self/statm", O_RDONLY);
        page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
        if (statm_fd == -1) return false;
    }

    char buffer[64];
    ssize_t bytesRead = pread(statm_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) return false;

    unsigned long long residentPages = 0;
    char* p = buffer;

    // 1. Degeri (Sanal bellek) tamamen es gec (bosluga kadar ilerle)
    while (*p != ' ') {
        ++p;
    }
    ++p; // Boslugu atla

    // 2. Deger (Anlik fiziksel bellek / RSS)
    while (*p >= '0' && *p <= '9') {
        residentPages = residentPages * 10 + (*p - '0');
        ++p;
    }

    pmc->WorkingSetSize = residentPages * page_size_kb;

    return true;
}

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

    ProcessMemoryCountersLinux pmc;
    if (GetProcessMemoryInfoFast(&pmc)) {
        out << "Fiziksel RAM Kullanimi (Working Set) : " << pmc.WorkingSetSize / 1024ULL << " MB\n";
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

    out << "Kullanilan Yontem              : clock_gettime(CLOCK_PROCESS_CPUTIME_ID)\n";
    struct timespec cpu_time;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu_time) == 0) {
        const double cpu_time_sec = static_cast<double>(cpu_time.tv_sec) +
                                    static_cast<double>(cpu_time.tv_nsec) / 1000000000.0;
        out << "Mevcut Process CPU Zamani      : " << cpu_time_sec << " saniye\n";
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
    out << "--- Sistem Islemci (CPU) API Cagri Maliyeti (Linux: clock_gettime()) ---\n";
    out << "Kullanilan Yontem              : clock_gettime(CLOCK_PROCESS_CPUTIME_ID)\n";

    struct timespec cpu_time;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu_time) != 0) {
            out << "CPU API cagri maliyeti olculemedi!\n";
            if (waitForEnter) {
                out << "\nCikmak icin bir tusa basin...";
                std::cin.get();
            }
            return;
        }
    }
    const auto end = std::chrono::steady_clock::now();
    
    long long totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double averageNs = static_cast<double>(totalNs) / iterations;

    out << "Toplam Cagirilan API Sayisi: " << iterations << "\n";
    out << "Toplam Gecen Zaman         : " << (totalNs / 1000.0) << " us\n";
    out << std::fixed << std::setprecision(3);
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns (" << (averageNs / 1000.0) << " us)\n";

    if (waitForEnter) {
        out << "\nCikmak icin bir tusa basin...";
        std::cin.get();
    }
}

inline void benchmarkSystemRamApiCost(std::ostream &out, int iterations) {
    out << "--- Sistem RAM API Cagri Maliyeti (Linux: sysinfo()) ---\n";
    
    struct sysinfo info;
    
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        sysinfo(&info);
    }
    const auto end = std::chrono::steady_clock::now();
    
    long long totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double averageNs = static_cast<double>(totalNs) / iterations;

    out << "Toplam Cagirilan API Sayisi: " << iterations << "\n";
    out << "Toplam Gecen Zaman         : " << (totalNs / 1000.0) << " us\n";
    out << std::fixed << std::setprecision(3);
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns (" << (averageNs / 1000.0) << " us)\n";
}

inline void benchmarkProcessRamApiCost(std::ostream &out, int iterations) {
    out << "--- Process RAM API Cagri Maliyeti (Linux: statm pread()) ---\n";
    
    ProcessMemoryCountersLinux pmc;
    
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        GetProcessMemoryInfoFast(&pmc);
    }
    const auto end = std::chrono::steady_clock::now();
    
    long long totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double averageNs = static_cast<double>(totalNs) / iterations;

    out << "Toplam Cagirilan API Sayisi: " << iterations << "\n";
    out << "Toplam Gecen Zaman         : " << (totalNs / 1000.0) << " us\n";
    out << std::fixed << std::setprecision(3);
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns (" << (averageNs / 1000.0) << " us)\n";
}

inline void runNetworkCheck(std::ostream &out, bool waitForEnter) {
    const auto startTime = std::chrono::steady_clock::now();

    out << "--- Ag Arayuzu Bilgileri ---\n";

    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        out << "Ag arayuzu bilgisi alinamadi!\n";
    } else {
        bool found = false;
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL) continue;

            // Sadece IPv4 adreslerini listele
            if (ifa->ifa_addr->sa_family == AF_INET) {
                out << "Arayuz: " << ifa->ifa_name << "\n";
                if (ifa->ifa_flags & IFF_UP) {
                    out << "  Durum: AKTIF\n";
                    found = true;
                } else {
                    out << "  Durum: PASIF\n";
                }
            }
        }

        freeifaddrs(ifaddr);
        
        if (!found) {
            out << "Aktif ag arayuzu bulunamadi.\n";
        }
    }

    // /proc/net/dev'den trafik bilgilerini oku
    out << "\n--- Ag Istatistikleri ---\n";
    std::ifstream netDevFile("/proc/net/dev");
    if (!netDevFile.is_open()) {
        out << "Ag trafik bilgisi alinamadi!\n";
    } else {
        std::string line;
        // Skip first two lines
        std::getline(netDevFile, line);
        std::getline(netDevFile, line);

        while (std::getline(netDevFile, line)) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string ifaceName = line.substr(0, colonPos);
                // trim spaces from ifaceName
                size_t first = ifaceName.find_first_not_of(" \t");
                if (first != std::string::npos) ifaceName = ifaceName.substr(first);

                std::istringstream iss(line.substr(colonPos + 1));
                unsigned long long rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame, rx_compressed, rx_multicast;
                unsigned long long tx_bytes, tx_packets, tx_errs, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;

                if (iss >> rx_bytes >> rx_packets >> rx_errs >> rx_drop >> rx_fifo >> rx_frame >> rx_compressed >> rx_multicast
                        >> tx_bytes >> tx_packets >> tx_errs >> tx_drop >> tx_fifo >> tx_colls >> tx_carrier >> tx_compressed) {
                    
                    out << "Arayuz: " << ifaceName << "\n";
                    out << "  Gelen Bayt: " << rx_bytes << "\n";
                    out << "  Cikan Bayt: " << tx_bytes << "\n";
                    out << "  Gelen Paket: " << rx_packets << "\n";
                    out << "  Cikan Paket: " << tx_packets << "\n";
                }
            }
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

inline void benchmarkNetworkApiCost(std::ostream &out, int iterations) {
    out << "--- Ag Bilgisi API Cagri Maliyeti (Linux: getifaddrs()) ---\n";
    
    struct ifaddrs *ifaddr;
    
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        if (getifaddrs(&ifaddr) == 0) {
            freeifaddrs(ifaddr);
        }
    }
    const auto end = std::chrono::steady_clock::now();
    
    long long totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double averageNs = static_cast<double>(totalNs) / iterations;

    out << "Toplam Cagirilan API Sayisi: " << iterations << "\n";
    out << "Toplam Gecen Zaman         : " << (totalNs / 1000.0) << " us\n";
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns\n";
}

inline void benchmarkNetworkTableCost(std::ostream &out, int iterations) {
    out << "--- Ag Tablo API Cagri Maliyeti (Linux: /proc/net/dev Dosya Okuma) ---\n";
    
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        std::ifstream netDevFile("/proc/net/dev");
        if (netDevFile.is_open()) {
            std::string line;
            std::getline(netDevFile, line);
            std::getline(netDevFile, line);

            while (std::getline(netDevFile, line)) {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::istringstream iss(line.substr(colonPos + 1));
                    unsigned long long rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame, rx_compressed, rx_multicast;
                    unsigned long long tx_bytes, tx_packets, tx_errs, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;

                    iss >> rx_bytes >> rx_packets >> rx_errs >> rx_drop >> rx_fifo >> rx_frame >> rx_compressed >> rx_multicast
                        >> tx_bytes >> tx_packets >> tx_errs >> tx_drop >> tx_fifo >> tx_colls >> tx_carrier >> tx_compressed;
                }
            }
        }
    }
    const auto end = std::chrono::steady_clock::now();
    
    long long totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double averageNs = static_cast<double>(totalNs) / iterations;

    out << "Toplam Cagirilan API Sayisi: " << iterations << "\n";
    out << "Toplam Gecen Zaman         : " << (totalNs / 1000.0) << " us\n";
    out << std::fixed << std::setprecision(3);
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns (" << (averageNs / 1000.0) << " us)\n";
}

} // namespace linux_resource_check