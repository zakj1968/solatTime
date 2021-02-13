# solatTime
A muslim solat (prayer) time application running on ESP32. 
List of Hardware:
1. ESP32
2. SD card reader 
3. DS3231
4. LCD (2004)
5. MAX98357A - I2S decoder and amplifier
6. Speaker (4 ohm or more)
7. Characteristics:
 7.1: API: api.azanpro.com - Thanks for the best solat API provider for Malaysia's prayer time.
 7.2: Solat time display, presently on LCD 2004 display. In future I intend to display on bigger screen/display.
 7.3: Time recording time using DS3231, mainly for time comparison and current time display on the screen. This more stable and suitable in most sittuation. The NTP time is acquired to update the DS3231.
 7.4: Microprocessor: ESP32. This microprocessor is used because its 2 cores processor and more importantly it support I2S protocol natively.
 8. Features at present is very basic. It fetches API data twice a day, format,display them on screen and triggered to play Azan from SD card when solat time is reached.
 9. Future feature upgrade:
  9.1: Parameters for WiFi and solat API parameters setup via web-based (websocket) interface. Therefore the gadget will be portable. At present it is hard-coded.
  9.2: Bigger display like double height MAX7219 display.
  9.3: Hardware volume control.
  9.4: Read Qurán, may be 10 minutes before solat time or read at any time selected.
  9.5: Improve ESP32 energy saving mode e.g. sleeping mode etc.
 
 At time this writing you need to upgrade ESP32 arduino core to 1.05 (required by audio library) which is available at ESP developmental branch, otherwise the
 code won't compile. Visit here how to do it:  https://github.com/espressif/arduino-esp32#installation-instructions
 
 For audio file, download any azan audio file you prefer and convert them (using Audacity software or any online audio converter) to wav file. I have not tested
 with other audio file format. Make sampling rate 16000 Hz and stereo. Save them on your SD card.
 
 Note: Issue in WiKi.
 
<img width="411" alt="solatTime" src="https://user-images.githubusercontent.com/78830805/107587834-7532cb00-6c3d-11eb-9ca3-d12b0ef0e7d6.png">
 
Disclaimer:
I am just a hobbyist with minimal knowledge on this technology, so don't expect high expectation from me. You are welcome if you would like to contribute to make this project a more usable one.
