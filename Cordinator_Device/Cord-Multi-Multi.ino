#include <GSM.h>
#include <XBee.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdlib.h>
#include <LiquidCrystal.h>
#include "DataStructures.h"


/////////////////////////////////   PUBLIC OBJECT INSTANCE    /////////////////////////////////
//GSM
GSM gsmAccess;
GSMClient client;
GPRS gprs;
GsmData gsmData;

//XBEE
XBee xbee = XBee();

//CLOCK
tmElements_t tm;
AlarmData alarmData;
AlarmID_t onAlarm ;

//End Devices
int devicesNum;
//Send measurements Buffer
int JARRAY_BUFFER_SIZE, JSON_BUFFER_SIZE;

/////////////////////////////////   Public variables    /////////////////////////////////
//WATERING
unsigned long previousMillis = 0, userRequestAlgorithmMillis = 0;

struct endDevice enddevices[MAX_DEVICES_INPUT];
struct record_data records[MAX_DEVICES_INPUT];
struct measuringWateringFlags deviceFlags[MAX_DEVICES_INPUT];

float measuresValues[4];


/////////////////////////////////   SETUP    /////////////////////////////////
void setup() {
  //bool parse = false;     //RTC flags
  bool config = false;      //RTC flags

  Serial.begin(9600);     //SERIAL CONFIGURATION

  ///////////////////////////////////////////////           ZIGBEE SETUP                       /////////////////////////
  Serial1.begin(9600);
  xbee.begin(Serial1);

  /////////////////////////////////////////////             RTC SETUP             /////////////////////////////////////
  Wire.begin();
  if (getDate(__DATE__) && getTime(__TIME__)) {
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }

  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet) {
    Serial.println(F("Unable to sync with the RTC"));
    //TODO set from gsm
  }
  else {
    Serial.println(F("RTC has set the system time"));
  }


  ///////////////////////////////////////////   LCD SETUP   ///////////////////////////////////////////////
  lcd.begin(16, 2);              // start the library
  lcd.setCursor(0, 0);
  lcd.print("Give pin number"); // print a simple message

  while (true) {
    unsigned long currentTime = millis();
    if (currentTime - previousMillis >= 5000) {  //delay(5000);
      previousMillis = currentTime;
      lcd.clear();
      break;
    }
  }

  lcd.setCursor(0, 0);
  lcd.print("0123456789"); // print a simple message
  lcd.setCursor(0, 1);
  lcd.print("____"); // print a simple message

  ///////////////////////////////////////////   GSM SETUP    ///////////////////////////////////////////////

  // Start GSM shield
  // If your SIM has PIN, pass it as a parameter of begin() in quotes

  boolean checkWhile = true;
  String PINNUMBER_LOCAL;
  PINNUMBER_LOCAL.reserve(4);

  while (true) {
    while (checkWhile) {
      lcd.setCursor(blinkX, 0);
      lcd.blink();
      int lcd_key = read_LCD_buttons();
      if (lcd_key == btnRIGHT && blinkX <= 9) {
        delay(150);
        blinkX++;
        lcd.setCursor(blinkX, 0);
      }
      else if (lcd_key == btnLEFT && blinkX >= 0) {
        delay(150);
        blinkX--;
        lcd.setCursor(blinkX, 0);
      } else if (lcd_key == btnSELECT && blinkY <= 3) {
        delay(200);
        lcd.setCursor(blinkY, 1);
        lcd.print(blinkX);
        lcd.setCursor(blinkX, 0);
        /////////////////////////// metatroph tou int se string kai meta se char /n the end of char array so b is 1+ /n
        String c = (String)blinkX;
        ///////////////////////////
        //PINNUMBER_LOCAL[blinkY] = c;
        PINNUMBER_LOCAL += c;
        Serial.println(c);
        blinkY++;
      }

      if (PINNUMBER_LOCAL[3] != (char)0) {
        checkWhile = false;
        char buf[4];
        PINNUMBER_LOCAL.toCharArray(buf, PINNUMBER_LOCAL.length() + 1);
        //gsmData.PINNUMBER = (char*)buf;
        gsmData.PINNUMBER = (char*)"8492";
        Serial.println(gsmData.PINNUMBER);
        Serial.println(F("PIN Initialized"));
      }
    }

    if ((gsmAccess.begin(gsmData.PINNUMBER) == GSM_READY) & (gprs.attachGPRS(gsmData.GPRS_APN, gsmData.GPRS_LOGIN, gsmData.GPRS_PASSWORD) == GPRS_READY)) {

      break;
    } else if (gsmAccess.begin(gsmData.PINNUMBER) != GSM_READY) {
      Serial.println("Wrong Password");
      checkWhile = true;
      for (int i = 0; i < 4; i++) {
        lcd.setCursor(i, 1);
        lcd.print("_");
      }
      blinkX = 0;
      blinkY = 0;
      PINNUMBER_LOCAL = "";
    } else {
      Serial.println(F("Not connected"));
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= 1000) {  //delay(1000);
        previousMillis = currentMillis;
      }
    }
  }

  Serial.println(F("GSM initialized"));

  /////////////////    GET ALARM DATA FROM SERVER   //////////////////////////////////
  //If you get a connection, report back via serial:
  //Try to connect to Server
  //Timeout connection for server 30sec
  int timeout = 30000;
  unsigned long currentTime = millis();

  while (true)
  {
    client.stop();
    if (connect())
    {
      const char* resource = "/embedded/setup?identifier=40E7CC41";
      //Send request and skipHeaders for parsing the response
      Serial.println(millis() - currentTime);
      if ((millis() - currentTime) >= timeout) {
        //TODO set flag for sim not initialized
        Serial.println("Im in");
        break;
      }

      sendRequest(resource);
      if (skipResponseHeaders())
      {
        char response[MAX_CONTENT_SIZE];
        readReponseContent(response, sizeof(response));

        if (parseSetupData(response, &alarmData)) {
          //////  Print Response Data  ////////////////////////
          printUserData(&alarmData);
          //// Set Alarm //////////////
          //TODO onAlarm
          onAlarm = Alarm.alarmOnce(alarmData.frHours, alarmData.frMinutes, 0, WakeUpAndTakeMeasures);
          client.stop();
          Serial.println(F("Disconnect"));
          break;
        }
      }
    }
    else
    {
      //TODO msg to lcd - connection on server failed
      break;
    }
  }

  for (int i = 0; i < devicesNum; i++) {
    enddevices[i].measureflag = false;
    enddevices[i].wateringflag = false;
    if (minpinNumber + i > maxpinNumber) {
      return;
    }
    enddevices[i].valvePin = minpinNumber + i;
  }

  previousMillis = millis();
  userRequestAlgorithmMillis = millis();
}


