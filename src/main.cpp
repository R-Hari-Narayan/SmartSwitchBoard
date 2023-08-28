
#include <Arduino.h>
#include <PNGdec.h>
//#include "itadori.h" // Image is stored here in an 8 bit array
#include <WiFi.h>
#include "SPI.h"
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "time.h"
#include "sntp.h"
#include <string.h>
#include <ESP32Time.h>
#include "Clear_sky.h"
#include "Few_clouds.h"
#include "Scattered_clouds.h"
#include "Broken_cloud.h"
#include "Shower_rain.h"
#include "Rain.h"
#include "Thunderstorm.h"
#include "Snow.h"
#include "Haze.h"
#include "Fog.h"
#include "iconError.h"

WiFiClient client;
HTTPClient http;

const char* ssid = "YourSSID";
const char* password = "YourPassword";

String openWeatherAPI= "YourOpenWeatherAPIkey";

String JSON_buffer;

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 5*3600+1800;
const int   daylightOffset_sec = 0;
//ESP32Time rtc;
ESP32Time rtc(0);  // offset in seconds GMT+1

const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

void pngDraw(PNGDRAW *pDraw);
PNG png; // PNG decoder inatance

#define MAX_IMAGE_WDITH 320 // Adjust for your images

int16_t xpos = 240;
int16_t ypos = 10;

TFT_eSPI tft = TFT_eSPI();         // Invoke custom library

//====================================================================================
//                                  Days function
//====================================================================================
String day(int a){
  String Day;
  switch (a){
    case 0: Day= "Sunday";
            break;
    case 1: Day= "Monday";
            break;
    case 2: Day= "Tuesday";
            break;
    case 3: Day= "Wednesday";
            break;
    case 4: Day= "Thursday";
            break;
    case 5: Day= "Friday";
            break;
    case 6: Day= "Saturday";
            break;
  }
  return Day;
}

//====================================================================================
//                                  Months function
//====================================================================================
//Function to select current month name from month number provided by ntp server
String month(int x){
  String Month;
  switch (x){
    case 0: Month= "January";
            break;
    case 1: Month= "February";
            break;
    case 2: Month= "March";
            break;
    case 3: Month= "April";
            break;
    case 4: Month= "May";
            break;
    case 5: Month= "June";
            break;
    case 6: Month= "July";
            break;
    case 7: Month= "August";
            break;
    case 8: Month= "September";
            break;
    case 9: Month= "October";
            break;
    case 10: Month= "November";
            break;
    case 11: Month= "December";
            break;
  }
  return Month;
}
//====================================================================================
//                                  Scan For WiFi
//====================================================================================
void ScanForWiFi(){
  WiFi.mode(WIFI_STA);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,2);
  tft.setTextColor(TFT_WHITE);
  Serial.println(WiFi.status());
  tft.println("WiFi Status: " + String(WiFi.status()));

  if (WiFi.status() == WL_CONNECTED){
    WiFi.disconnect();
  }

  Serial.println("Scanning for available networks");
  tft.println("Scanning for available networks");
  int n = WiFi.scanNetworks();

  if (n==0){
    Serial.println("No networks found");
    tft.println("No networks found.");
  }
  else{
    Serial.println("Available WiFi networks");
    tft.println("Available WiFi networks");
    for (int i=0; i<n; i++){
      Serial.print(i+1);
      tft.print(i+1);
      Serial.print(": ");
      tft.print(": ");
      Serial.print(WiFi.SSID(i));
      tft.print(WiFi.SSID(i));
      Serial.print(" <");
      tft.print(" <");
      Serial.print(WiFi.RSSI());
      tft.print(WiFi.RSSI());
      Serial.print("> ");
      tft.print("> ");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      tft.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
    }
  }
  
}

//====================================================================================
//                              Connect to WiFi
//====================================================================================
void ConnectToWiFi(){
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,2);
  tft.setTextColor(TFT_WHITE);
  WiFi.begin(ssid,password);
  Serial.print("Connecting to WiFi.");
  tft.print("Connecting to WiFi.");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    tft.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to WiFi");
  tft.println("\nConnected to WiFi");
  Serial.print("Signal strenth: ");
  Serial.println(WiFi.RSSI());
  tft.print("Signal strength: ");
  tft.println(WiFi.RSSI());
}
//====================================================================================
//                              Display Weather Icon
//====================================================================================
void printIcon(String icon){
  Serial.println(icon);
  int16_t rc;
  tft.fillRect(240,10,50,50,TFT_BLACK);
  if (icon=="\"01d\"" || icon== "\"01n\""){
    rc = png.openFLASH((uint8_t *)Clear_sky, sizeof(Clear_sky), pngDraw);
  }
  else if (icon=="\"02d\"" || icon== "\"02n\""){
    rc = png.openFLASH((uint8_t *)Few_clouds, sizeof(Few_clouds), pngDraw);
  }
  else if (icon=="\"03d\"" || icon== "\"03n\""){
    rc = png.openFLASH((uint8_t *)Scattered_clouds, sizeof(Scattered_clouds), pngDraw);
  }
  else if (icon=="\"04d\"" || icon== "\"04n\""){
    rc = png.openFLASH((uint8_t *)Broken_cloud, sizeof(Broken_cloud), pngDraw);
  }
  else  if (icon=="\"09d\"" || icon== "\"09n\""){
    rc = png.openFLASH((uint8_t *)Shower_rain, sizeof(Shower_rain), pngDraw);
  }
  else if (icon=="\"10d\"" || icon== "\"10n\""){
    rc = png.openFLASH((uint8_t *)Rain, sizeof(Rain), pngDraw);
  }
  else if (icon=="\"11d\"" || icon== "\"11n\""){
    rc = png.openFLASH((uint8_t *)Thunderstorm, sizeof(Thunderstorm), pngDraw);
  }
  else if (icon=="\"13d\"" || icon== "\"13n\""){
    rc = png.openFLASH((uint8_t *)Snow, sizeof(Snow), pngDraw);
  }
  else if (icon=="\"50d\""){
    Serial.println("Haze");
    rc = png.openFLASH((uint8_t *)Haze, sizeof(Haze), pngDraw);
  }
  else if (icon=="\"50n\""){
    Serial.println("Fog");
    rc = png.openFLASH((uint8_t *)Fog, sizeof(Fog), pngDraw);
  }
  else{
    Serial.println("unexpected icon code");
    rc = png.openFLASH((uint8_t *)iconError, sizeof(iconError), pngDraw);
  }
  if (rc == PNG_SUCCESS) {
    tft.startWrite();
    rc = png.decode(NULL, 0);
    tft.endWrite();
  }
}


