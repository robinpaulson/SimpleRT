# SimpleRT
Reverse [Tethering](https://en.wikipedia.org/wiki/Tethering) for Android.

Allows you to share your computer's internet connection with your Android device via a USB cable.

### No root, no adb required!

Linux & OSX supported! Windows - never mind (in the near future).

- Android part:

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

   Makefile is universal for both linux and osx, just type make.
   
Usage:

- run console util as root
- connect your android device

Issues: Some apps do not recognize the reverse tethered internet connection due to ConnectivityService policy. Just leave WiFi or 3g connection active.

Partially used code from [linux-adk](https://github.com/gibsson/linux-adk).

#####License: GNU GPL v3
