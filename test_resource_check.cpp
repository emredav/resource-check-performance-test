#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
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

void benchmarkResourceCheck(int iterations) {
    std::cout << "[Test Platformu]: " << PLATFORM_NAME << " algilandi ve RAM test baslatiliyor...\n\n";

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

    const long long averageMicroseconds = totalMicroseconds / iterations;

    std::cout << "Toplam test calisma suresi (RAM): " << totalElapsed.count() << " us" << std::endl;
    std::cout << "Ortalama (1 tekrar icin) calisma suresi: " << averageMicroseconds << " us\n" << std::endl;
    std::cout << "--- Son Iterasyon Ciktisi (RAM) ---" << std::endl;
    std::cout << lastOutput;
}

void benchmarkCpuCheck(int iterations) {
    std::cout << "[Test Platformu]: " << PLATFORM_NAME << " algilandi ve CPU test baslatiliyor...\n\n";

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

    const long long averageMicroseconds = totalMicroseconds / iterations;

    std::cout << "Toplam test calisma suresi (CPU): " << totalElapsed.count() << " us" << std::endl;
    std::cout << "Ortalama (1 tekrar icin) calisma suresi: " << averageMicroseconds << " us\n" << std::endl;
    std::cout << "--- Son Iterasyon Ciktisi (CPU) ---" << std::endl;
    std::cout << lastOutput;
}

void runCpuApiCostTest(int apiIterations) {
    std::cout << "[Test Platformu]: " << PLATFORM_NAME << " algilandi ve islemci API maliyet testi baslatiliyor...\n\n";
    
    std::string apiOutput = captureCpuApiCostOutput(apiIterations);
    
    requireContains(apiOutput, "Cagri Basina API Maliyeti", "API Cagri maliyeti ciktisi eksik.");
    
    std::cout << apiOutput;
}

} // namespace

int main() {
    benchmarkResourceCheck(1000000);
    std::cout << "-------------------------------------------\n\n";
    benchmarkCpuCheck(1000000);
    std::cout << "-------------------------------------------\n\n";
    runCpuApiCostTest(1000000);
    return 0;
}