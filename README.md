# ESP32 Audio - Muslim Prayer Time (Solat Time) with Azan
A muslim solat (prayer) time running on ESP32. This app uses solat API is for Malaysia solat time, https:\\api.azanpro.com. The data in JSON format. This app collect the data display them on LCD display and process them to trigger azan audio.  The new release has features for setting wifi and API area code via web browser from pc, phone etc. thus, the app is more portable. Details of setup in the link below.
List of Hardware:
1. ESP32. Any ESP32 will do. However the code won't work with Arduino microcontroller. ESP32 has native support for I2S protocol and dual cores.
2. SD card reader. I am using DIY Sd card adapter by soldering the pins directly on to the adapter.
3. UPDATE 26/4/2021: External rtc no more required.
4. LCD (2004) for text display.
5. MAX98357A - I2S decoder and amplifier
6. Speaker (4 ohm or more)

Software:
1. Code as above.
2. index.html to be handled by ESP32's SPIFFS. This file must be placed in data directory. Create this directory in same folder of arduino sketch. Visit the link below to give an idea how to install ESP32 Sketch data Upload. The system will auto-create config.txt file for wifi and API setup.

Note: 
1. Use the latest ESP32 arduino core of at least version 1.05 for audio library to compile.
2. For audio file, download any azan audio file you prefer and convert them (using Audacity software or any online audio converter) to wav file. I have not tested
 with other audio file format. Make sampling rate 16000 Hz and stereo. Save them on your SD card.
3. Your wifi credentials and API zone code no more hard coded. The setup can be done via web page from PC or phones etc. This is one-time setup unless you change your wifi and API zone.
4. Setup description: https://fornextlife.wordpress.com/2021/03/07/muslim-prayer-time-on-esp32/
5. Video clip: https://www.youtube.com/watch?v=1s4jHMRSESk
6. NOTE: You need separate azan audio files for fajr(subuh) and other solat. Create these files in wav format and place them in SD card. In the code I named it as azSubuh.wav and azan.wav respectively. You can change filename as you prefer, but for reliability please use short 8.3 file name.

# UPDATE: Version 2.2 - external rtc is removed. Time acquired from NTP server.
 
<img width="411" alt="solatTime" src="https://user-images.githubusercontent.com/78830805/107587834-7532cb00-6c3d-11eb-9ca3-d12b0ef0e7d6.png">
 
Disclaimer:
I am a hobbyist in learning mode. Constructive feedbacks are welcome with thank you.
