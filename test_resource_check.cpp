#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include "windows_resource_check.h"
namespace resource_check = windows_resource_check;
const char* PLATFORM_NAME = "Windows";
#else
#include "linux_resource_check.h"
namespace resource_check = linux_resource_check;
const char* PLATFORM_NAME = "Linux";
#endif

namespace {

struct BenchmarkSummary {
    std::string testName;
    long long totalMicroseconds = 0;
    long long averageMicroseconds = 0;
    std::string lastOutput;
};

struct CpuApiSummary {
    std::string testName;
    int iterations = 0;
    double averageNanoseconds = 0.0;
    std::string output;
};

std::string captureResourceCheckOutput() {
    std::ostringstream output;
    resource_check::runResourceCheck(output, false);
    return output.str();
}

std::string captureCpuCheckOutput() {
    std::ostringstream output;
    resource_check::runCpuCheck(output, false);
    return output.str();
}

std::string captureCpuApiCostOutput(int iterations) {
    std::ostringstream output;
    resource_check::benchmarkCpuApiCost(output, iterations, false);
    return output.str();
}

std::string captureNetworkCheckOutput() {
    std::ostringstream output;
    resource_check::runNetworkCheck(output, false);
    return output.str();
}

std::string captureSystemRamApiCostOutput(int iterations) {
    std::ostringstream output;
    resource_check::benchmarkSystemRamApiCost(output, iterations);
    return output.str();
}

std::string captureProcessRamApiCostOutput(int iterations) {
    std::ostringstream output;
    resource_check::benchmarkProcessRamApiCost(output, iterations);
    return output.str();
}

std::string captureNetworkApiCostOutput(int iterations) {
    std::ostringstream output;
    resource_check::benchmarkNetworkApiCost(output, iterations);
    return output.str();
}

std::string captureNetworkTableCostOutput(int iterations) {
    std::ostringstream output;
    resource_check::benchmarkNetworkTableCost(output, iterations);
    return output.str();
}

void requireContains(const std::string &text, const std::string &needle, const std::string &message) {
    if (text.find(needle) == std::string::npos) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

double extractMetricValue(const std::string &text, const std::string &metricPrefix) {
    const std::size_t metricPos = text.find(metricPrefix);
    if (metricPos == std::string::npos) {
        return -1.0;
    }

    const std::size_t valueStart = metricPos + metricPrefix.size();
    const std::size_t valueEnd = text.find_first_of(" \n\r", valueStart);
    const std::string valueText = text.substr(valueStart, valueEnd - valueStart);

    try {
        return std::stod(valueText);
    } catch (...) {
        return -1.0;
    }
}

BenchmarkSummary benchmarkResourceCheck(int iterations) {
    BenchmarkSummary summary;
    summary.testName = "RAM";

    long long totalMicroseconds = 0;
    const auto globalStartTime = std::chrono::steady_clock::now();

    std::string lastOutput;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        const auto startTime = std::chrono::steady_clock::now();
        lastOutput = captureResourceCheckOutput();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - startTime);
        totalMicroseconds += elapsed.count();
    }

    const auto totalElapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - globalStartTime);

    requireContains(lastOutput, "Toplam Fiziksel RAM", "Sistem RAM ciktisi eksik.");
    requireContains(lastOutput, "Fiziksel RAM Kullanimi (Working Set)", "Process RAM ciktisi eksik.");
    requireContains(lastOutput, "Son Dongu Calisma Suresi", "Calisma suresi ciktisi eksik.");

    summary.totalMicroseconds = totalElapsed.count();
    summary.averageMicroseconds = totalMicroseconds / iterations;
    summary.lastOutput = lastOutput;

    return summary;
}

BenchmarkSummary benchmarkCpuCheck(int iterations) {
    BenchmarkSummary summary;
    summary.testName = "CPU";

    long long totalMicroseconds = 0;
    const auto globalStartTime = std::chrono::steady_clock::now();

    std::string lastOutput;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        const auto startTime = std::chrono::steady_clock::now();
        lastOutput = captureCpuCheckOutput();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - startTime);
        totalMicroseconds += elapsed.count();
    }

    const auto totalElapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - globalStartTime);

    requireContains(lastOutput, "Mevcut Process User CPU Zamani", "CPU zamani ciktisi eksik.");
    requireContains(lastOutput, "Son Dongu Calisma Suresi", "Calisma suresi ciktisi eksik.");

    summary.totalMicroseconds = totalElapsed.count();
    summary.averageMicroseconds = totalMicroseconds / iterations;
    summary.lastOutput = lastOutput;

    return summary;
}

