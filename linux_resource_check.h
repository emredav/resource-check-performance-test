#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/sysinfo.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>

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

    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        // getrusage() RSS'ini long tipinde verir (ki sayfalarla hesaplanır)
        unsigned long long rss_pages = usage.ru_maxrss;
        
        // Linux'ta getrusage() RSS'i kilobayt cinsinden verir
        unsigned long long rss_kb = rss_pages;
        unsigned long long rss_mb = rss_kb / 1024ULL;

        out << "Fiziksel RAM Kullanimi (Working Set) : " << rss_mb << " MB\n";
    } else {
        // Fallback: /proc/self/statm
        std::ifstream statmFile("/proc/self/statm");
        if (!statmFile.is_open()) {
            out << "Process RAM bilgisi alinamadi!\n";
        } else {
            unsigned long long residentPages = 0;
            unsigned long long totalPages = 0;
            statmFile >> totalPages >> residentPages;
            if (!statmFile) {
                out << "Process RAM bilgisi alinamadi!\n";
            } else {
                const long pageSize = sysconf(_SC_PAGESIZE);
                const unsigned long long pageSizeBytes = pageSize > 0 ? static_cast<unsigned long long>(pageSize) : 4096ULL;
                const unsigned long long residentKb = (residentPages * pageSizeBytes) / 1024ULL;

                out << "Fiziksel RAM Kullanimi (Working Set) : " << residentKb / 1024ULL << " MB\n";
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

inline void runCpuCheck(std::ostream &out, bool waitForEnter) {
    const auto startTime = std::chrono::steady_clock::now();

    out << "--- Sistem Islemci (CPU) Bilgileri ---\n";
    
    // Uygulamanin o ana kadar harcadigi CPU zamani
    static const long tick_hz = sysconf(_SC_CLK_TCK);
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
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns\n";
}

inline void benchmarkProcessRamApiCost(std::ostream &out, int iterations) {
    out << "--- Process RAM API Cagri Maliyeti (Linux: getrusage()) ---\n";
    
    struct rusage usage;
    
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        getrusage(RUSAGE_SELF, &usage);
    }
    const auto end = std::chrono::steady_clock::now();
    
    long long totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double averageNs = static_cast<double>(totalNs) / iterations;

    out << "Toplam Cagirilan API Sayisi: " << iterations << "\n";
    out << "Toplam Gecen Zaman         : " << (totalNs / 1000.0) << " us\n";
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns\n";
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
    out << "\n--- Ag Trafikii (son degisim) ---\n";
    std::ifstream netDevFile("/proc/net/dev");
    if (!netDevFile.is_open()) {
        out << "Ag trafik bilgisi alinamadi!\n";
    } else {
        std::string line;
        int count = 0;
        while (std::getline(netDevFile, line) && count < 15) {
            if (line.find(':') != std::string::npos) {
                out << line << "\n";
                count++;
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

inline void benchmarkNetworkFileReadCost(std::ostream &out, int iterations) {
    out << "--- Ag Dosya Okuma API Cagri Maliyeti (Linux: /proc/net/dev) ---\n";
    
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        std::ifstream netDevFile("/proc/net/dev");
        std::string line;
        int count = 0;
        while (std::getline(netDevFile, line) && count < 15) {
            if (line.find(':') != std::string::npos) {
                count++;
            }
        }
    }
    const auto end = std::chrono::steady_clock::now();
    
    long long totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double averageNs = static_cast<double>(totalNs) / iterations;

    out << "Toplam Okuma Sayisi: " << iterations << "\n";
    out << "Toplam Gecen Zaman : " << (totalNs / 1000.0) << " us\n";
    out << "Okuma Basina Zaman : " << averageNs << " ns\n";
}

inline void benchmarkNetworkTableCost(std::ostream &out, int iterations) {
    out << "--- Ag Tablo API Cagri Maliyeti (Linux: getifaddrs()) ---\n";
    
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

} // namespace linux_resource_check