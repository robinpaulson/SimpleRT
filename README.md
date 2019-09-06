# SimpleRT
Reverse [Tethering](https://en.wikipedia.org/wiki/Tethering) utility for Android.

Allows you to share your computer's internet connection with your Android device via a USB cable.

Development is still in progress, bugs and errors can occur.

```
IMPORTANT!
   If you have any issues with this tool, please, provide some logs:
   - run util in debug mode (-d), connect your device
   - run "ip addr show"
   - run "ip route show"
   - store this output into issue ticket on github
```

```
FIRST RUN: check out -h option!
   simple-rt -h
   usage: sudo ./simple-rt [-h] [-i interface] [-n nameserver|"local" ]
   default params: -i eth0 -n 8.8.8.8
```

### No root, no adb required!

## Full Linux and macOS support! Windows version is in early researching.

   Current version features:
   - Multi-tether. You can connect several android devices into one virtual network!
   - DNS server specifying (custom or system one).

The SimpleRT utility consists of 2 parts:

- Android part:

   Implemented as standalone service, no gui, no activities. Simplicity!

   Dependencies:
   - Android 4.0 and higher.

   Build system based on gradle + gradle experimental android plugin (supporting ndk). For build you need both sdk & ndk.
Create local.properties file in root dir, it should be looking like that:
   ```
   ndk.dir=/home/viper/Android/Sdk/ndk-bundle
   sdk.dir=/home/viper/Android/Sdk
   ```
   build:
   ```
   ./gradlew assembleDebug
   ```
   app/build/outputs/apk/app-debug.apk is your apk.

   A prebuilt apk can also be found under `Downloads` at the [Github releases](https://github.com/iteratec/SimpleRT/releases)
   tab.

- Desktop part:

   Dependencies:
   - libusb-1.0
   - libresolv (usually already present in both linux and macos)
   - tuntap kernel module (linux version), utun (macos version, builtin)

   before build (debian based example):
   ```
   sudo apt-get install build-essential pkg-config libusb-1.0-0-dev
   ```

   Makefile is universal for all platforms, just type "make && make install" to install it in prefix /usr/local and
   check /usr/local/sbin is in your PATH-variable.  
   If you want to change the prefix path eg. to install it in /usr instead of /usr/local you can
   type "make prefix=/usr && make prefix=/usr install".  
   If you don't want to install but just run it from within build directory you can type "make iface_up_sh_path=.".

Usage:

- run console util as root
- connect your android device

First connection requires some trivial steps:

![First step](screens/accessory.png)

![Second step](screens/vpn.png)

![Third step](screens/connected.png)

Issues: Some apps do not recognize the reverse tethered internet connection due to ConnectivityManager policy. Just leave WiFi or 3g connection active, connection will go through SimpleRT anyway.

Partially uses code from [linux-adk](https://github.com/gibsson/linux-adk), which is licensed under the GNU GPLv2 or later. This project is under the GNU GPLv3 or later, which is compatible with the license of linux-adk.

##### License: GNU GPL v3

```
SimpleRT: Reverse tethering utility for Android
Copyright (C) 2016 Konstantin Menyaev

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
```
