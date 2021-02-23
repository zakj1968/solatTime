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
////////////////////////////////////

// SD card pins
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
//I2S pins
#define I2S_DOUT 25
#define I2S_BCLK 27
#define I2S_LRC 26

TaskHandle_t Task1; // Core 0 task
Audio audio;
RtcDS3231<TwoWire> Rtc(Wire);

const char *ssid = "your-wifi-ssid";
const char *password = "your-wifi-password";
int counter = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
HTTPClient client;

const uint8_t lcdColumns = 20;
const uint8_t lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

volatile int intCounter;
bool fetchOnce;
bool solatNow = false;

static bool timerTick;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

struct PrIntData1D
{
  int prHr;
  int prMinute;
  int prNdx;
};

void IRAM_ATTR onTime()
{
  portENTER_CRITICAL_ISR(&timerMux);
  intCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

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
void showMessage(char message[])
{
  char waktuSolat[9];
  strncpy(waktuSolat, message, sizeof(message));
  lcd.clear();
  lcd.setCursor(2, 2);
  lcd.print("Waktu Solat ");
  lcd.print(waktuSolat);
}
void azanNow(int solatIndex)
{
  char message[9];
  switch (solatIndex)
  {
  case 0:
  {
    audio.connecttoFS(SD, "/your_azan_file.wav");
  }
  case 1:
  {
    showMessage("Subuh");
    audio.connecttoFS(SD, "/your_azan_file.wav");
  }
  break;
  case 2:
  {
    showMessage("Zohor");
    audio.connecttoFS(SD, "/your_azan_file.wav");
  }
  break;
  case 3:
  {
    showMessage("Asar");
    audio.connecttoFS(SD, "/your_azan_file.wav");
  }
  break;
  case 4:
  {
    showMessage("Maghrib");
    audio.connecttoFS(SD, "/your_azan_file.wav");
  }
  break;
  case 5:
  {
    showMessage("Isyak");
    audio.connecttoFS(SD, "/your_azan_file.wav");
  }
  break;
  default:
    Serial.println("Invalid index");
    break;
  }
}

void compareSolatTime(int prayerTime[][3], RtcDateTime &time)
{

  int subuhIndex = prayerTime[1][2];
  int zohorIndex = prayerTime[2][2];
  int asarIndex = prayerTime[3][2];
  int maghribIndex = prayerTime[4][2];
  int isyakIndex = prayerTime[5][2];

  if (prayerTime[1][0] - 1 == time.Hour())
  {
    azanNow(0); // Azan 1 hour before subuh
  }

  if (subuhIndex == 1)
  {
    if (((prayerTime[1][0] == time.Hour()) && (prayerTime[1][1] == time.Minute())))
    {
      azanNow(1);
    }
  }
  else if (zohorIndex == 2)
  {
    if (time.Hour() > 12)
    {
      prayerTime[2][0] += 12;
    }
    if ((prayerTime[2][0] == time.Hour()) && (prayerTime[2][1] == time.Minute()))
    {
      azanNow(2);
    }
  }
  else if (asarIndex == 3)
  {
    prayerTime[3][0] += 12;
    if ((prayerTime[3][0] == time.Hour()) && (prayerTime[3][1] == time.Minute()))
    {
      azanNow(3);
    }
  }
  else if (maghribIndex == 4)
  {
    prayerTime[4][0] += 12;
    if ((prayerTime[4][0] == time.Hour()) && (prayerTime[4][1] == time.Minute()))
    {
      azanNow(4);
    }
  }
  else if (isyakIndex == 5)
  {
    prayerTime[5][0] += 12;
    if ((prayerTime[5][0] == time.Hour()) && (prayerTime[5][1] == time.Minute()))
    {
      azanNow(5);
    }
  }
  else
  {
    Serial.println("Invalid Solat Index");
  }
  Serial.println("Not a prayer time");
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

PrIntData1D processApiData(String payload)
{

  payload.replace(" ", "");

  StaticJsonDocument<550> doc;

  DeserializationError err = deserializeJson(doc, payload);
  if (err)
  {
    Serial.print("ERROR in deserialization JSON");
    Serial.println(err.c_str());
    //return;
  }
  //const  char*  date = doc["prayer_times"]["date"]; /
  const char *imsak = doc["prayer_times"]["imsak"];
  const char *subuh = doc["prayer_times"]["subuh"];
  //const  char*  syuruk = doc["prayer_times"]["syuruk"]; // not enough display space
  const char *zohor = doc["prayer_times"]["zohor"];
  const char *asar = doc["prayer_times"]["asar"];
  const char *maghrib = doc["prayer_times"]["maghrib"];
  const char *isyak = doc["prayer_times"]["isyak"];

  lcd.setCursor(0, 1);
  lcd.print("I "); //Imsak
  lcd.setCursor(2, 1);
  lcd.print(imsak);
  lcd.setCursor(10, 1);
  lcd.print("S "); //Subuh
  lcd.setCursor(12, 1);
  lcd.print(subuh);
  lcd.setCursor(0, 2);
  lcd.print("Z "); //Zohor
  lcd.setCursor(2, 2);
  lcd.print(zohor);
  lcd.setCursor(10, 2);
  lcd.print("A "); //Asar
  lcd.setCursor(12, 2);
  lcd.print(asar);
  lcd.setCursor(0, 3);
  lcd.print("M "); //Maghrib
  lcd.setCursor(2, 3);
  lcd.print(maghrib);
  lcd.setCursor(10, 3);
  lcd.print("I "); //Isyak
  lcd.setCursor(12, 3);
  lcd.print(isyak);

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

  char *prTimeArr[] = {imsak_t, subuh_t, zohor_t, asar_t, maghrib_t, isyak_t};
  int i, j;
  int HrMinArr[3];

  for (i = 0; i < 6; i++)
  {
    char *arr = prTimeArr[i];
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
    int Hr_int = atoi(HrChar);
    int Min_int = atoi(MinChar);
    int ndx = i; // 0=imsak,1=subuh,2=zohor,3=asar,4=maghrib,5=isyak

    int HrMinArr[] = {Hr_int, Min_int, ndx};
    PrIntData1D prData1;
    prData1.prHr = HrMinArr[0];
    prData1.prMinute = HrMinArr[1];
    prData1.prNdx = HrMinArr[2];

    return prData1;
  }
}

void dataPool(PrIntData1D *ptr)
{
  int Hr = ptr->prHr;
  int Minute = ptr->prMinute;
  int ndx = ptr->prNdx;

  RtcDateTime timenow = Rtc.GetDateTime();

  //Building 2D array of solat data
  int imHr, imMin, imNdx, suHr, suMin, suNdx, zoHr, zoMin, zoNdx, asHr, asMin, asNdx, maHr, maMin, maNdx, isHr, isMin, isNdx;
  switch (ndx)
  {
  case 0:
  {
    imHr = Hr;
    imMin = Minute;
    imNdx = ndx;
  }
  break;
  case 1:
  {
    suHr = Hr;
    suMin = Minute;
    suNdx = ndx;
  }
  break;
  case 2:
  {
    zoHr = Hr;
    zoMin = Minute;
    zoNdx = ndx;
  }
  break;
  case 3:
  {
    asHr = Hr;
    asMin = Minute;
    asNdx = ndx;
  }
  break;
  case 4:
  {
    maHr = Hr;
    maMin = Minute;
    maNdx = ndx;
  }
  break;
  case 5:
  {
    isHr = Hr;
    isMin = Minute;
    isNdx = ndx;
  }
  break;
  default:
    Serial.print("Invalid prayer index");
    break;
  }

  int prTime[6][3] = {
      {imHr, imMin, imNdx},
      {suHr, suMin, suNdx},
      {zoHr, zoMin, zoNdx},
      {asHr, asMin, asNdx},
      {maHr, maMin, maNdx},
      {isHr, isMin, isNdx}};
  timerTick = true;
  if (timerTick)
  {
    compareSolatTime(prTime, timenow); //comparing prayer time with rtc time every 3 seconds
    timerTick = !timerTick;
  }
}
String getApiData(bool fetchOnce)
{
  if (fetchOnce) //fetch when rebooting or else fetch at specified time. set to true too if require complete sequence of data processing
  {
    fetchOnce = false;
    client.begin("https://api.azanpro.com/times/today.json?zone=trg01&format=12-hour");
    int httpCode = client.GET();
    if (httpCode > 0)
    {
      Serial.println("httpCode: ");
      Serial.print(httpCode);
      Serial.println();
      String payload = client.getString();
      payload.replace(" ", "");
      return payload;
    }
    else
    {
      Serial.println("Error on HTTP request");
    }

    fetchOnce = false;
  }

  RtcDateTime timeNow = Rtc.GetDateTime();
  if (((timeNow.Hour() == 1) && (timeNow.Minute() == 0) && (timeNow.Second() == 0)) || ((timeNow.Hour() == 12) && (timeNow.Minute() == 15) && (timeNow.Second() == 0)))
  {
    client.begin("https://api.azanpro.com/times/today.json?zone=trg01&format=12-hour");
    int httpCode = client.GET();
    if (httpCode > 0)
    {
      fetchOnce = true; // This will enable update sequence of data processing.
      String payload = client.getString();
      payload.replace(" ", "");
      return payload;
    }
    else
    {
      Serial.println("Error on HTTP request");
    }
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

  lcd.init();
  lcd.backlight();
  audio_SD_setup();

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTime, true);
  timerAlarmWrite(timer, 3000000, true); //Every 3 seconds
  timerAlarmEnable(timer);
  timerTick = false;
  fetchOnce = true;
}

void loop()
{
  RtcDateTime rtctime = Rtc.GetDateTime();
  printDateTime(rtctime);
  if (intCounter > 0)
  {
    portENTER_CRITICAL(&timerMux);
    intCounter--;
    portEXIT_CRITICAL(&timerMux);
    timerTick = true;
  }

  if ((WiFi.status() != WL_CONNECTED))
  {

    WiFi.begin(ssid, password);
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter >= 120)
    {
      ESP.restart();
    }
  }
  counter = 0;
  if ((WiFi.status() == WL_CONNECTED))
  {
    String payloadStr;
    PrIntData1D prData2;
    //Need to do this to avoid system crash if new data fetching from API exceed the limit
    if (fetchOnce)
    {
      fetchOnce = false;
      payloadStr = getApiData(true); //fetchOnce is true
      processApiData(payloadStr);
      prData2 = processApiData(payloadStr); //struct type PrIntData1D contain int of hr,min,ndx
      dataPool(&prData2);
    }
    else
    { // else, id there is no new data, take the existing data and keep pushing them out. Otherwise we may miss azan at solat time.
      dataPool(&prData2);
    }
  }
  client.end();
  delay(2000);
}
