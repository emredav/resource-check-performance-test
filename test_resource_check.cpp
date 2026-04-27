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

    BenchmarkSummary ramSummary = benchmarkResourceCheck(repeatCount);
    BenchmarkSummary cpuSummary = benchmarkCpuCheck(repeatCount);
    CpuApiSummary apiSummary = runCpuApiCostTest(repeatCount);

    std::ostringstream report;
    report << "[Test Platformu]: " << PLATFORM_NAME << "\n";
    report << "Tekrar Sayisi   : " << repeatCount << "\n\n";

    report << "=== RAM Testi ===\n";
    report << "Toplam test calisma suresi (RAM): " << ramSummary.totalMicroseconds << " us\n";
    report << "Ortalama (1 tekrar icin) calisma suresi: " << ramSummary.averageMicroseconds << " us\n\n";
    report << "--- Son Iterasyon Ciktisi (RAM) ---\n";
    report << ramSummary.lastOutput;
    report << "-------------------------------------------\n\n";

    report << "=== CPU Testi ===\n";
    report << "Toplam test calisma suresi (CPU): " << cpuSummary.totalMicroseconds << " us\n";
    report << "Ortalama (1 tekrar icin) calisma suresi: " << cpuSummary.averageMicroseconds << " us\n\n";
    report << "--- Son Iterasyon Ciktisi (CPU) ---\n";
    report << cpuSummary.lastOutput;
    report << "-------------------------------------------\n\n";

    report << "=== CPU API Maliyet Testi ===\n";
    report << apiSummary.output;
    report << "-------------------------------------------\n\n";

    std::cout << report.str();
    outputFile << report.str();

    std::cout << "\nRapor dosyaya yazildi: " << outputFilePath << std::endl;
    return 0;
}