//====================================================================================
//                                 Display time & date
//====================================================================================
void printLocalTime(void * parameters)
{
  for(;;){
    Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
    if (rtc.getHour()==0 && rtc.getMinute()==0 && rtc.getSecond()==0){tft.fillRect(10,130,230,110,TFT_BLACK);}
    //Print DATE
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    String Month= month(rtc.getMonth());
    String Date = String(rtc.getDay());
    String Year = String(rtc.getYear());
    String Current_Date= Month+" "+Date+", "+Year;
    tft.drawString(Current_Date, 10, 180,4);

    //Print DAY
    

    tft.setTextColor(TFT_PINK,TFT_BLACK);
    tft.drawString(day(rtc.getDayofWeek()),10,206,4);
    
    //Print TIME
    //tft.fillRect(10,130,140,48,0x0A4B);
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    //tft.setTextDatum(BR_DATUM);
    tft.setTextPadding(tft.textWidth("88:88",7));
    if (rtc.getHour(false)== 0 && rtc.getMinute() < 10){
      tft.drawString("12:0"+String(rtc.getMinute()),10,130,7);
    }
    else if(rtc.getHour(false)== 0 && rtc.getMinute() > 9){
      tft.drawString("12:"+String(rtc.getMinute()),10,130,7);
    }
    else if(rtc.getMinute()<10){
      tft.drawString(String(rtc.getHour(false))+":"+"0"+String(rtc.getMinute()),10,130,7);
    }
    else{
      tft.drawString(String(rtc.getHour(false))+":"+String(rtc.getMinute()),10,130,7);
    }
    tft.setTextPadding(0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  
}

//====================================================================================
//                                httpGETRequest
//====================================================================================
String httpGETRequest(const char* serverName){
  
  http.begin(client,serverName);
  int httpResponseCode= http.GET();

  if (httpResponseCode>0){
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  }
  else{
    Serial.print("Error code");
  }

  return http.getString();  
}
//====================================================================================
//                                Display Weather
//====================================================================================
void displayWeather(void * parameters){
  for (;;){
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    String serverpath= "http://api.openweathermap.org/data/2.5/weather?q=Delhi,IN&APPID=6e1643cd95fa2684c8663e2d62888fd1";
    JSON_buffer= httpGETRequest(serverpath.c_str());
    JSONVar myObject= JSON.parse(JSON_buffer);
    if (JSON.typeof(myObject)!="undefined"){
      printIcon(JSON.stringify(myObject["weather"][0]["icon"]));
      Serial.print("Temperature: ");
      int temp= int(myObject["main"]["temp"])-273.15;
      Serial.println(String(temp)+"°C");
      tft.drawString(String(temp),242,65,4);
      tft.drawString("o",270,60,2);
      tft.drawString("C",277,65,4);
      vTaskDelay(300000/portTICK_PERIOD_MS);
    }
    else{
      Serial.println("Parsing input failed!");
    }
  }
}
//====================================================================================
//                                    Setup
//====================================================================================
void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n Using the PNGdec library");

  // Initialise the TFT
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  //ScanForWiFi();
  //delay(5000);
  ConnectToWiFi();
  delay(5000);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0,0,2);
  tft.println("Connected");
  Serial.println("\r\nInitialisation done.");

  /*---------set rtc Manually---------------*/
  //rtc.setTime(58, 59, 23, 30, 9, 2020); // 30th September 2020, 23:59:58
  /*---------set rtc with NTP---------------*/
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)){
    rtc.setTimeStruct(timeinfo); 
  }

  xTaskCreatePinnedToCore(printLocalTime,"Dislay_Time&Date",100000,NULL,1,NULL,1);
  xTaskCreatePinnedToCore(displayWeather,"Dislay_Temp",20000,NULL,1,NULL,1);
  
}

//====================================================================================
//                                    Loop
//====================================================================================
void loop(){}

//====================================================================================
//                                 PNG DRAW
//====================================================================================
void pngDraw(PNGDRAW *pDraw) {
  uint16_t lineBuffer[MAX_IMAGE_WDITH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}