CpuApiSummary runCpuApiCostTest(int apiIterations) {
    CpuApiSummary summary;
    summary.testName = "CPU API Cagri";
    summary.iterations = apiIterations;

    summary.output = captureCpuApiCostOutput(apiIterations);

    requireContains(summary.output, "Cagri Basina API Maliyeti", "API Cagri maliyeti ciktisi eksik.");

    summary.averageNanoseconds = extractMetricValue(summary.output, "Cagri Basina API Maliyeti  : ");
    if (summary.averageNanoseconds < 0.0) {
        std::cerr << "API Cagri maliyeti parse edilemedi." << std::endl;
        std::exit(1);
    }

    return summary;
}

CpuApiSummary runSystemRamApiCostTest(int apiIterations) {
    CpuApiSummary summary;
    summary.testName = "Sistem RAM API Cagri";
    summary.iterations = apiIterations;

    summary.output = captureSystemRamApiCostOutput(apiIterations);

    requireContains(summary.output, "Cagri Basina API Maliyeti", "Sistem RAM API Cagri maliyeti ciktisi eksik.");

    summary.averageNanoseconds = extractMetricValue(summary.output, "Cagri Basina API Maliyeti  : ");
    if (summary.averageNanoseconds < 0.0) {
        std::cerr << "Sistem RAM API Cagri maliyeti parse edilemedi." << std::endl;
        std::exit(1);
    }

    return summary;
}

CpuApiSummary runProcessRamApiCostTest(int apiIterations) {
    CpuApiSummary summary;
    summary.testName = "Process RAM API Cagri";
    summary.iterations = apiIterations;

    summary.output = captureProcessRamApiCostOutput(apiIterations);

    requireContains(summary.output, "Cagri Basina API Maliyeti", "Process RAM API Cagri maliyeti ciktisi eksik.");

    summary.averageNanoseconds = extractMetricValue(summary.output, "Cagri Basina API Maliyeti  : ");
    if (summary.averageNanoseconds < 0.0) {
        std::cerr << "Process RAM API Cagri maliyeti parse edilemedi." << std::endl;
        std::exit(1);
    }

    return summary;
}

BenchmarkSummary benchmarkNetworkCheck(int iterations) {
    BenchmarkSummary summary;
    summary.testName = "Network";

    long long totalMicroseconds = 0;
    const auto globalStartTime = std::chrono::steady_clock::now();

    std::string lastOutput;
    for (int iteration = 0; iteration < iterations; ++iteration) {
        const auto startTime = std::chrono::steady_clock::now();
        lastOutput = captureNetworkCheckOutput();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - startTime);
        totalMicroseconds += elapsed.count();
    }

    const auto totalElapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - globalStartTime);

    requireContains(lastOutput, "Ag Arayuzu Bilgileri", "Ag Arayuzu ciktisi eksik.");

    summary.totalMicroseconds = totalElapsed.count();
    summary.averageMicroseconds = totalMicroseconds / iterations;
    summary.lastOutput = lastOutput;

    return summary;
}

CpuApiSummary runNetworkApiCostTest(int apiIterations) {
    CpuApiSummary summary;
    summary.testName = "Network API Cagri";
    summary.iterations = apiIterations;

    summary.output = captureNetworkApiCostOutput(apiIterations);

    requireContains(summary.output, "Cagri Basina API Maliyeti", "Network API Cagri maliyeti ciktisi eksik.");

    summary.averageNanoseconds = extractMetricValue(summary.output, "Cagri Basina API Maliyeti  : ");
    if (summary.averageNanoseconds < 0.0) {
        std::cerr << "Network API Cagri maliyeti parse edilemedi." << std::endl;
        std::exit(1);
    }

    return summary;
}

CpuApiSummary runNetworkTableApiCostTest(int apiIterations) {
    CpuApiSummary summary;
    summary.testName = "Network Tablo API Cagri";
    summary.iterations = apiIterations;

    summary.output = captureNetworkTableCostOutput(apiIterations);

    requireContains(summary.output, "Cagri Basina API Maliyeti", "Network Tablo API Cagri maliyeti ciktisi eksik.");

    summary.averageNanoseconds = extractMetricValue(summary.output, "Cagri Basina API Maliyeti  : ");
    if (summary.averageNanoseconds < 0.0) {
        std::cerr << "Network Tablo API Cagri maliyeti parse edilemedi." << std::endl;
        std::exit(1);
    }

    return summary;
}

} // namespace