void loop() {
  Alarm.delay(1000);
  printTime();

  unsigned long currentMillis = millis();
  //Check if the time is in auto irrigation algorithm
  if (!checkAutomaticWaterTime()) {
    Serial.println("User Time");
    ///---------- User Request to embedded
    if (currentMillis - previousMillis >= userRequestInterval) { //CHECK FOR USER request
      //get request from server per 5minutes
      previousMillis = currentMillis;
      if (connect()) {
        const char* resource = "/embedded/measureIrrigation?identifier=40E7CC41";
        sendRequest(resource);
        if (skipResponseHeaders())
        {
          char response[MAX_CONTENT_SIZE];
          readReponseContent(response, sizeof(response));

          if (parseEndDevicesData(response)) {
            if (deviceFlags[0].measurement) {
              for (int i = 0; i < devicesNum; i++) {
                DeviceStart(enddevices[i].zigbeeaddress, i);  //WAKE UP & Take measures (BY USER REQUEST)
              }
              sendMeasuresToServer();
            }

            Serial.println(F("Disconnect"));
          }
          client.stop();
        }
      }
    }

    //    unsigned long currentTime = millis();
    //    if (currentTime - userRequestAlgorithmMillis >= userRequestAlgorithmInterval) {
    //      userRequestAlgorithmMillis = currentTime;
    //      if (connect()) {
    //        const char* resource = "/embedded/setup?identifier=40E7CC41";
    //        sendRequest(resource);
    //
    //        if (skipResponseHeaders()) {
    //          char response[MAX_CONTENT_SIZE];
    //          readReponseContent(response, sizeof(response));
    //
    //          if (parseSetupData(response, &alarmData)) {
    //            //////  Print Response Data  ////////////////////////
    //            printUserData(&alarmData);
    //
    //            //// Set Alarm ////
    //            onAlarm = Alarm.alarmOnce(alarmData.frHours, alarmData.frMinutes, 0, WakeUpAndTakeMeasures);
    //            client.stop();
    //            Serial.println(F("Setup Updated"));
    //          }
    //        }
    //      }
    //    }


    /////////////    check Measurement Irrigation Flags from Server (BY USER REQUEST) //////////////////////////////////////
    //    for (int i = 0; i < devicesNum; i++) {
    //      if (deviceFlags[i].irrigation) {
    //        // END IRRIGATION FROM USER Request/////
    //        if (hour() > deviceFlags[i].untilHour || (hour() == deviceFlags[i].untilHour && minute() >= deviceFlags[i].untilMinute) ) {
    //          Serial.println("END THE IRRIGATION NOW!!!");
    //          digitalWrite(enddevices[i].valvePin, LOW);
    //          deviceFlags[i].startIrrigationFlag = false;
    //          sendIrrigationToServer(i);
    //          deviceFlags[i].irrigation = false;
    //          //TODO set enddevices[i].startDateTime from USER REQUEST from struct measuringWateringFlags (deviceFlags[i].fromhour, deviceFlags[i].fromminute)
    //          //TODO set enddevices[i].endDateTime = Current moment & send irrigation times to server
    //          continue;
    //        }
    //
    //        if (hour() > deviceFlags[i].fromHour || (hour() == deviceFlags[i].fromHour && minute() >= deviceFlags[i].fromminute) ) {
    //          Serial.println("START THE IRRIGATION NOW!!!");
    //          digitalWrite(enddevices[i].valvePin, HIGH);
    //          deviceFlags[i].startIrrigationFlag = true;
    //        }
    //      }
    //    }
    //----END of USER Request to Embeeded---/////////
  } else {
    Serial.println("Automatic Algorithm Time");
    for (int i = 0; i < devicesNum; i++) {
      Serial.print("DtimeIrrigation : ");
      Serial.println(enddevices[i].DtimeIrrigation);
      if (enddevices[i].wateringflag == true) {
        digitalWrite(enddevices[i].valvePin, HIGH);
      } else {
        digitalWrite(enddevices[i].valvePin, LOW);
        if (enddevices[i].DtimeIrrigation != "") {
          Serial.print("End of Irrigation ");
          Serial.print(i, DEC);
          Serial.println(enddevices[i].DtimeIrrigation);
          //TDO send data to server for automatic watering
          enddevices[i].DtimeIrrigation = "";
        }
      }
    }
  }
}

