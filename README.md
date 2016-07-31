# SimpleRT
Reverse [Tethering](https://en.wikipedia.org/wiki/Tethering) utility for Android.

Allows you to share your computer's internet connection with your Android device via a USB cable.

Development is still in progress, bugs and errors can occur.

### No root, no adb required!

Linux & OSX are supported! Windows is not, and unlikely to be in the future.

- Android part:

   Implemented as standalone service, no gui, no activities. Simplicity!

   Dependencies:
   - Android 4.0 and higher.

   Build system based on gradle + gradle experimental android plugin (supporting ndk). For build you need both sdk & ndk.
Create local.properties file in root dir, it should be looks like that:
   ```
   ndk.dir=/home/viper/Android/Sdk/ndk-bundle
   sdk.dir=/home/viper/Android/Sdk
   ```
   build:
   ```
   ./gradlew assembleDebug
   ```
   app/build/outputs/apk/app-debug.apk is your apk.

- Desktop part:

   Dependencies:
   - libusb-1.0
   - tun/tap kernel module. osx version is [here](http://tuntaposx.sourceforge.net/).

   ####FIrst of all, check out iface_up.sh and change LOCAL_INTERFACE, if you need.
   
   Makefile is universal for both linux and osx, just type make.
   
Usage:

- run console util as root
- connect your android device

First connection requires some trivial steps:

![First step]
(https://github.com/vvviperrr/SimpleRT/blob/master/screens/accessory.png)

![Second step]
(https://github.com/vvviperrr/SimpleRT/blob/master/screens/vpn.png)

![Third step]
(https://github.com/vvviperrr/SimpleRT/blob/master/screens/connected.png)

Issues: Some apps do not recognize the reverse tethered internet connection due to ConnectivityManager policy. Just leave WiFi or 3g connection active, connection will go through SimpleRT anyway.

Partially used code from [linux-adk](https://github.com/gibsson/linux-adk).

#####License: GNU GPL v3
