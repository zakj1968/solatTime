# ESP32 Audio - Muslim Prayer Time (Solat Time) with Azan
A muslim solat (prayer) time application running on ESP32. This app uses folat API is for Malaysia solat time, https:\\api.azanpro.com. The data in JSON format. This app collect the data display them on LCD display and process them to trigger azan audio.  The new release has features for setting wifi and API area code via web browser from pc, phone etc. thus, the app is more portable. Details of setup in the link below.
List of Hardware:
1. ESP32. Any ESP32 will do. However the code won't work with Arduino microcontroller. ESP32 has native support for I2S protocol and dual cores.
2. SD card reader. I am using DIY Sd card adapter by soldering the pins directly on to the adapter.
3. DS3231 for time recording (The NTP time is used to update the DS3231).
4. LCD (2004) for text display.
5. MAX98357A - I2S decoder and amplifier
6. Speaker (4 ohm or more)

Software:
1. Code as above.
2. index.html and config.txt files in SPIFFS (This two files to be placed in data directory in your Arduino IDE project directory). Follow this link to install SPIFFS uploader in your Arduino IDE: https://github.com/me-no-dev/arduino-esp32fs-plugin#:~:text=Make%20sure%20you%20have%20selected,display%20SPIFFS%20Image%20Uploaded%20message. Use index.html file above. The system will auto-create config.txt file or you can create one and upload to SPIFFS together with index.html

Note: 
1. Use the latest ESP32 arduino core of at least version 1.05 for audio library to compile.
2. For audio file, download any azan audio file you prefer and convert them (using Audacity software or any online audio converter) to wav file. I have not tested
 with other audio file format. Make sampling rate 16000 Hz and stereo. Save them on your SD card.
3. Your wifi credentials and API zone code no more hard coded. Read in the link below how to enter them via web page from PC or phones etc. This is one-time setup unless you change your wifi and API zone.
4. Setup description: https://fornextlife.wordpress.com/2021/03/07/muslim-prayer-time-on-esp32/
5. Video clip: https://www.youtube.com/watch?v=1s4jHMRSESk
 
<img width="411" alt="solatTime" src="https://user-images.githubusercontent.com/78830805/107587834-7532cb00-6c3d-11eb-9ca3-d12b0ef0e7d6.png">
 
Disclaimer:
I am a hobbyist in learning mode. Constructive feedbacks are welcome with thank you.