void sendIrrigationToServer(int counter) {
  /////////  TO DO ----   CHANGE TO DYNAMIC BUFFER -----------------------
  DynamicJsonBuffer jsonBuffer(JSON_BUFFER_SIZE);

  JsonObject& root = jsonBuffer.createObject();
  root["autoIrrigFromTime"] = getDateTime(deviceFlags[counter].fromHour, deviceFlags[counter].fromminute);
  root["autoIrrigUntilTime"] = getDateTime(hour(), minute());
  root["waterConsumption"] = 0;

  String strAddr(deviceFlags[counter].zbAddress, HEX);
  strAddr.reserve(8);
  strAddr.toUpperCase();
  root["identifier"] = strAddr;


  root.prettyPrintTo(Serial);
  if (client.connect(gsmData.server, gsmData.port)) {
    Serial.println(F("connected...Sending Post..."));

    client.println("POST /FarmCloud/embedded/manualwatering/save/ HTTP/1.0");
    client.println("Host: 78.46.70.93");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/json;charset=UTF-8");
    client.print("Content-Length: ");
    client.println(root.measureLength());
    client.println();

    root.printTo(client);
  } else {
    Serial.println(F("NOT CONNECTED"));
  }

  Serial.println(F("Disconnect"));
  client.stop();


}

String getDateTime(int hours , int minutes) {
  String dtime;
  dtime.reserve(19);
  dtime = String(String(year()) + "-" + String(month()) + "-" + String(day()) + " " + String(hours) + ":" + String(minutes) + ":00");
  return dtime;
}

