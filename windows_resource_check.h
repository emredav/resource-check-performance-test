#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include <windows.h>
#include <psapi.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

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
        const SIZE_T physMemUsedByMe = pmc.WorkingSetSize / (1024 * 1024);

        out << "Fiziksel RAM Kullanimi (Working Set) : " << physMemUsedByMe << " MB\n";
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
    out << std::fixed << std::setprecision(3);
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns (" << (averageNs / 1000.0) << " us)\n";

    if (waitForEnter) {
        out << "\nCikmak icin bir tusa basin...";
        std::cin.get();
    }
}

inline void benchmarkSystemRamApiCost(std::ostream &out, int iterations) {
    out << "--- Sistem RAM API Cagri Maliyeti (Windows: GlobalMemoryStatusEx()) ---\n";
    
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        GlobalMemoryStatusEx(&memInfo);
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
    out << "--- Process RAM API Cagri Maliyeti (Windows: GetProcessMemoryInfo()) ---\n";
    
    PROCESS_MEMORY_COUNTERS pmc;
    HANDLE currentProcess = GetCurrentProcess();

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        GetProcessMemoryInfo(currentProcess, &pmc, sizeof(pmc));
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

    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    ULONG ulOutBufLen = 0;

    // Ag adaptoru bilgisini almak icin gereken buffer boyutunu belirle
    if (GetAdaptersInfo(NULL, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
        
        if (pAdapterInfo && GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter) {
                out << "Arayuz: " << pAdapter->AdapterName << "\n";
                out << "  Aciklama: " << pAdapter->Description << "\n";
                if (pAdapter->Type == MIB_IF_TYPE_ETHERNET) {
                    out << "  Tur: Ethernet\n";
                } else if (pAdapter->Type == MIB_IF_TYPE_PPP) {
                    out << "  Tur: PPP\n";
                } else if (pAdapter->Type == MIB_IF_TYPE_LOOPBACK) {
                    out << "  Tur: Loopback\n";
                } else {
                    out << "  Tur: Diger (" << pAdapter->Type << ")\n";
                }
                pAdapter = pAdapter->Next;
            }
        }
        free(pAdapterInfo);
    } else {
        out << "Ag arayuzu bilgisi alinamadi!\n";
    }

    out << "\n--- Ag Istatistikleri ---\n";
    
    DWORD dwOutBufLen = 0;

    if (GetIfTable(NULL, &dwOutBufLen, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_IFTABLE pIfTable = (PMIB_IFTABLE) malloc(dwOutBufLen);

        if (pIfTable && GetIfTable(pIfTable, &dwOutBufLen, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIfTable->dwNumEntries; ++i) {
                const MIB_IFROW &ifRow = pIfTable->table[i];
                out << "Arayuz: " << ifRow.wszName << "\n";
                out << "  Gelen Bayt: " << ifRow.dwInOctets << "\n";
                out << "  Cikan Bayt: " << ifRow.dwOutOctets << "\n";
                out << "  Gelen Paket: " << ifRow.dwInUcastPkts << "\n";
                out << "  Cikan Paket: " << ifRow.dwOutUcastPkts << "\n";
            }
        }
        free(pIfTable);
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
    out << "--- Ag Bilgisi API Cagri Maliyeti (Windows: GetAdaptersInfo()) ---\n";
    
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        PIP_ADAPTER_INFO pAdapterInfo = NULL;
        ULONG ulOutBufLen = 0;
        
        if (GetAdaptersInfo(NULL, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
            pAdapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
            if (pAdapterInfo) {
                GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
                free(pAdapterInfo);
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

inline void benchmarkNetworkTableCost(std::ostream &out, int iterations) {
    out << "--- Ag Tablosu API Cagri Maliyeti (Windows: GetIfTable2()) ---\n";
    
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        DWORD dwOutBufLen = 0;
        if (GetIfTable(NULL, &dwOutBufLen, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
            PMIB_IFTABLE pIfTable = (PMIB_IFTABLE) malloc(dwOutBufLen);
            if (pIfTable && GetIfTable(pIfTable, &dwOutBufLen, FALSE) == NO_ERROR) {
                free(pIfTable);
            } else {
                free(pIfTable);
            }
        }
    }
    const auto end = std::chrono::steady_clock::now();
    
    long long totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double averageNs = static_cast<double>(totalNs) / iterations;

    out << "Toplam Cagirilan API Sayisi: " << iterations << "\n";
    out << "Toplam Gecen Zaman         : " << (totalNs / 1000.0) << " us\n";
    out << "Cagri Basina API Maliyeti  : " << averageNs << " ns\n";
}

} // namespace windows_resource_check