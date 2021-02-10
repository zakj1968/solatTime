
//Attention:
//1.At time this code written ESP32 arduino core was at version 1.04. You need to upgrade it to version 1.05 from development
//branch, otherwise your code won't compile.
// Visit here how to do it: https://github.com/espressif/arduino-esp32#installation-instructions
//2.Only tested on audio wav format (sampling rate 16000 Hz, stereo).
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // https://arduinojson.org/?utm_source=meta&utm_medium=library.properties
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h> // Library by Fabrice weinberg. This lib do not contain formattedDate method.
#include <WiFiUdp.h>
#include <Wire.h>
#include <RtcDS3231.h> //https://github.com/Makuna/Rtc/wiki
/////////////////Audio
#include "Arduino.h"
#include "Audio.h" //https://github.com/schreibfaul1/ESP32-audioI2S
#include "SD.h"
#include "FS.h"
/////////////////////
// to use second core - core 0
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

// SD card pins
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
//I2S pins
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26
/////////////////
TaskHandle_t Task1; // Core 0 task
Audio audio;
RtcDS3231<TwoWire> Rtc(Wire);

const char *ssid = "your ssid";
const char *password = "your password";
int counter = 0;
bool api_fetch_once = true;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
HTTPClient client;

const uint8_t lcdColumns = 20;
const uint8_t lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

void pinToCore()
{
  xTaskCreatePinnedToCore(audioLoop, "Task1", 10000, NULL, 1, &Task1, 0);
}

void audioLoop(void *pvParameters)
{
  vTaskDelay(10);
  for (;;)
  {
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;
    audio.loop(); // audio.loop need to run alone in loop function, otherwise audio library
                  // may not function properly.
  }
}

void timeIsNow(bool azanNow, int ndx)
{
  char waktuSolat[9];
  switch (ndx)
  {
  case 1:
    strncpy(waktuSolat, "Subuh", sizeof(waktuSolat));
    break;
  case 2:
    strncpy(waktuSolat, "Zohor", sizeof(waktuSolat));
    break;
  case 3:
    strncpy(waktuSolat, "Asar", sizeof(waktuSolat));
    break;
  case 4:
    strncpy(waktuSolat, "Maghrib", sizeof(waktuSolat));
    break;
  case 5:
    strncpy(waktuSolat, "Isyak", sizeof(waktuSolat));
    break;
  }

  if (azanNow)
  {
    Serial.println("azanNow ");
    lcd.clear();
    lcd.setCursor(2,2);
    lcd.print("Waktu Solat ");
    lcd.print(waktuSolat);
    audio.connecttoFS(SD, "/your_azan_file.wav");
  }
  azanNow = false;
}

void isItSolatTime(int prayerTime[], RtcDateTime &dt)
{
  
  int solatIndex = prayerTime[2];
    
   if (dt.Hour() > 12)
  {
    prayerTime[0] += 12;
  }
	
 for (byte i = 0; i < (sizeof(prayerTime)/sizeof(prayerTime[0])); i++)
  {
    if ((prayerTime[0] == dt.Hour() && (prayerTime[1] == dt.Minute())))
    {
      Serial.println("Solat time"); //for debugging
      timeIsNow(true, solatIndex);
    }
    else
    {
      Serial.println("Not solat time yet");
    }
  }
 }
	
//Breaking up character strings and convert to int of hours and minutes
void reformatPrayerTime(char *prayerTime[], int arrLength)
{

  for (int i = 0; i < arrLength; i++)
  {
    int ndx = i; // 0=imsak,1=subuh,2=zohor,3=asar,4=maghrib,5=isyak
    char *arr = prayerTime[i];

    byte length = strlen(arr);

    if (length)
    {
      arr[length - 2] = '\0';
    }
    
    char *ptr = NULL;
    char *newCharStr[4];
    byte index = 0;

    ptr = strtok(arr, ":"); 

    while (ptr != NULL) 
    {
      newCharStr[index] = ptr;
      index++;
      ptr = strtok(NULL, " ");
    }
    
    char *HrChar = newCharStr[0];
    char *MinChar = newCharStr[1];
    //Convert them to int and assemble into array
    int Hr_int = atoi(HrChar);
    int Min_int = atoi(MinChar);
    
    int HrMin_int[]={Hr_int,Min_int,ndx};
    RtcDateTime timeNow = Rtc.GetDateTime();
   
    isItSolatTime(HrMin_int, timeNow);
  }
}

void printDateTime(const RtcDateTime &dt)
{
  char datestring[11];
  char timestring[10];

  snprintf_P(datestring, sizeof(datestring), PSTR("%02u/%02u/%04u"), dt.Day(), dt.Month(), dt.Year());
  snprintf_P(timestring, sizeof(timestring), PSTR("%02u:%02u:%02u"), dt.Hour(), dt.Minute(), dt.Second());

  lcd.setCursor(0, 0); 
  lcd.print(datestring);
  lcd.setCursor(11, 0);
  lcd.print(timestring);
}