boolean checkAutomaticWaterTime() {
  int alarmfromminutes = alarmData.frHours * 60 + alarmData.frMinutes;
  int alarmtominutes = alarmData.toHours * 60 + alarmData.toMinutes;
  int nowTotalminutes = hour() * 60 + minute();

  if (nowTotalminutes >= alarmfromminutes) {
    if (nowTotalminutes <= alarmtominutes) { //p.e 11:00 - 15:00 now: 12:00
      return true;
    } else {
      if (alarmData.toHours < alarmData.frHours) {
        alarmtominutes = 24 * 60 + alarmData.toHours * 60 + alarmData.toMinutes;
      }

      if ((nowTotalminutes >= alarmfromminutes) && (nowTotalminutes <= alarmtominutes)) {
        return true;
      }
    }
  } else {
    if (nowTotalminutes <= alarmtominutes) { //p.e 23:15 - 04:10 now: 01:55
      return true;
    } else {                                //p.e 11:00 - 15:00 now: 10:00
      return false;
    }
  }

  return false;
}

//////////////////////     FUNCTIONS FOR RTC SETUP  //////////////////////////////////////
//////////////             get Time-Date from PC    /////////////////////
bool getTime(const char *str) {
  int Hour, Min, Sec;
  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;

  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str) {
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

/////////////////////////////////////////////        Open connection to the HTTP server  ////////////////////////////////////////////////////////////////////////////////
bool connect() {
  Serial.print(F("Connecting to Server... "));
  bool ok = client.connect(gsmData.server, gsmData.port);
  Serial.println(ok ? "Connected!" : "Connection Failed!");
  return ok;
}

//////////////////////////////////////////////////////    FUNCTIONS FOR GETTING SERVER SETUP   //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////      Send the HTTP GET request to the server    //////////////////////////////////////////////
void sendRequest(const char* resource) {
  Serial.println(F("Sending Request ......"));

  // Make a HTTP request:
  client.print("GET /FarmCloud");
  client.print(resource);
  client.println(" HTTP/1.1");
  client.println("Host: 78.46.70.93");
  client.println("Connection: close");
  client.println();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////            Skip HTTP headers so that we are at the beginning of the response's body  //////////////////////////////////////////////////////////
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";
  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println(F("No response or invalid response!"));
  }

  return ok;
}

/////////////////////////////////////////////////////////////      Read the body of the response from the HTTP server //////////////////////////////////////////////////////
void readReponseContent(char* content, size_t maxSize) {
  size_t length = client.readBytes(content, maxSize);
  content[length] = 0;
}

/////////////////////////////////////////////////   Parse end devices data from Json to STRUCT    ///////////////////////////////////////////////////////////////////////////
bool parseEndDevicesData(char* content) {
  DynamicJsonBuffer jsonBuffer;
  JsonArray& root = jsonBuffer.parseArray(content);

  root.prettyPrintTo(Serial);

  if (!root.success()) {
    Serial.println(F("JSON parsing failed!"));
    return false;
  }

  for (int i = 0; i < devicesNum; i++) {
    const char* identifier = root[i]["id"];
    deviceFlags[i].zbAddress = strtoul(identifier, NULL, 16);
    deviceFlags[i].irrigation = root[i]["irrig"];
    deviceFlags[i].measurement = root[i]["meas"];
    deviceFlags[i].fromHour = root[i]["frh"];
    deviceFlags[i].fromminute = root[i]["frm"];
    deviceFlags[i].untilHour = root[i]["toh"];
    deviceFlags[i].untilMinute = root[i]["tom"];
  }
  return true;
}

