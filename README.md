# Test Resource Check

Bu proje hem Linux hem de Windows sistemlerde çalışan ve sistem belleği (RAM), işlemci (CPU) ve ağ bilgilerini yüksek hassasiyetle benchmark test eden C/C++ kodlarını içerir. Her API çağrısının maliyeti nanosaniye düzeyinde ölçülür.

## Dosya Yapısı

* `linux_resource_check.h` : Linux'a özgü sistem kaynağı çekme implementasyonları:
  - `sysinfo()` - Sistem RAM bilgisi (122.5 ns)
  - `getrusage()` - Process RAM bilgisi (179.3 ns)
  - `times()` - CPU zaman bilgisi (143.0 ns)
  - `getifaddrs()` - Ağ arabirüzü bilgisi (63.4 µs)
  
* `windows_resource_check.h` : Windows'a özgü sistem kaynağı çekme implementasyonları:
  - `GlobalMemoryStatusEx()` - Sistem RAM bilgisi
  - `GetProcessMemoryInfo()` - Process RAM bilgisi
  - `GetProcessTimes()` - CPU zaman bilgisi
  - `GetAdaptersInfo()` ve `GetIfTable2()` - Ağ bilgisi
  
* `test_resource_check.cpp` : Kapsamlı benchmark test harness'i:
  - İşletim sistemini otomatik tespit eder
  - Sistem kaynakları çekme sürelerini ölçer
  - Her API çağrısının maliyetini nanosaniye (ns) cinsinden hesaplar
  - Platform arası adil karşılaştırma sağlar
  - Detaylı raporlama ve analiz

## Detaylı Özellikler

### 1. RAM Testleri
- **Genel RAM Testi**: Toplam sistem RAM'i, kullanılan ve boş RAM'i ölçer (1 iterasyon = ~1 µs)
- **Sistem RAM API Maliyeti**: `sysinfo()` (Linux) vs `GlobalMemoryStatusEx()` (Windows)
- **Process RAM API Maliyeti**: `getrusage()` (Linux) vs `GetProcessMemoryInfo()` (Windows)

### 2. CPU Testleri
- **Genel CPU Testi**: Process'in tükettiği CPU zamanını (User, Kernel, Total) ölçer
- **CPU API Maliyeti**: `times()` (Linux) vs `GetProcessTimes()` (Windows)

### 3. Ağ Testleri (Yeni!)
- **Genel Ağ Testi**: Aktif ağ arabirüzlerini ve trafik bilgisini gösterir
- **Ağ API Maliyeti**: `getifaddrs()` (Linux) vs `GetAdaptersInfo()` (Windows)
- **Ağ Tablo API Maliyeti**: `getifaddrs()` (Linux) vs `GetIfTable2()` (Windows)

### 4. Karşılaştırmalı Analiz
- Tüm API çağrılarının ortalama süresi (nanosaniye cinsinden)
- En hızlı ve en yavaş API'lerin tanımlanması
- API'ler arasındaki performans oranı

## Derleme ve Çalıştırma

### Linux

```bash
# Derleme
g++ -std=c++11 test_resource_check.cpp -o test_resource_check -lrt

# Varsayılan çalıştırma (1 milyon tekrar)
./test_resource_check

# Kustom tekrar sayısı ve output dosyasıyla
./test_resource_check 100000 benchmark_output.txt
```

### Windows (MinGW)

```bash
g++ -std=c++11 test_resource_check.cpp -o test_resource_check.exe -lpsapi -liphlpapi -lws2_32

test_resource_check.exe 100000 benchmark_output.txt
```

### Windows (MSVC)

```cmd
cl /std:c++latest test_resource_check.cpp /link psapi.lib iphlpapi.lib ws2_32.lib
test_resource_check.exe 100000 benchmark_output.txt
```

### Linux'ta Windows İçin Çapraz Derleme

```bash
x86_64-w64-mingw32-g++ -std=c++11 test_resource_check.cpp -o test_resource_check.exe -lpsapi -liphlpapi -lws2_32
```

## Örnek Çıktı (Linux - 100,000 Tekrar)

```
========================================
      SISTEM KAYNAGI BENCHMARK RAPORU
========================================

[Test Platformu]: Linux
[Tekrar Sayisi - Genel Testler]: 100000
[Tekrar Sayisi - RAM API Testleri]: 100000
[Tekrar Sayisi - CPU API Testleri]: 100000
[Tekrar Sayisi - Network Testleri]: 1000
[Tekrar Sayisi - Network API Testleri]: 1000

========================================
=== API MALIYET KARSILASTIRMASI ===
========================================

API Cagri Ortalama Süresi (nanosaniye cinsinden):
1. Sistem RAM API        : 122.504 ns
2. Process RAM API       : 179.306 ns
3. CPU API               : 142.994 ns
4. Network API           : 63434.4 ns
5. Network Tablo API     : 63534.5 ns

En Hizli API         : Sistem RAM API (122.504 ns)
En Yavas API         : Network Tablo API (63534.5 ns)
Hiz Farki            : 518.632x
```

## Kullanım

```bash
# Varsayılan: 1,000,000 iterasyon, benchmark_output.txt dosyasına yaz
./test_resource_check

# Kustom iterasyon sayısı
./test_resource_check 10000

# Kustom iterasyon sayısı ve output dosyası
./test_resource_check 100000 my_benchmark.txt
```

## API Maliyeti Özeti

Linux sistemlerde nanosaniye düzeyinde ölçülen API çağrı maliyetleri:

| API | Adı | Ortalama Maliyet |
|-----|-----|------------------|
| sysinfo() | Sistem RAM | ~122 ns |
| getrusage() | Process RAM | ~179 ns |
| times() | CPU Zaman | ~143 ns |
| getifaddrs() | Ağ Info | ~63.4 µs |

## Notlar

- Tüm süreler **nanosaniye (ns)** cinsinden ölçülür
- Network API'leri diğerlerinden önemli ölçüde daha yavaştır
- RAM API'leri en hızlı ve en kararlı sonuçları verirler
- Test sonuçları sistem konfigürasyonuna bağlıdır
- Her işlem için ayrı API benchmark testleri mevcuttur
