# CalendarFlyout

**A lightweight native Windows calendar with a clean, Fluent-inspired interface.**

CalendarFlyout is a small desktop calendar application that lives in your Windows system tray. It gives you quick access to a monthly calendar without opening a full calendar application or using a web browser.

Built with **C++20 and the native Windows API**, CalendarFlyout is designed to be lightweight, simple, and easy to use.

---

## Table of Contents

### 🇮🇩 Bahasa Indonesia
1. [Tentang CalendarFlyout](#-bahasa-indonesia)
2. [Fitur](#fitur)
3. [Tampilan Aplikasi](#tampilan-aplikasi)
4. [Cara Menggunakan](#cara-menggunakan)
5. [Persyaratan Sistem](#persyaratan-sistem)
6. [Cara Build dari Source Code](#cara-build-dari-source-code)
7. [Lisensi](#lisensi)

### 🇬🇧 English
1. [About CalendarFlyout](#-english)
2. [Features](#features)
3. [Screenshots](#screenshots)
4. [How to Use](#how-to-use)
5. [System Requirements](#system-requirements)
6. [Build from Source](#build-from-source)
7. [License](#license)

---

# 🇮🇩 Bahasa Indonesia

## Tentang CalendarFlyout

CalendarFlyout adalah aplikasi kalender desktop sederhana yang berada di area **system tray** Windows (area ikon kecil di sebelah jam dan tanggal pada taskbar).

Dengan satu klik, kamu bisa membuka kalender bulanan tanpa harus membuka aplikasi kalender yang besar.

Aplikasi ini dibuat menggunakan C++20 dan API bawaan Windows, bukan menggunakan framework aplikasi web atau browser.

Tujuan CalendarFlyout adalah menyediakan kalender yang:
- Ringan dan sederhana.
- Cepat diakses dari taskbar.
- Memiliki tampilan modern yang terinspirasi desain Fluent Windows.
- Tidak membutuhkan akun atau koneksi internet untuk fungsi kalender dasarnya.

## Fitur

- **Akses melalui System Tray** — buka kalender langsung dari ikon di taskbar.
- **Navigasi Bulanan** — berpindah ke bulan sebelumnya dan berikutnya.
- **Penanda Hari Ini** — tanggal hari ini ditampilkan dengan penanda khusus.
- **Tema Terang dan Gelap** — tampilan mengikuti tema Windows.
- **Desain Minimalis** — antarmuka sederhana dengan sudut membulat.
- **Interaksi Mouse** — dukungan hover dan klik pada elemen kalender.
- **Run at Startup** — pilihan untuk menjalankan aplikasi otomatis saat Windows dimulai.
- **Show on Hover** — pilihan untuk membuka kalender ketika kursor diarahkan ke ikon tray.

## Tampilan Aplikasi

Berikut beberapa tampilan CalendarFlyout:

<!-- Ganti gambar berikut dengan screenshot milikmu -->
![CalendarFlyout - Light Mode](docs/images/calendar-light.png)

![CalendarFlyout - Dark Mode](docs/images/calendar-dark.png)

## Cara Menggunakan

1. Unduh file aplikasi dari halaman [Releases](https://github.com/sholihdinrizky/CalenderFluentWindows/releases).
2. Ekstrak file jika aplikasi didistribusikan dalam ZIP.
3. Jalankan `CalendarFlyout.exe`.
4. Temukan ikon CalendarFlyout di system tray Windows.
5. Klik ikon untuk membuka kalender.

Kamu juga dapat menggunakan menu klik kanan pada ikon tray untuk mengakses pengaturan aplikasi.

## Persyaratan Sistem

- Windows 11 (platform utama yang dituju).
- Arsitektur Windows x64.
- Tidak membutuhkan koneksi internet untuk fungsi kalender dasar.

Kompatibilitas Windows 10 belum dinyatakan secara resmi dan masih perlu pengujian.

## Cara Build dari Source Code

Jika ingin mengompilasi aplikasi sendiri, siapkan:

- Windows 10 atau Windows 11.
- C++20 compiler (MSYS2 UCRT64 GCC direkomendasikan).
- CMake versi 3.20 atau lebih baru.
- Ninja Build.

### Langkah-langkah

1. Clone repository:

   ```bash
   git clone https://github.com/sholihdinrizky/CalenderFluentWindows.git
   ```

2. Masuk ke folder proyek:

   ```bash
   cd CalenderFluentWindows
   ```

3. Jalankan:

   ```cmd
   build.bat
   ```

4. Setelah proses build berhasil, file aplikasi akan tersedia di:

   ```text
   build/CalendarFlyout.exe
   ```

## Lisensi

CalendarFlyout menggunakan **MIT License**.

Kamu bebas menggunakan, mempelajari, memodifikasi, dan mendistribusikan source code sesuai ketentuan lisensi.

Lihat file [LICENSE](LICENSE) untuk informasi lengkap.

---

# 🇬🇧 English

## About CalendarFlyout

CalendarFlyout is a simple desktop calendar application that lives in the **Windows system tray** (the small icon area next to the clock and date on your taskbar).

With a single click, you can open a monthly calendar without launching a full calendar application.

Built with C++20 and native Windows APIs, CalendarFlyout does not rely on web application frameworks or browser engines.

The goal is to provide a calendar that is:
- Lightweight and simple.
- Easily accessible from the taskbar.
- Designed with a modern, Fluent-inspired Windows interface.
- Independent of accounts or an internet connection for basic calendar functionality.

## Features

- **System Tray Access** — open the calendar directly from the taskbar.
- **Monthly Navigation** — move between previous and next months.
- **Today Indicator** — highlights the current date.
- **Light and Dark Themes** — adapts to the Windows theme.
- **Minimal Design** — a clean interface with rounded corners.
- **Mouse Interaction** — hover and click support for calendar elements.
- **Run at Startup** — optionally launch the application when Windows starts.
- **Show on Hover** — optionally open the calendar when hovering over the tray icon.

## Screenshots

Here are some screenshots of CalendarFlyout:

![CalendarFlyout - Light Mode](docs/images/calendar-light.png)

![CalendarFlyout - Dark Mode](docs/images/calendar-dark.png)

## How to Use

1. Download the application from the [Releases](https://github.com/sholihdinrizky/CalenderFluentWindows/releases) page.
2. Extract the downloaded archive if the application is distributed as a ZIP file.
3. Run `CalendarFlyout.exe`.
4. Find the CalendarFlyout icon in the Windows system tray.
5. Click the icon to open the calendar.

You can also right-click the tray icon to access application settings.

## System Requirements

- Windows 11 (primary target platform).
- Windows x64 architecture.
- No internet connection required for basic calendar functionality.

Windows 10 compatibility has not been officially confirmed and still requires testing.

## Build from Source

To compile the application yourself, you will need:

- Windows 10 or Windows 11.
- A C++20 compiler (MSYS2 UCRT64 GCC recommended).
- CMake 3.20 or later.
- Ninja Build.

### Steps

1. Clone the repository:

   ```bash
   git clone https://github.com/sholihdinrizky/CalenderFluentWindows.git
   ```

2. Navigate to the project directory:

   ```bash
   cd CalenderFluentWindows
   ```

3. Run:

   ```cmd
   build.bat
   ```

4. After a successful build, the executable will be available at:

   ```text
   build/CalendarFlyout.exe
   ```

## License

CalendarFlyout is licensed under the **MIT License**.

You are free to use, study, modify, and distribute the source code under the terms of the license.

See the [LICENSE](LICENSE) file for full details.

---

**Made with C++20 and native Windows APIs.**