/////////////////////////////////////////////////   Parse setup data from Json to STRUCT    /////////////////////////////////////////////////////////////////////////////////
bool parseSetupData(char* content, struct AlarmData * alarmData) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // This is only required if you use StaticJsonBuffer.
  //const size_t BUFFER_SIZE = JSON_OBJECT_SIZE(6);

  // Allocate a temporary memory pool on the stack
  //StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  // If the memory pool is too big for the stack, use this instead:
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(content);

  if (!root.success()) {
    Serial.println(F("JSON parsing failed!"));
    return false;
  }

  root.prettyPrintTo(Serial);

  //TODO must change for enddevices
  // Here were copy the strings we're interested in
  alarmData->frHours    = root["fh"];
  alarmData->frMinutes  = root["fm"];
  alarmData->toHours    = root["th"];
  alarmData->toMinutes  = root["tm"];
  devicesNum = root["num"];

  JARRAY_BUFFER_SIZE = devicesNum * 350;
  JSON_BUFFER_SIZE = JARRAY_BUFFER_SIZE + 20;

  Serial.print(F("devicesNum = "));
  Serial.println(devicesNum);

  //struct endDevice enddevices[devicesNum];

  for (int i = 0; i < devicesNum; i++) {
    const char* idenntifier = root["devices"][i]["id"];
    //enddevices[i].zigbeeCharAddress = strdup(idenntifier);
    enddevices[i].zigbeeaddress = strtoul(idenntifier, NULL, 16);

    enddevices[i].minhumidity = root["devices"][i]["minh"];
    enddevices[i].maxhumidity = root["devices"][i]["maxh"];
    enddevices[i].mintemp = root["devices"][i]["mint"];
    enddevices[i].maxtemp = root["devices"][i]["maxt"];
    enddevices[i].measureflag = false;
    enddevices[i].wateringflag = false;
    enddevices[i].valvePin = minpinNumber + i;

    ///////////////////////   SOLENOID OUTPUT PINS    ///////////////////////
    pinMode(minpinNumber + i, OUTPUT);
  }

  return true;
}

///////////////////////////////////////////////////            Print the data extracted from the JSON   //////////////////////////////////////////////////////////
void printUserData(const struct AlarmData * alarmData) {
  Serial.print(F("FROM Hours = "));
  Serial.println(alarmData->frHours);
  Serial.print(F("FROM Minutes = "));
  Serial.println(alarmData->frMinutes);
  Serial.print(F("TO Hours = "));
  Serial.println(alarmData->toHours);
  Serial.print(F("TO Minutes = "));
  Serial.println(alarmData->toMinutes);
}

void WakeUpAndTakeMeasures() {
  printDateTime();
  Serial.println("..........Wake Up End Devices And Take Measures..........");
  boolean chk = false;
  //TODO compare with humidity and temperature min and max
  int frHours = 0;
  int frMinutes = 0;

  for (int i = 0; i < devicesNum; i++) {
    //Set watering flag
    if (DeviceStart(enddevices[i].zigbeeaddress, i)) {
      enddevices[i].wateringflag = true;
      chk = true;
      if (enddevices[i].DtimeIrrigation == "") {
        String minutestr;
        minutestr.reserve(2);
        minutestr =  String(minute() < 10 ? "0" + String(minute()) : String(minute()));
        String dtime;
        dtime.reserve(19);
        dtime = String(String(year()) + "-" + String(month()) + "-" + String(day()) + " " + String(hour()) + ":" + minutestr + ":00");
        enddevices[i].DtimeIrrigation = dtime;
      }
    } else {
      enddevices[i].wateringflag = false;
    }

    unsigned long kathisterisi = millis();
    while (millis() - kathisterisi < 8000) { } //do nothing
  }

  if (chk) {
    if (minute() >= 50) {
      frMinutes = minute() % 50;
      frHours = addhours(1);
    } else {
      frMinutes = minute() + 2;
      frHours = hour();
    }

    Serial.print("FROM HOURS :");
    Serial.println(frHours);
    Serial.print("FROM MINUTES : ");
    Serial.println(frMinutes);

    onAlarm = Alarm.alarmOnce(frHours, frMinutes, 30, WakeUpAndTakeMeasures);
  } else {
    frHours = addhours(2);
    frMinutes = minute();

    //checkIrrigationUntilTime(frHours, frMinutes) ? Alarm.alarmOnce(alarmData.frHours,alarmData.frMinutes, 0, WakeUpAndTakeMeasures) : Alarm.alarmOnce(frHours,frMinutes, 0, WakeUpAndTakeMeasures);

    if (checkIrrigationUntilTime(frHours, frMinutes)) {
      onAlarm = Alarm.alarmOnce(alarmData.frHours, alarmData.frMinutes, 0, WakeUpAndTakeMeasures);
    } else {
      onAlarm = Alarm.alarmOnce(frHours, frMinutes, 0, WakeUpAndTakeMeasures);
    }
  }

  sendMeasuresToServer();

}

