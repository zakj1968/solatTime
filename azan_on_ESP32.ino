#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <RtcDS3231.h>
//////////
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <AsyncTCP.h>
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

const uint8_t lcdColumns = 20;
const uint8_t lcdRows = 4;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
bool fetchOnce;
int counter = 0; //wifi conn countdown

AsyncWebServer webserver(80);
WebSocketsServer webSocket = WebSocketsServer(81);
const char *filename = "/config.txt";
char apiCode[10];

struct PrData
{
  int imHr, imMin,suHr, suMin, zoHr, zoMin,asHr, asMin, maHr, maMin,isHr, isMin;
};
bool wifi_connection(const char *ssid,const char *pw)
{
 
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid,pw);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    counter++;
    if (counter >= 15)
    {
      return false;
    }
  }
  counter = 0;
  return true;
}

void wifi_AP_mode()
{
   WiFi.disconnect(true);       
   WiFi.softAP("ESP32AP", "");
   Serial.println(WiFi.softAPIP());	  
   webserver.begin();
   webserver.on("/", HTTP_GET, onIndexRequest);
   webserver.onNotFound(onPageNotFound); 
   webSocket.begin();
   webSocket.onEvent(onWebSocketEvent); 	  
}

void loadConfigData(const char *filename) {
  File file = SPIFFS.open(filename);
  if (!file) {
    return;
  }
  StaticJsonDocument<150>doc;
  DeserializationError err = deserializeJson(doc,file);
  if (err)
  {
	Serial.println("Deserialization Error");
	wifi_AP_mode();
  } 
  const char *ssidDat = doc["ssid"];
  const char *pwDat = doc["pw"];
  const char *codeDat = doc["zone"]; 
  if (codeDat)
  {
	  strlcpy(apiCode,codeDat,sizeof(apiCode));
	  
  }else{
	  strcpy(apiCode,"N/A");
  }
  file.close();
  if (wifi_connection(ssidDat,pwDat))
  {
	  Serial.println("WiFi STA mode started!");
  }else{
	  wifi_AP_mode();
  }   
}