int main(int argc, char* argv[]) {
    int repeatCount = 1000000;
    std::string outputFilePath = "benchmark_output.txt";

    if (argc >= 2) {
        try {
            repeatCount = std::stoi(argv[1]);
        } catch (const std::exception&) {
            std::cerr << "Tekrar sayisi gecersiz. Ornek kullanim: test_resource_check.exe 10000 output.txt" << std::endl;
            return 1;
        }

        if (repeatCount <= 0) {
            std::cerr << "Tekrar sayisi pozitif bir tam sayi olmali." << std::endl;
            return 1;
        }
    }

    if (argc >= 3) {
        outputFilePath = argv[2];
    }

    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
        std::cerr << "Output dosyasi acilamadi: " << outputFilePath << std::endl;
        return 1;
    }

    // Repeat count'u API test icin ayarlama (daha hizli API testleri icin daha buyuk sayi)
    int apiIterations = repeatCount;
    int networkIterations = repeatCount / 100; // Network testi daha yavass, daha az tekrar

    BenchmarkSummary ramSummary = benchmarkResourceCheck(repeatCount);
    BenchmarkSummary cpuSummary = benchmarkCpuCheck(repeatCount);
    BenchmarkSummary networkSummary = benchmarkNetworkCheck(networkIterations);
    
    CpuApiSummary systemRamApiSummary = runSystemRamApiCostTest(apiIterations);
    CpuApiSummary processRamApiSummary = runProcessRamApiCostTest(apiIterations);
    CpuApiSummary cpuApiSummary = runCpuApiCostTest(apiIterations);
    CpuApiSummary networkApiSummary = runNetworkApiCostTest(apiIterations / 100); // Network API daha yavass
    CpuApiSummary networkTableApiSummary = runNetworkTableApiCostTest(apiIterations / 100);

    std::ostringstream report;
    report << "========================================\n";
    report << "      SISTEM KAYNAGI BENCHMARK RAPORU\n";
    report << "========================================\n\n";
    report << "[Test Platformu]: " << PLATFORM_NAME << "\n";
#ifdef _WIN32
    report << "[Network Ag Yontemi]: GetAdaptersInfo() ve GetIfTable()\n";
#else
    report << "[Network Ag Yontemi]: getifaddrs() ve /proc/net/dev okuma/ayristirma\n";