void sendMeasuresToServer() {
  /////////  TO DO ----   CHANGE TO DYNAMIC BUFFER -----------------------
  DynamicJsonBuffer jsonBuffer(JSON_BUFFER_SIZE);
  DynamicJsonBuffer jsonBufferNes(JARRAY_BUFFER_SIZE);

  //StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
  //StaticJsonBuffer<JARRAY_BUFFER_SIZE> jsonBufferNes;

  JsonObject& root = jsonBuffer.createObject();
  JsonArray& endDevices = root.createNestedArray("emList");

  for (int i = 0; i < devicesNum; i++) {
    //TODO change from 3 to size of end devices count
    for (int j = 0; j < ARRAYSIZE; j++)
    {
      JsonObject& nestedroot =  jsonBufferNes.createObject();

      nestedroot["dt"] = records[i].Dtime;
      nestedroot["zb"] = records[i].zbaddress;
      //nestedroot["observableProperty"] = results[j];

      if (j == 0) {           //humidity
        nestedroot["mv"] =  records[i].Shumidity;
        nestedroot["oid"] =  1;
        nestedroot["uid"] = 21;
      } else if (j == 1) {    //Internal temprature
        nestedroot["mv"] = records[i].itemp;
        nestedroot["oid"] =  2;
        nestedroot["uid"] = 7;
      } else if (j == 2) {    //External temperature
        nestedroot["mv"] = records[i].wtemp;
        nestedroot["oid"] =  3;
        nestedroot["uid"] = 7;
      } else { //j=3          //Soil
        nestedroot["mv"] = records[i].soilm;
        nestedroot["oid"] =  4;
        nestedroot["uid"] = 21;
      }
      endDevices.add(nestedroot);
    }
  }

  root.prettyPrintTo(Serial);
  if (client.connect(gsmData.server, gsmData.port)) {
    Serial.println(F("connected...Sending Post..."));

    client.println("POST /FarmCloud/embedded/savemeasures/ HTTP/1.0");
    client.println("Host: 78.46.70.93");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/json;charset=UTF-8");
    client.print("Content-Length: ");
    client.println(root.measureLength());
    client.println();

    root.printTo(client);
  } else {
    Serial.println(F("NOT CONNECTED"));
  }

  Serial.println(F("Disconnect"));
  client.stop();

}

/////////////////////////  Start End Device Process  //////////////////////////////////////////////////
boolean DeviceStart(uint32_t destAddress, int counter) {
  Serial.print(F("Starting End Device Process: "));
  Serial.println(destAddress, HEX);

  //TODO must change to char*;
  String sensorsData;
  sensorsData.reserve(25);
  unsigned long previousMillis = millis();

  if (WakeUpEndDevice(destAddress)) {
    while (millis() - previousMillis < 5000) {
      /*wait for 5sec*/
    }
    Serial.println(F("Reading End Device Sensor Data..."));
    sensorsData = ReadEndDeviceData();

    if (sensorsData == "M") {
      return false;
      /// MODEM STATUS
    } else if (sensorsData == "F") {
      return false;
    } else if (sensorsData == "O") {
      return false;
    } else { //We have sample Ladies

      float humidity, itemp, wtemp;
      int soil;

      //char buf[30];
      //sensorsData.toCharArray(buf, sensorsData.length() + 1);

      //sscanf(buf, "%f,%f,%f,%d", humidity, itemp, wtemp, soil);
      splitMeasures(sensorsData);
      humidity = measuresValues[0];
      itemp = measuresValues[1];
      wtemp = measuresValues[2];
      soil = map((int)measuresValues[3], 270, 1023, 100, 0);

      //float* observationValues;
      //observationValues = splitMeasures(sensorsData);

      Serial.println(F("-----MEASURES GET FROM END DEVICE------"));
      Serial.print(F("END DEVICE ADDRESS :"));
      Serial.println(destAddress, HEX);
      Serial.print(F("HUMIDITY :"));
      Serial.println(humidity);
      Serial.print(F("Int Temperature :"));
      Serial.println(itemp);
      Serial.print(F("WaterProof Temp :"));
      Serial.println(wtemp);
      Serial.print(F("SOIL :"));
      Serial.println(soil);

      //TODO all string must change to char array
      String minutestr;
      minutestr.reserve(2);
      minutestr =  String(minute() < 10 ? "0" + String(minute()) : String(minute()));

      //TODO change to const char
      String dtime;
      dtime.reserve(19);
      dtime = String(String(year()) + "-" + String(month()) + "-" + String(day()) + " " + String(hour()) + ":" + minutestr + ":00");

      records[counter].Dtime = dtime;

      String strAddr(destAddress, HEX);
      strAddr.toUpperCase(); //TODO change on server
      records[counter].zbaddress = strAddr;

      //char buff [8];
      //records[counter].zbaddress = itoa(destAddress,buff,16);
      records[counter].Shumidity = humidity;
      records[counter].itemp = itemp;
      records[counter].wtemp = wtemp;
      records[counter].soilm = soil;

      //Serial.println(records[counter].zbaddress, HEX);
      Serial.println(F("End Parse Data"));

      if (wtemp > enddevices[counter].maxtemp && soil < enddevices[counter].minhumidity) {
        Serial.println(F("Start watering: true"));
        //enddevices[counter]
        return true;
      }
      return false;
    }
  } else {
    sensorsData = "EF";
    Serial.println(F("EF"));
    return false;
  }
}

