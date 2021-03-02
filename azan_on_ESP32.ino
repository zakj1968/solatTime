#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <RtcDS3231.h>
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

TaskHandle_t Task1;
Audio audio;
RtcDS3231<TwoWire> Rtc(Wire);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
HTTPClient client;
const char *ssid = "your-wifi-ssid";
const char *password = "your-wifi-password";
const uint8_t lcdColumns = 20;
const uint8_t lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
volatile int intCounter;
bool fetchOnce;
int counter = 0; // For restarting ESP

struct PrIntData1D
{
  int imHr, imMin, imNdx, suHr, suMin, suNdx, zoHr, zoMin, zoNdx, asHr, asMin, asNdx, maHr, maMin, maNdx, isHr, isMin, isNdx;
};
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
    audio.loop();
  }
}
bool compareSolatTime(int prayerTime[][3], RtcDateTime &time)
{	
int *p;
for (p=&prayerTime[0][0]; p<= &prayerTime[5][2]; p++)
{ 
	if (time.Hour() >12)
	{
		*(p+6) += 12;
		*(p+9) += 12;
		*(p+12) += 12;
		*(p+15) += 12;
	}
	 if (*(p+3)-1 == time.Hour()){ // 1 hr before subuh
		return true;
	 }else if (*(p+3) == time.Hour() && *(p+4) == time.Minute()){//Subuh
		 return true;	
	 }else if (*(p+6) == time.Hour() && *(p+7) == time.Minute()){//Zohor
		 return true;		
	 }else if (*(p+9) == time.Hour() && *(p+10) == time.Minute()){//Asar
		 return true;	
	 }else if (*(p+12) == time.Hour() && *(p+13) == time.Minute()){ //Maghrib
		 return true;	
	 }else if (*(p+15) == time.Hour() && *(p+16) == time.Minute()){ //isyak
		 return true;	
	 }else{
		 return false;
	 }	
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
    Rtc.SetDateTime(compiled);
  }
  if (!Rtc.GetIsRunning())
  {
    Rtc.SetIsRunning(true);
  }
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Rtc.SetDateTime(compiled);
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
  audio.setVolume(21); // 0...21
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
  int prArr[6][3];
  PrIntData1D prData1;
  for (i = 0; i < 6; i++)
  {
    char *arr = prTimeArr[i];
    byte length = strlen(arr);
    int ndx = i;

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

    int HrMinArr[] = {Hr_int, Min_int, ndx};

    for (int j = 0; j < 3; j++)
    {
      prArr[i][j] = HrMinArr[j];
    }
  }
  prData1.imHr = prArr[0][0];
  prData1.imMin = prArr[0][1];
  prData1.imNdx = prArr[0][2];
  prData1.suHr = prArr[1][0];
  prData1.suMin = prArr[1][1];
  prData1.suNdx = prArr[1][2];
  prData1.zoHr = prArr[2][0];
  prData1.zoMin = prArr[2][1];
  prData1.zoNdx = prArr[2][2];
  prData1.asHr = prArr[3][0];
  prData1.asMin = prArr[3][1];
  prData1.asNdx = prArr[3][2];
  prData1.maHr = prArr[4][0];
  prData1.maMin = prArr[4][1];
  prData1.maNdx = prArr[4][2];
  prData1.isHr = prArr[5][0];
  prData1.isMin = prArr[5][1];
  prData1.isNdx = prArr[5][2];

  return prData1;
}
void dataPool(PrIntData1D *ptr)
{
  int iHr = ptr->imHr;
  int iMin = ptr->imMin;
  int iNdx = ptr->imNdx;
  int sHr = ptr->suHr;
  int sMin = ptr->suMin;
  int sNdx = ptr->suNdx;
  int zHr = ptr->zoHr;
  int zMin = ptr->zoMin;
  int zNdx = ptr->zoNdx;
  int aHr = ptr->asHr;
  int aMin = ptr->asMin;
  int aNdx = ptr->asNdx;
  int mHr = ptr->maHr;
  int mMin = ptr->maMin;
  int mNdx = ptr->maNdx;
  int isyHr = ptr->isHr;
  int isyMin = ptr->isMin;
  int isyNdx = ptr->isNdx;
  RtcDateTime timenow = Rtc.GetDateTime();
  int prTime[6][3] = {
      {iHr, iMin, iNdx},
      {sHr, sMin, sNdx},
      {zHr, zMin, zNdx},
      {aHr, aMin, aNdx},
      {mHr, mMin, mNdx},
      {isyHr, isyMin, isyNdx}};
 if (compareSolatTime(prTime, timenow))
 {
    audio.connecttoFS(SD, "/your-azan-file.wav");//Note: Make sure filename is short (8 characters or less, eg. azan.wav)
    }else{
     Serial.println("Not Solat Time yet");
 }
}
String getApiData(bool fetchOnce)
{
  if (fetchOnce)
  {
    fetchOnce = false;
    client.begin("https://api.azanpro.com/times/today.json?zone=trg01&format=12-hour");
    int httpCode = client.GET();
    if (httpCode > 0)
    {
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
    {
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
  fetchOnce = true;
}
void loop()
{
  RtcDateTime rtctime = Rtc.GetDateTime();
  printDateTime(rtctime);
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
    if (fetchOnce)
    {
      fetchOnce = false;
      payloadStr = getApiData(true); //fetchOnce is true
      prData2 = processApiData(payloadStr);
      dataPool(&prData2);
    }
    else
    {
      dataPool(&prData2);
    }
  }
  client.end();
  delay(2000);
}