#endif
    report << "[Tekrar Sayisi - Genel Testler]: " << repeatCount << "\n";
    report << "[Tekrar Sayisi - RAM API Testleri]: " << apiIterations << "\n";
    report << "[Tekrar Sayisi - CPU API Testleri]: " << apiIterations << "\n";
    report << "[Tekrar Sayisi - Network Testleri]: " << networkIterations << "\n";
    report << "[Tekrar Sayisi - Network API Testleri]: " << (apiIterations / 100) << "\n\n";

    // ===== GENEL RAM TESTI =====
    report << "========================================\n";
    report << "=== GENEL RAM TESTI (Komple Islem) ===\n";
    report << "========================================\n";
    report << "Toplam test calisma suresi (RAM): " << ramSummary.totalMicroseconds << " us\n";
    report << "Ortalama (1 tekrar icin): " << ramSummary.averageMicroseconds << " us\n";
    report << "--- Son Iterasyon Ciktisi (RAM) ---\n";
    report << ramSummary.lastOutput;
    report << "-------------------------------------------\n\n";

    // ===== SISTEM RAM API MALIYET TESTI =====
    report << "========================================\n";
    report << "=== SISTEM RAM API MALIYET TESTI ===\n";
    report << "========================================\n";
    report << systemRamApiSummary.output;
    report << "-------------------------------------------\n\n";

    // ===== PROCESS RAM API MALIYET TESTI =====
    report << "========================================\n";
    report << "=== PROCESS RAM API MALIYET TESTI ===\n";
    report << "========================================\n";
    report << processRamApiSummary.output;
    report << "-------------------------------------------\n\n";

    // ===== GENEL CPU TESTI =====
    report << "========================================\n";
    report << "=== GENEL CPU TESTI (Komple Islem) ===\n";
    report << "========================================\n";
    report << "Toplam test calisma suresi (CPU): " << cpuSummary.totalMicroseconds << " us\n";
    report << "Ortalama (1 tekrar icin): " << cpuSummary.averageMicroseconds << " us\n";
    report << "--- Son Iterasyon Ciktisi (CPU) ---\n";
    report << cpuSummary.lastOutput;
    report << "-------------------------------------------\n\n";

    // ===== CPU API MALIYET TESTI =====
    report << "========================================\n";
    report << "=== CPU API MALIYET TESTI ===\n";
    report << "========================================\n";
    report << cpuApiSummary.output;
    report << "-------------------------------------------\n\n";

    // ===== GENEL NETWORK TESTI =====
    report << "========================================\n";
    report << "=== GENEL NETWORK TESTI (Komple Islem) ===\n";
    report << "========================================\n";
    report << "Toplam test calisma suresi (Network): " << networkSummary.totalMicroseconds << " us\n";
    report << "Ortalama (1 tekrar icin): " << networkSummary.averageMicroseconds << " us\n";
    report << "--- Son Iterasyon Ciktisi (Network) ---\n";
    report << networkSummary.lastOutput;
    report << "-------------------------------------------\n\n";

    // ===== NETWORK API MALIYET TESTLERI =====
    report << "========================================\n";
    report << "=== NETWORK API MALIYET TESTLERI ===\n";
    report << "========================================\n";
    report << networkApiSummary.output;
    report << "-------------------------------------------\n\n";

    report << "========================================\n";
    report << "=== NETWORK TABLO API MALIYET TESTI ===\n";
    report << "========================================\n";
    report << networkTableApiSummary.output;
    report << "-------------------------------------------\n\n";

    // ===== KARSILASTIRMA TABLOSU =====
    report << "========================================\n";
    report << "=== API MALIYET KARSILASTIRMASI ===\n";
    report << "========================================\n\n";
    report << "API Cagri Ortalama Süresi (nanosaniye cinsinden):\n";
    report << "1. Sistem RAM API        : " << systemRamApiSummary.averageNanoseconds << " ns\n";
    report << "2. Process RAM API       : " << processRamApiSummary.averageNanoseconds << " ns\n";
    report << "3. CPU API               : " << cpuApiSummary.averageNanoseconds << " ns\n";
    report << "4. Network API           : " << networkApiSummary.averageNanoseconds << " ns\n";
    report << "5. Network Tablo API     : " << networkTableApiSummary.averageNanoseconds << " ns\n\n";

    // En hizli ve en yavas API'leri belirle
    double minTime = systemRamApiSummary.averageNanoseconds;
    std::string minApi = "Sistem RAM API";
    double maxTime = systemRamApiSummary.averageNanoseconds;
    std::string maxApi = "Sistem RAM API";

    if (processRamApiSummary.averageNanoseconds < minTime) {
        minTime = processRamApiSummary.averageNanoseconds;
        minApi = "Process RAM API";
    }
    if (cpuApiSummary.averageNanoseconds < minTime) {
        minTime = cpuApiSummary.averageNanoseconds;
        minApi = "CPU API";
    }
    if (networkApiSummary.averageNanoseconds < minTime) {
        minTime = networkApiSummary.averageNanoseconds;
        minApi = "Network API";
    }
    if (networkTableApiSummary.averageNanoseconds < minTime) {
        minTime = networkTableApiSummary.averageNanoseconds;
        minApi = "Network Tablo API";
    }

    if (processRamApiSummary.averageNanoseconds > maxTime) {
        maxTime = processRamApiSummary.averageNanoseconds;
        maxApi = "Process RAM API";
    }
    if (cpuApiSummary.averageNanoseconds > maxTime) {
        maxTime = cpuApiSummary.averageNanoseconds;
        maxApi = "CPU API";
    }
    if (networkApiSummary.averageNanoseconds > maxTime) {
        maxTime = networkApiSummary.averageNanoseconds;
        maxApi = "Network API";
    }
    if (networkTableApiSummary.averageNanoseconds > maxTime) {
        maxTime = networkTableApiSummary.averageNanoseconds;
        maxApi = "Network Tablo API";
    }

    report << "\nEn Hizli API         : " << minApi << " (" << minTime << " ns)\n";
    report << "En Yavas API         : " << maxApi << " (" << maxTime << " ns)\n";
    report << "Hiz Farki            : " << (maxTime / minTime) << "x\n";

    std::cout << report.str();
    outputFile << report.str();

    std::cout << "\nRapor dosyaya yazildi: " << outputFilePath << std::endl;
    return 0;
}