boolean checkIrrigationUntilTime(int frHours, int frMinutes) {
  int alarmTotalMinues = alarmData.toHours * 60 + alarmData.toMinutes;
  int nextTotalMinutes = frHours * 60 + frMinutes;

  if (nextTotalMinutes >= alarmTotalMinues) {
    return true;
  }
  return false;
}

int addhours(int hours) {
  int nexthour = hour() + hours;
  if (hour() > 23) {
    nexthour = hour() - 24; //22, 23
  }
  return nexthour;
}

///////////////////////////////////////////////// WAKE UP END DEVICE //////////////////////////////////////////////////////////
boolean WakeUpEndDevice(uint32_t DestAddress) {
  //  previousMillis = millis();

  Serial.print(F("Waking Up End Device: "));
  Serial.println(DestAddress, HEX);
  String str = "W";
  byte byteDataSend[str.length() + 1];
  str.getBytes(byteDataSend, str.length() + 1);

  XBeeAddress64 addr64 = XBeeAddress64();
  addr64.setMsb(0X0013A200);//XXXXX -> Msb address of router/end node
  addr64.setLsb(DestAddress);//XXXXX -> Lsb address of router/end node
  ZBTxRequest zbTx = ZBTxRequest(addr64, byteDataSend, sizeof(byteDataSend));
  ZBTxStatusResponse txStatus = ZBTxStatusResponse();
  xbee.send(zbTx);

  // after sending a tx request, we expect a status response
  // wait up to half second for the status response
  if (xbee.readPacket(500)) {
    // got a response!
    //     Serial.print(" ApiID:");
    //     Serial.println(xbee.getResponse().getApiId());
    // should be a znet tx status
    if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
      //          Serial.print("ZB_TX_STATUS_RESPONSE ApiID:");
      //          Serial.println(xbee.getResponse().getApiId());
      xbee.getResponse().getZBTxStatusResponse(txStatus);

      // get the delivery status, the fifth byte
      if (txStatus.getDeliveryStatus() == SUCCESS) {
        Serial.println(F("END DEVICE GOT THE MESSAGE"));
        return true;
        //        unsigned long currentMillis = millis();
        //        Serial.print("Send packet data FOR WAKE UP: "); //360 sec
        //        Serial.print(currentMillis - previousMillis); //360 sec
        //        Serial.println(" msec"); //360 sec
      } else {
        // the remote XBee did not receive our packet. is it powered on?
        Serial.println(F("END DEVICE DID NOT GET THE MESSAGE"));
        return false;
      }
    }
  } else if (xbee.getResponse().isError()) {

    //   Serial.print("ERROR READING PACKET.ERROR CODE:");
    //   Serial.println(xbee.getResponse().getErrorCode());

    //nss.print("Error reading packet.  Error code: ");
    //nss.println(xbee.getResponse().getErrorCode());
  } else {
    return false;
    // local XBee did not provide a timely TX Status Response -- should not happen
    //  Serial.println("local XBee did not provide a timely TX Status Response -- should not happen");
  }

}

