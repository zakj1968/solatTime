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
 7.2: Solat time display, presently on LCD 2004 display. In future I intent to display on bigger screen/display.
 7.3: Time recording time using DS3231, mainly for time comparison and current time display on the screen. The NTP time is acquired
      to update the DS3231. NTP time is like a backup for DS3231.
 7.4: Microprocessor: ESP32. This microprocessor is used because it's 2 cores processor and more importantly it support I2S protocol natively.
 8. Features at present is very basic. It fetches API data twice a day, format data and display them on screen. The code will compare RTC time
    and API solat time and call audio file and play it from SD card.
 9. Future feature upgrade:
  9.1: Parameters for WiFi and solat API parameters setup via web-based (websocket) interface. Therefore the gadget will be portable. At present it is hard-coded.
  9.2: Bigger display like double height MAX7219 display.
  9.3: Volume control, both hardware and software.
  9.4: Read Qurán, may be 10 minutes before solat time or read at any time selected.
  9.5: Improve ESP32 energy saving mode e.g. sleeping mode etc.