void onWebSocketEvent(uint8_t num, WStype_t type,uint8_t * payload, size_t length) {
  switch (type) 
  {
    case WStype_TEXT:
    {
      char payloadData[80];
      strcpy(payloadData,(char*)payload);
      SPIFFS.remove(filename);
      File file = SPIFFS.open(filename,FILE_WRITE);
      if (!file)
      {
		return;
	  }
	 file.print(payloadData);
	 int bytesWritten = file.print(payloadData);
	 if (bytesWritten >0)
	 {
		const char *msg = "Data save success: ";
		char srvMsg[110];
		snprintf(srvMsg,sizeof(srvMsg),"%s%s",msg,payloadData);
		webSocket.sendTXT(num,srvMsg);
	 }else{
		webSocket.sendTXT(num,"Fail save data");//Fail save
	 }
	 file.close();  
	}
	 break;
    case WStype_PING:
	{
	Serial.println("Get ping");
	}
	break;
    case WStype_PONG:
    {
	Serial.println("Get pong");
     }
	break;
	case WStype_CONNECTED:
      {
        webSocket.sendTXT(num,"Connected to ESP32");
      }
     break;
  }
}
void onIndexRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.html", "text/html");
}
void onPageNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
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
    audio.loop();
  }
}
int compareSolatTime(int prayerTime[][2], RtcDateTime &time)
{	
int *p;
bool pm ;
for (p=&prayerTime[0][0]; p<= &prayerTime[5][1]; p++)
{ 
	if (time.Hour() >12)
	{
		*(p+4) += 12;
		*(p+6) += 12;
		*(p+8) += 12;
		*(p+10) += 12;
		pm = true;
	}else{
		pm = false;
	}
	 return (*(p + 2) == time.Hour() && *(p + 3) == time.Minute() && pm == false)?1:
            (*(p + 4) == time.Hour() && *(p + 5) == time.Minute() && pm == true)?2:
            (*(p + 6) == time.Hour() && *(p + 7) == time.Minute() && pm == true)?3:
            (*(p + 8) == time.Hour() && *(p + 9) == time.Minute() && pm == true)?4: 
          (*(p + 10) == time.Hour() && *(p + 11) == time.Minute() && pm == true)?5:0;
	}
}
void printDateTime(const RtcDateTime &dt) 
{
  char timestring[10];
  snprintf_P(timestring, sizeof(timestring), PSTR("%02u:%02u:%02u"), dt.Hour(), dt.Minute(), dt.Second());
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
 
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);

  if (!Rtc.IsDateTimeValid())
  {
    Rtc.SetDateTime(compiled);
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
PrData processApiData(String payload)
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
  
  const char *date = doc["prayer_times"]["date"]; 
  const char *imsak = doc["prayer_times"]["imsak"];
  const char *subuh = doc["prayer_times"]["subuh"];
  //const  char*  syuruk = doc["prayer_times"]["syuruk"]; // not enough display space
  const char *zohor = doc["prayer_times"]["zohor"];
  const char *asar = doc["prayer_times"]["asar"];
  const char *maghrib = doc["prayer_times"]["maghrib"];
  const char *isyak = doc["prayer_times"]["isyak"];
  lcd.setCursor(0,0);lcd.print(date);
  lcd.setCursor(0, 1);lcd.print("I "); //Imsak
  lcd.setCursor(2, 1);lcd.print(imsak);
  lcd.setCursor(10, 1);lcd.print("S "); //Subuh
  lcd.setCursor(12, 1);lcd.print(subuh);
  lcd.setCursor(0, 2);lcd.print("Z "); //Zohor
  lcd.setCursor(2, 2);lcd.print(zohor);
  lcd.setCursor(10, 2);lcd.print("A "); //Asar
  lcd.setCursor(12, 2);lcd.print(asar);
  lcd.setCursor(0, 3);lcd.print("M "); //Maghrib
  lcd.setCursor(2, 3);lcd.print(maghrib);
  lcd.setCursor(10, 3);lcd.print("I "); //Isyak
  lcd.setCursor(12, 3);lcd.print(isyak);
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
  int HrMinArr[2];
  int prArr[6][2];
  PrData prData1;
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

    int HrMinArr[] = {Hr_int, Min_int};

    for (int j = 0; j < 2; j++)
    {
      prArr[i][j] = HrMinArr[j];
    }
  }
  prData1.imHr = prArr[0][0];prData1.imMin = prArr[0][1];
  prData1.suHr = prArr[1][0];prData1.suMin = prArr[1][1];
  prData1.zoHr = prArr[2][0];prData1.zoMin = prArr[2][1];
  prData1.asHr = prArr[3][0];prData1.asMin = prArr[3][1];
  prData1.maHr = prArr[4][0];prData1.maMin = prArr[4][1];
  prData1.isHr = prArr[5][0];prData1.isMin = prArr[5][1];
  return prData1;
}
void dataPool(PrData *ptr)
{
  int iHr = ptr->imHr;int iMin = ptr->imMin;
  int sHr = ptr->suHr;int sMin = ptr->suMin;
  int zHr = ptr->zoHr;int zMin = ptr->zoMin;
  int aHr = ptr->asHr;int aMin = ptr->asMin;
  int mHr = ptr->maHr;int mMin = ptr->maMin;
  int isyHr = ptr->isHr;int isyMin = ptr->isMin;
  RtcDateTime timenow = Rtc.GetDateTime();
  int prTime[6][2] = {
      {iHr, iMin},{sHr, sMin},{zHr, zMin},{aHr, aMin},{mHr, mMin},{isyHr, isyMin}};
int sCode = compareSolatTime(prTime, timenow);
sCode == 0 ? Serial.println("Not Solat Time yet") : sCode == 1 ? audio.connecttoFS(SD, "/azSubuh.wav")
                                                                 : audio.connecttoFS(SD, "/azan.wav");
}
String getApiData(bool fetchOnce)
{
  const char *url1 ="https://api.azanpro.com/times/today.json?zone=";
  const char *url2 = apiCode;
  const char *url3 ="&format=12-hour";

  char apiURL[80];
  snprintf(apiURL, sizeof(apiURL),"%s%s%s",url1,url2,url3);
  if (fetchOnce)
  {
    fetchOnce = false;
    client.begin(apiURL);
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
}
void setup()
{
  Serial.begin(115200);
  pinToCore();
  setup_rtc();
  if( !SPIFFS.begin()){
    while(1);
  }
  loadConfigData(filename);  
	
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
 
  if ((WiFi.status() == WL_CONNECTED))
  {
    String payloadStr;
    PrData prData2;
	if ((rtctime.Hour() == 1 && rtctime.Minute() == 0) || (rtctime.Hour() == 12 && rtctime.Minute() == 0))
	{
	  fetchOnce = true;    
	} 
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
  webSocket.loop();
  delay(2000);
}