String ReadEndDeviceData() {
  // previousMillis = millis();
  //TODO reserve Sample
  String sample;
  sample.reserve(25);
  ZBRxResponse rx = ZBRxResponse();
  ModemStatusResponse msr = ModemStatusResponse();

  //  Serial.println("mphka sthn read end device");
  xbee.readPacket();
  if (xbee.getResponse().isAvailable()) {
    // got something
    //  Serial.print("ApiID");
    //  Serial.println(xbee.getResponse().getApiId());
    if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
      // got a zb rx packet
      //     Serial.print("ZB_RX_RESPONSE ApiID");
      //   Serial.println(xbee.getResponse().getApiId());

      // now fill our zb rx class
      xbee.getResponse().getZBRxResponse(rx);

      if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
        // the sender got an ACK
        //     Serial.println("SENDER GOT AN ACK");
      } else {
        // we got it (obviously) but sender didn't get an ACK
        //     Serial.println("SENDER DID NOT GET AN ACK");
      }
      for (int i = 0; i < rx.getDataLength(); i++) {
        sample += (char)rx.getData(i);
      }
      Serial.println(sample);
    } else if (xbee.getResponse().getApiId() == MODEM_STATUS_RESPONSE) {
      xbee.getResponse().getModemStatusResponse(msr);
      // the local XBee sends this response on certain events, like association/dissociation
      //  Serial.print("MODEM STATUS RESPONSE :");
      //  Serial.println(xbee.getResponse().getApiId());
      if (msr.getStatus() == ASSOCIATED) {
        // yay this is great.  flash led

        //    Serial.println("ASSOCIATED");

      } else if (msr.getStatus() == DISASSOCIATED) {
        // this is awful.. flash led to show our discontent
        //    Serial.println("DISASSOCIATED");
      } else {
        // another status
        //    Serial.println("NORMAL STATUS");
      }
      sample = "M";

    } else {

      sample = "O";
    }
  } else if (xbee.getResponse().isError()) {
    //  Serial.println("ERROR READING PACKET");
    sample = "F";

  }
  return  sample;
}

void printDateTime() {
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year());
  Serial.print("\t");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.println(second());
}

void printTime() {
  Serial.print(hour(), DEC);
  Serial.print(":");
  Serial.print(minute(), DEC);
  Serial.print(":");
  Serial.println(second(), DEC);
}

// Calculate based on max input size expected for one command
#define INPUT_SIZE 30
void splitMeasures(String input) {
  Serial.println(input);

  char buf[INPUT_SIZE];
  input.toCharArray(buf, input.length() + 1);
  char * pch;
  pch = strtok (buf, ",");
  int counter = 0;
  while (pch != NULL)
  {
    //Serial.println(pch);
    float f;
    f = (float)atof(pch);
    measuresValues[counter] = f;
    pch = strtok (NULL, ",");
    counter++;
  }
}

// read the buttons
int read_LCD_buttons() {
  int adc_key_in = analogRead(0);      // read the value from the sensor
  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 980) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
  // For V1.1 us this threshold
  Serial.println(adc_key_in);

  if (adc_key_in < 100)  return btnRIGHT;
  if (adc_key_in < 250)  return btnUP;
  if (adc_key_in < 350)  return btnDOWN;
  if (adc_key_in < 550)  return btnLEFT;
  if (adc_key_in < 850)  return btnSELECT;

  // For V1.0 comment the other threshold and use the one below:
  /*
    if (adc_key_in < 50)   return btnRIGHT;
    if (adc_key_in < 195)  return btnUP;
    if (adc_key_in < 380)  return btnDOWN;
    if (adc_key_in < 555)  return btnLEFT;
    if (adc_key_in < 790)  return btnSELECT;
  */


  return btnNONE;  // when all others fail, return this...
}
