# Test Resource Check

Bu proje hem Linux hem de Windows sistemlerde çalışan ve sistem belleği (RAM) tüketimini (genel ve çalışan uygulamanın anlık kullanımı dahil) izleyen C/C++ kodlarını içerir.

## Dosya Yapısı

* `linux_resource_check.h` : Linux'a özgü `sysinfo` ve `/proc/self/status` okumalarını barındıran çekirdek implementasyon arayüzü.
* `windows_resource_check.h` : Windows'a özgü `MEMORYSTATUSEX` ve `PROCESS_MEMORY_COUNTERS` okumalarını barındıran çekirdek implementasyon arayüzü.
* `test_resource_check.cpp` : Hangi işletim sisteminde (Windows mu, Linux mu) derlendiğini otomatik tespit eder. Sistem CPU ve RAM kullanım bilgilerini almak için işletim sistemlerinin apilerini test eder ve işletim sistemlerine göre CPU API'leri çağırmanın nanometre bazındaki gecikme maliyetlerini (cost) benchmark raporu olarak sunar.

## Detaylı Özellikler

- **RAM Kullanımı Testi**: Her iki işletim sisteminde donanıma ait genel fiziksel bellek kullanımı ile process (uygulama) bazlı anlık RAM yükünü (`Working Set` / `resident set`) ölçer. Saniye/mikrosaniye mertebesinde çalışma sürelerini tespit edip averaj sürelerini söyler.
- **CPU Kullanımı Testi**: Process’in güncel tükettiği CPU zamanını (User Time, Kernel Time, Total Time) ölçen rutinleri içerir ve ölçümlerin ortalama okuma/değerlendirme sürelerini belirtir.
- **API Maliyet Benchmark’ı**: Linux üzerinde `times()` ile Windows üzerinde `GetProcessTimes()` gibi işletim sistemine özel sistem çağrılarının saf işlem yükünü (Cost) ve tekil çağrı maliyetlerini yüz binlerce tekrara sokarak nano saniye (ns) düzeyinde karşılaştırmalı olarak bildirir.

## Derleme (Linux)

Linux makinenizde `g++` kullanarak testleri ve Linux için uygulamanızı derleyebilirsiniz:

```bash
# Test uygulamanızı derlemek ve benchmark koşmak için
g++ -std=c++17 test_resource_check.cpp -o test_resource_check
./test_resource_check
```

## Derleme (Windows)

Windows üzerinden `MinGW`/`g++` veya `MSVC` ile derleyebilirsiniz (`psapi` kütüphanesini bağlamayı unutmayın):

Windows'ta MinGW üzerinden:
```cmd
g++ -std=c++17 test_resource_check.cpp -o test_resource_check.exe -lpsapi
test_resource_check.exe
```

Eğer Linux içerisinde Windows için çapraz (cross) derleme yapmak isterseniz:
```bash
x86_64-w64-mingw32-g++ -std=c++17 windows.cpp -o windows_resource_check.exe -lpsapi
```
*(Sonrasında `.exe` dosyasını Windows ortamında test edebilirsiniz).*

## Çıktı Örneği

Testi çalıştırdığınızda otomatik algılanan işletim sistemiyle beraber şu formatta bir metrik bildirilir:

```
Son Dongu Calisma Suresi: 7 us
-------------------------------------------

[Test Platformu]: Linux algilandi ve CPU test baslatiliyor...

Toplam test calisma suresi (CPU): 53 us
Ortalama (1 tekrar icin) calisma suresi: 0 us

--- Son Iterasyon Ciktisi (CPU) ---
--- Sistem Islemci (CPU) Bilgileri ---
Mevcut Process User CPU Zamani  : 0 saniye
Mevcut Process Kernel CPU Zamani: 0 saniye
Mevcut Process Toplam CPU Zamani: 0 saniye

Son Dongu Calisma Suresi: 0 us
-------------------------------------------

[Test Platformu]: Linux algilandi ve islemci API maliyet testi baslatiliyor...

--- Sistem Islemci (CPU) API Cagri Maliyeti (Linux: times()) ---
Toplam Cagirilan API Sayisi: 1000000
Toplam Gecen Zaman         : 39598.6 us
Cagri Basina API Maliyeti  : 39.5986 ns
```