void RTC_Update()
{
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime() - 946684800UL;
  Rtc.SetDateTime(epochTime);
}

void setup_rtc()
{
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid())
  {
    if (Rtc.LastError() != 0)
    {
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else
    {
      Serial.println("RTC lost confidence in the DateTime!");

      Rtc.SetDateTime(compiled);
    }
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
}
void audio_SD_setup()
{
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  if (!SD.begin(SD_CS))
  {
    Serial.println("Error talking to SD card!");
    while (true)
      ; // end program
  }
  Serial.println("SD OK");
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(20); // 0...21
}
void process_api_data()
{

  String payload = client.getString();
  payload.replace(" ", "");

  //Serial.println(payload);

  StaticJsonDocument<550> doc;

  DeserializationError err = deserializeJson(doc, payload);
  if (err)
  {
    Serial.print("ERROR in deserialization JSON");
    Serial.println(err.c_str());
    return;
  }

  //const  char*  date = doc["prayer_times"]["date"]; /
  const char *imsak = doc["prayer_times"]["imsak"];
  const char *subuh = doc["prayer_times"]["subuh"];
  //const  char*  syuruk = doc["prayer_times"]["syuruk"]; // not enough display space
  const char *zohor = doc["prayer_times"]["zohor"];
  const char *asar = doc["prayer_times"]["asar"];
  const char *maghrib = doc["prayer_times"]["maghrib"];
  const char *isyak = doc["prayer_times"]["isyak"];

  //char date_t[10];
  char imsak_t[8];
  char subuh_t[8];
  char zohor_t[8];
  char asar_t[8];
  char maghrib_t[8];
  char isyak_t[8];

  //strcpy(date_t,date);
  strcpy(imsak_t, imsak);
  strcpy(subuh_t, subuh);
  strcpy(zohor_t, zohor);
  strcpy(asar_t, asar);
  strcpy(maghrib_t, maghrib);
  strcpy(isyak_t, isyak);

  lcd.setCursor(0, 1);
  lcd.print("I "); //Imsak
  lcd.setCursor(2, 1);
  lcd.print(imsak_t);
  lcd.setCursor(10, 1);
  lcd.print("S "); //Subuh
  lcd.setCursor(12, 1);
  lcd.print(subuh_t);
  lcd.setCursor(0, 2);
  lcd.print("Z "); //Zohor
  lcd.setCursor(2, 2);
  lcd.print(zohor_t);
  lcd.setCursor(10, 2);
  lcd.print("A "); //Asar
  lcd.setCursor(12, 2);
  lcd.print(asar_t);
  lcd.setCursor(0, 3);
  lcd.print("M "); //Maghrib
  lcd.setCursor(2, 3);
  lcd.print(maghrib_t);
  lcd.setCursor(10, 3);
  lcd.print("I "); //Isyak
  lcd.setCursor(12, 3);
  lcd.print(isyak_t);

  char *prayertimeArr[] = {imsak_t, subuh_t, zohor_t, asar_t, maghrib_t, isyak_t};

  reformatPrayerTime(prayertimeArr, 6);
}

void fetch_It_Now()
{
  // get your area code and time format at api.azanpro.com
  client.begin("https://api.azanpro.com/times/today.json?zone=trg01&format=12-hour");
  int httpCode = client.GET();
  if (httpCode > 0)
  {
    process_api_data();
  }
  else
  {
    Serial.println("Error on HTTP request");
  }
}
// Very frequent API fetches cause HTTPClient to crash, so I decided to fetch only twice in 24 hours.
void get_api_data()
{

  if (api_fetch_once) //get the first, once, API fetch
  {
    fetch_It_Now();
    api_fetch_once = false;
  }
  // then fetch at 1:00 am and 12.15 pm
  RtcDateTime timeNow = Rtc.GetDateTime();
  if (((timeNow.Hour() == 1) && (timeNow.Minute() == 0) && (timeNow.Second() == 0)) || ((timeNow.Hour() == 12) && (timeNow.Minute() == 15) && (timeNow.Second() == 0)))
  {
    fetch_It_Now();
  }
}

void setup()
{
  Serial.begin(115200);
  pinToCore();
  setup_rtc();
  WiFi.disconnect(true);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter >= 120)
    { //after 120 seconds timeout - reset board
      Serial.println("Restart the board");
      ESP.restart();
    }
  }
  counter = 0;
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  timeClient.setTimeOffset(28800);
  timeClient.update();
  RTC_Update();
  //start_lcd();
  lcd.init();
  lcd.backlight();

  audio_SD_setup();
}

void loop()
{
  RtcDateTime rtctime = Rtc.GetDateTime();
  printDateTime(rtctime);
  if ((WiFi.status() != WL_CONNECTED))
  {
    // counter = 0; //reset counter
    WiFi.begin(ssid, password);
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter >= 120) // about 120 seconds, then restart
    {
      ESP.restart();
    }
  }
  counter = 0;
  if ((WiFi.status() == WL_CONNECTED))
  {
    get_api_data();
  }
  client.end();
  delay(2000);
}
