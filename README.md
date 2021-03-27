# solatTime (Muslim Prayer Time) for ESP32
A muslim solat (prayer) time application running on ESP32. This app uses folat API is for Malaysia solat time, https:\\api.azanpro.com. The data in JSON format. This app collect the data display them on LCD display and process them to trigger azan audio.  The new release has features for setting of wifi and API area code via web browser from pc, phone etc. make the app is more portable. Details of setup in the link below.
List of Hardware:
1. ESP32. Any ESP32 will do. However the code won't work with Arduino microcontroller. ESP32 has native support for I2S protocol and dual cores.
2. SD card reader. I am using DIY Sd card adapter by soldering the pins directly on to the adapter.
3. DS3231 for time recording (The NTP time is used to update the DS3231).
4. LCD (2004) for text display.
5. MAX98357A - I2S decoder and amplifier
6. Speaker (4 ohm or more)

Software:
1. Code as above.
2. index.html and config.txt files in SPIFFS. Link below describes how to upload the files into SPIFFS of ESP32.

Note: 
1. Update your ESP32 arduino core to at least version 1.05 as audio library require it to compile.
2. For audio file, download any azan audio file you prefer and convert them (using Audacity software or any online audio converter) to wav file. I have not tested
 with other audio file format. Make sampling rate 16000 Hz and stereo. Save them on your SD card.
3. Make sure to fill up your wifi credentials and name of your audio file in the code.
4. Brief hardware setup: https://fornextlife.wordpress.com/2021/03/07/muslim-prayer-time-on-esp32/
5. Video clip: https://www.youtube.com/watch?v=1s4jHMRSESk
 
<img width="411" alt="solatTime" src="https://user-images.githubusercontent.com/78830805/107587834-7532cb00-6c3d-11eb-9ca3-d12b0ef0e7d6.png">
 
Disclaimer:
I am in learning mode. Constructive feedbacks are welcome with thank you.
