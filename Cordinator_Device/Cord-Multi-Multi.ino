#include <GSM.h>
#include <XBee.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <ArduinoJson.h>



const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

#define PINNUMBER "8492"

////////////////////--------------- APN information obrained from your network provider------------------//////////////////////
#define GPRS_APN       "internet.vodafone.gr" // replace with your GPRS APN
#define GPRS_LOGIN     ""    // replace with your GPRS login
#define GPRS_PASSWORD  "" // replace with your GPRS password
//////////////////////////////////////////////////////////////////////////////////////////////////////////
XBee xbee = XBee();
GSM gsmAccess;
GSMClient client;
GPRS gprs;
tmElements_t tm;
AlarmID_t onAlarm ;
int Valve1pin = 23;
int Valve2pin = 25;
int Valve3pin = 27;
unsigned long previousMillis = 0;
boolean wakedup = false;
boolean readData = false;
boolean flag2wres = false;
boolean flag10lepta = false;
boolean dev1flag = false;
boolean dev2flag = false;
boolean dev3flag = false;
boolean watering1 = false;
boolean watering2 = false;
boolean watering3 = false;
int MaxSoil = 500;
int Counter = 0;
int pointEnd3;
int pointEnd2;
int pointEnd1;

const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response

//////////////SERVER INFO ////////////////
char server[] = "78.46.70.93"; // the base URL
int port = 8080; // the port, 80 for HTTP

// The type of data that we want to extract from the page
struct AlarmData {
  int frMinutes;
  int frHours;
  int toMinutes;
  int toHours;
};
/////////Send data post struct //////////////////////
//////////////////////////////////////////////////////
typedef struct {
  String Dtime;
  String zbaddress;
  float Shumidity;
  float itemp;
  float wtemp;
  int soilm;
} record_data;
record_data record[3];


struct measuringWateringFlags {
  String identifier = "";
  String irrigation = "false";
  int untilMoment = 0;
  String measurement = "false";
  int untilHour = 0;

};
measuringWateringFlags recordFlag[3];

////////////////////////////////////////////////////////////////////////////////     SETUP  ////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

  ///  SOLENOID OUTPUT PINS ///////////////////////
  pinMode(Valve1pin, OUTPUT);
  pinMode(Valve2pin, OUTPUT);
  pinMode(Valve3pin, OUTPUT);

  bool parse = false;
  bool config = false;
  Serial.begin(9600); //////////  SERIAL CONFIGURATION //////////////

  ///////////////////////////////////////////////           ZIGBEE SETUP                       /////////////////////////
  Serial1.begin(9600);
  xbee.begin(Serial1);
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////             RTC SETUP             /////////////////////////////////////
  Wire.begin();
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  }
  else {
    Serial.println("RTC has set the system time");
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////                GSM SETUP    ///////////////////////////////////////////////
  //// connection state
  boolean notConnected = true;

  // Start GSM shield
  // If your SIM has PIN, pass it as a parameter of begin() in quotes
  while (notConnected)
  {
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &
        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY))
      notConnected = false;
    else
    {
      Serial.println("Not connected");
      delay(1000);
    }
  }
  Serial.println("GSM initialized");
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /////////////////    GET ALARM DATA FROM SERVER //////////////////////////////////
  // if you get a connection, report back via serial:
  //TRy to connect to Server
  if (connect()) {
    const char* resource = "/getsetup?identifier=40E7CC41";
    //Send request and skipHeaders for parsing the response
    if (sendRequest(resource) && skipResponseHeaders()) {
      char response[MAX_CONTENT_SIZE];
      readReponseContent(response, sizeof(response));
      AlarmData alarmData;
      if (parseUserData(response, &alarmData)) {
        ////     Print Response Data ////////////////////////
        printUserData(&alarmData);

        //// Set Alarm //////////////
        onAlarm = Alarm.alarmOnce(alarmData.frHours, alarmData.frMinutes, 0, WakeUpAndTakeMeasures);

        Serial.println("Disconnect");
        client.stop();

      }
    }
  }
}
////////////////////////////////////////////////////////////////////////     END SETUP ////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////    LOOP //////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // Enable Alarms
  Alarm.delay(1000);




  if (watering1) {
    Serial.println("WATERING 1");

    if (recordFlag[0].untilHour == hour() && recordFlag[0].untilMoment >= minute()  ) {
      dev1flag = true;
    } else {
      watering1 = false;
      dev1flag = false;
    }
  } else {
    dev1flag = false;
  }



  if (watering2) {
    Serial.println("WATERING 2");
    if (recordFlag[1].untilHour == hour() && recordFlag[1].untilMoment >= minute()  ) {
      dev2flag = true;
    } else {
      watering2 = false;
      dev2flag = false;
    }
  } else {
    dev2flag = false;
  }


  if (watering3) {
    Serial.println("WATERING 3");
    Serial.println(recordFlag[2].untilHour);
    Serial.println(hour());
    Serial.println(recordFlag[2].untilMoment);
    Serial.println(minute());
    if (recordFlag[2].untilHour == hour() && recordFlag[2].untilMoment >= minute()  ) {
      dev3flag = true;
    } else {
      watering3 = false;
      dev3flag = false;
    }
  } else {
    dev3flag = false;
  }




  if (dev1flag) {
    ////////watering device 1
    digitalWrite(Valve1pin, HIGH);
  } else {
    digitalWrite(Valve1pin, LOW);
  }
  if (dev2flag) {
    ////////watering device 2
    digitalWrite(Valve2pin, HIGH);
  } else {
    digitalWrite(Valve2pin, LOW);
  }
  if (dev3flag) {
    ////////watering device 3
    digitalWrite(Valve3pin, HIGH);
  } else {
    digitalWrite(Valve3pin, LOW);
  }
  /////////////    check Measurement Irrigation Flags from Server //////////////////////////////////////
   AlarmData alarmData;

  Serial.print("WRES");
  Serial.println(&alarmData.frHours);
  
  if (hour() < &alarmData.frHours || hour() > &alarmData.toHours ) {
    if (connect()) {
      const char* resource = "/getMeasIrr?identifier=40E7CC41";
      //Send request and skipHeaders for parsing the response
      if (sendRequest(resource) && skipResponseHeaders()) {
        char response[MAX_CONTENT_SIZE];
        readReponseContent(response, sizeof(response));

        if (parseMeasIrrigationData(response, recordFlag)) {
          ////     Print Response Data ////////////////////////
          //  printUserData(&alarmData);

          Serial.println("Disconnect");
          client.stop();

          // digitalWrite(Valve3pin, LOW);

          if (recordFlag[0].measurement == "true" && recordFlag[1].measurement == "true" && recordFlag[2].measurement == "true") {


            uint32_t DestAddress1 = {0X40E7CC39};
            uint32_t DestAddress2 = {0X40D6A2C9};
            uint32_t DestAddress3 = {0X40D6A2CF};

            DeviceStart(DestAddress1);

            unsigned long kathisterisi1 = millis();
            while (millis() - kathisterisi1 < 8000) {}

            DeviceStart(DestAddress2);

            unsigned long kathisterisi = millis();
            while (millis() - kathisterisi < 8000) {}

            DeviceStart(DestAddress3);


            Serial.println("DATA TO SEND");
            Serial.println(record[0].zbaddress);
            Serial.println(record[1].zbaddress);
            Serial.println(record[2].zbaddress);
            Serial.println("--------------------------------");

            /////////  TO DO ----   CHANGE TO DYNAMIC BUFFER -----------------------
            StaticJsonBuffer<1300> jsonBuffer;
            StaticJsonBuffer<1300> jsonBufferNes;
            JsonObject& root = jsonBuffer.createObject();
            JsonArray& end1dev = root.createNestedArray("embeddedDataList");

#define ARRAYSIZE 4
            String results[ARRAYSIZE] = { "humidity", "itemp", "wtemp", "soil" };


            for (int k = 0; k < 3; k++) {
              if (record[k].zbaddress != "") {

                for (int j = 0; j < ARRAYSIZE; j++) {
                  JsonObject& nestedroot = jsonBufferNes.createObject();
                  nestedroot["datetime"] = record[k].Dtime;
                  nestedroot["zbAddress"] = record[k].zbaddress;
                  nestedroot["observableProperty"] = results[j];
                  if (j == 0) {
                    nestedroot["measureValue"] =  record[k].Shumidity;
                    nestedroot["obsid"] =  1;
                    nestedroot["obspropid"] = 21;
                  } else if (j == 1) {
                    nestedroot["measureValue"] = record[k].itemp;
                    nestedroot["obsid"] =  2;
                    nestedroot["obspropid"] = 7;
                  } else if (j == 2) {
                    nestedroot["measureValue"] = record[k].wtemp;
                    nestedroot["obsid"] =  3;
                    nestedroot["obspropid"] = 7;
                  } else { //j=3
                    nestedroot["measureValue"] = record[k].soilm;
                    nestedroot["obsid"] =  4;
                    nestedroot["obspropid"] = 21;
                  }

                  end1dev.add(nestedroot);

                  //    nestedroot["humidity"] = record[k].Shumidity;
                  //    nestedroot["itemp"] = record[k].itemp;
                  //    nestedroot["wtemp"] = record[k].wtemp;
                  //    nestedroot["soil"] = record[k].soilm;

                }
              }
            }

            root.prettyPrintTo(Serial);


            if (client.connect(server, port)) {
              Serial.println("connected...Sending Post...");
              client.println("POST /FarmCloud/embedded/ HTTP/1.0");
              client.println("Host: 78.46.70.93");
              client.println("User-Agent: Arduino/1.0");
              client.println("Connection: close");
              client.println("Content-Type: application/json;charset=UTF-8");
              client.print("Content-Length: ");
              client.println(root.measureLength());
              client.println();

              root.printTo(client);

            } else
            {
              Serial.println("NOT CONECTED");
            }

            Serial.println("Disconnect");
            client.stop();

          }
          else if (recordFlag[0].irrigation == "true" || recordFlag[1].irrigation == "true" || recordFlag[2].irrigation == "true" ) {

            //   digitalWrite(Valve3pin, HIGH);

            Serial.println("Watering....");



            for (int v=0 ; v < 3; v++) {
              if (recordFlag[v].irrigation == "true") {
                if (recordFlag[v].identifier == "40E7CC39") {
                  watering1 = true;
          pointEnd1 = v;
                }
                else if (recordFlag[v].identifier == "40D6A2C9") {
                  watering2 = true;
          pointEnd2 = v;
                }
                else if (recordFlag[v].identifier == "40D6A2CF") {
                  watering3 = true;
          pointEnd3 = v;
                }
              }
            }




            //            recordFlag[0].untilHour = recordFlag[0].untilHour + hour();
            //            recordFlag[0].untilMoment = recordFlag[0].untilMoment + minute();
            //
            //
            //            recordFlag[1].untilHour = recordFlag[1].untilHour + hour();
            //            recordFlag[1].untilMoment = recordFlag[1].untilMoment + minute();
            //
            //
            //            recordFlag[2].untilHour = recordFlag[2].untilHour + hour();
            //            recordFlag[2].untilMoment = recordFlag[2].untilMoment + minute();

            Serial.print("Watering 1endDevice :");
            Serial.print(recordFlag[0].untilHour);
            Serial.print(":");
            Serial.print(recordFlag[0].untilMoment);
            Serial.print("Boolean");
            Serial.print(recordFlag[0].irrigation);
            Serial.print(",");
            Serial.println(watering1);
            Serial.print("Watering 2EndDevice");
            Serial.print(recordFlag[1].untilHour);
            Serial.print(":");
            Serial.print(recordFlag[1].untilMoment);
            Serial.print("Boolean");
            Serial.print(recordFlag[1].irrigation);
            Serial.print(",");
            Serial.println(watering2);
            Serial.print("Watering 3EndDevice");
            Serial.print(recordFlag[2].untilHour);
            Serial.print(":");
            Serial.print(recordFlag[2].untilMoment);
            Serial.print("Boolean");
            Serial.print(recordFlag[2].irrigation);
            Serial.print(",");
            Serial.println(watering3);


            ///// send post ///////////////////


            StaticJsonBuffer<800> jsonBuffer;
            StaticJsonBuffer<800> jsonBufferNes;
            JsonObject& root = jsonBuffer.createObject();
            JsonArray& end1dev = root.createNestedArray("deviceIrrigation");

            for (int b = 0; b < 3; b++) {
              JsonObject& nestedroot = jsonBufferNes.createObject();
              nestedroot["identifier"] = recordFlag[b].identifier;
              nestedroot["irrigationFlag"] = recordFlag[b].irrigation;

              end1dev.add(nestedroot);

            }

            root.prettyPrintTo(Serial);



            if (client.connect(server, port)) {
              Serial.println("connected...Sending Post...");
              client.println("POST /FarmCloud/setFlagIrr/ HTTP/1.0");
              client.println("Host: 78.46.70.93");
              client.println("User-Agent: Arduino/1.0");
              client.println("Connection: close");
              client.println("Content-Type: application/json;charset=UTF-8");
              client.print("Content-Length: ");
              client.println(root.measureLength());
              client.println();

              root.printTo(client);

            } else
            {
              Serial.println("NOT CONECTED");
            }

            Serial.println("Disconnect");
            client.stop();

          }

          for (int i = 0; i < 3; i++) {
            recordFlag[0].measurement == "false" ;
      recordFlag[0].irrigation == "false" ;
          }

        }
      }
      client.stop();
    }



  }



}
//////////////////////////////////////////////////////////////////// END LOOP ///////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////     FUNCTIONS FOR RTC SETUP //////////////////////////////////////
//////////////   get Time-Date from PC /////////////////////
bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
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
/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////            Skip HTTP headers so that we are at the beginning of the response's body //////////////////////////////////////////////////////////
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println("No response or invalid response!");
  }

  return ok;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////        Open connection to the HTTP server  ////////////////////////////////////////////////////////////////////////////////
bool connect() {
  Serial.print("Connecting to Server... ");

  bool ok = client.connect(server, port);

  Serial.println(ok ? "Connected!" : "Connection Failed!");
  return ok;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////      Send the HTTP GET request to the server    ////////////////////////////////////////////////////////////////////
bool sendRequest(const char* resource) {
  Serial.println("Sending Setup-Get Request ......");
  // Make a HTTP request:
  client.print("GET /FarmCloud");
  client.print(resource);
  client.println(" HTTP/1.1");
  client.println("Host: 78.46.70.93");
  client.println("Connection: close");
  client.println();

  return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////      Read the body of the response from the HTTP server //////////////////////////////////////////////////////
void readReponseContent(char* content, size_t maxSize) {
  size_t length = client.readBytes(content, maxSize);
  content[length] = 0;
  Serial.println(content);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////          Parse data from Json to STRUCT /////////////////////////////////////////////////////////////////////////////////
bool parseUserData(char* content, struct AlarmData* alarmData) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // This is only required if you use StaticJsonBuffer.
  const size_t BUFFER_SIZE = JSON_OBJECT_SIZE(6);

  // Allocate a temporary memory pool on the stack
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  // If the memory pool is too big for the stack, use this instead:
  // DynamicJsonBuffer jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(content);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  // Here were copy the strings we're interested in
  alarmData->frMinutes = root["frMinutes"];
  alarmData->frHours = root["frHours"];
  alarmData->toMinutes = root["toMinutes"];
  alarmData->toHours = root["toHours"];
  return true;
}

bool parseMeasIrrigationData(char* content, struct measuringWateringFlags recordFlag[3]) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // This is only required if you use StaticJsonBuffer.
  // const size_t BUFFER_SIZE = JSON_OBJECT_SIZE(1)+JSON_OBJECT_SIZE(3);

  // Allocate a temporary memory pool on the stack
  StaticJsonBuffer<1100> jsonBuffer;
  // If the memory pool is too big for the stack, use this instead:
  // DynamicJsonBuffer jsonBuffer;




  JsonObject& root = jsonBuffer.parseObject(content);

  //JsonArray& root = jsonBuffer.parseArray(content);


  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }
  root.prettyPrintTo(Serial);

  for (int i = 0; i < 3; i++) {
    String ident = root["Data"][i]["identifier"];
    String irri = root["Data"][i]["irrigation"];
    String measu = root["Data"][i]["measurement"];
    int fh = root["Data"][i]["untilHour"];
    int fm = root["Data"][i]["untilMoment"];

    recordFlag[i].identifier = ident;
    recordFlag[i].irrigation = irri;
    recordFlag[i].measurement = measu;
    recordFlag[i].untilHour = fh;
    recordFlag[i].untilMoment = fm;
  }
  return true;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////            Print the data extracted from the JSON   //////////////////////////////////////////////////////////
void printUserData(const struct AlarmData* alarmData) {
  Serial.print("FROM Hours = ");
  Serial.println(alarmData->frHours);
  Serial.print("FROM Seconds = ");
  Serial.println(alarmData->frMinutes);
  Serial.print("TO Hours = ");
  Serial.println(alarmData->toHours);
  Serial.print("TO Seconds = ");
  Serial.println(alarmData->toMinutes);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////          MAIN ALARM FOR WAKE UP END DEVICES AND TAKE MEASURES  ////////////////////////////////////////////////////////////
void WakeUpAndTakeMeasures() {

  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year());
  Serial.print("\t");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.println("..........Wake Up End Devices And Take Measures..........");
  uint32_t DestAddress1 = {0X40E7CC39};
  uint32_t DestAddress2 = {0X40D6A2C9};
  uint32_t DestAddress3 = {0X40D6A2CF};


  if (DeviceStart(DestAddress1)) {
    dev1flag = true;
  } else {
    dev1flag = false;
  }
  unsigned long kathisterisi1 = millis();
  while (millis() - kathisterisi1 < 8000) {}
  if (DeviceStart(DestAddress2)) {
    dev2flag = true;
  } else {
    dev2flag = false;
  }
  unsigned long kathisterisi = millis();
  while (millis() - kathisterisi < 8000) {}

  if (DeviceStart(DestAddress3)) {
    dev3flag = true;
  } else {
    dev3flag = false;
  }

  Serial.println("DATA TO SEND");
  Serial.println(record[0].zbaddress);
  Serial.println(record[1].zbaddress);
  Serial.println(record[2].zbaddress);
  Serial.println("--------------------------------");

  /////////  TO DO ----   CHANGE TO DYNAMIC BUFFER -----------------------
  StaticJsonBuffer<1300> jsonBuffer;
  StaticJsonBuffer<1300> jsonBufferNes;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& end1dev = root.createNestedArray("embeddedDataList");

#define ARRAYSIZE 4
  String results[ARRAYSIZE] = { "humidity", "itemp", "wtemp", "soil" };


  for (int k = 0; k < 3; k++) {
    for (int j = 0; j < ARRAYSIZE; j++) {
      JsonObject& nestedroot = jsonBufferNes.createObject();
      nestedroot["datetime"] = record[k].Dtime;
      nestedroot["zbAddress"] = record[k].zbaddress;
      nestedroot["observableProperty"] = results[j];
      if (j == 0) {
        nestedroot["measureValue"] =  record[k].Shumidity;
        nestedroot["obsid"] =  1;
        nestedroot["obspropid"] = 21;
      } else if (j == 1) {
        nestedroot["measureValue"] = record[k].itemp;
        nestedroot["obsid"] =  2;
        nestedroot["obspropid"] = 7;
      } else if (j == 2) {
        nestedroot["measureValue"] = record[k].wtemp;
        nestedroot["obsid"] =  3;
        nestedroot["obspropid"] = 7;
      } else { //j=3
        nestedroot["measureValue"] = record[k].soilm;
        nestedroot["obsid"] =  4;
        nestedroot["obspropid"] = 21;
      }

      end1dev.add(nestedroot);

      //    nestedroot["humidity"] = record[k].Shumidity;
      //    nestedroot["itemp"] = record[k].itemp;
      //    nestedroot["wtemp"] = record[k].wtemp;
      //    nestedroot["soil"] = record[k].soilm;

    }
  }

  root.prettyPrintTo(Serial);


  if (client.connect(server, port)) {
    Serial.println("connected...Sending Post...");
    client.println("POST /FarmCloud/embedded/ HTTP/1.0");
    client.println("Host: 78.46.70.93");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/json;charset=UTF-8");
    client.print("Content-Length: ");
    client.println(root.measureLength());
    client.println();

    root.printTo(client);

  } else
  {
    Serial.println("NOT CONECTED");
  }

  Serial.println("Disconnect");
  client.stop();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////  Start End Device Process  //////////////////////////////////////////////////
boolean DeviceStart(uint32_t DestAddress1) {
  boolean flagPotDev;
  Serial.print("Starting End Device Process");
  previousMillis = millis();
  WakeUpEndDevice(DestAddress1);
  // Serial.println(wakedup);
  // Serial.println(previousMillis);
  String SensorsData;
  //  Serial.println(wakedup);
  if (wakedup) {

    while (millis() - previousMillis < 5000) {}
    Serial.println("Reading End Device Sensor Data...");
    SensorsData = ReadEndDeviceData();
    //  Serial.print("RETURNED :");
    //   Serial.println(SensorsData);

    if (SensorsData == "M") {
      wakedup = false;
      flagPotDev = false;
      /// MODEM STATUS
    } else if (SensorsData == "F") {
      wakedup = false;
      flagPotDev = false;
    } else if (SensorsData == "O") {
      wakedup = false;
      flagPotDev = false;
    } else {

      int commaIndex = SensorsData.indexOf(',');
      int secondcommaIndex = SensorsData.indexOf(',', commaIndex + 1);
      int thirdcommaIndex = SensorsData.indexOf(',', secondcommaIndex + 1);

      String firstValue = SensorsData.substring(0, commaIndex);
      String secondValue = SensorsData.substring(commaIndex + 1, secondcommaIndex);
      String thirdValue = SensorsData.substring(secondcommaIndex + 1, thirdcommaIndex);
      String fourthValue = SensorsData.substring(thirdcommaIndex + 1);

      float humidity = firstValue.toFloat();
      float Itemp = secondValue.toFloat();
      float Wtemp = thirdValue.toFloat();
      int soil = fourthValue.toInt();


      Serial.println("-----MEASURES GET FROM END DEVICE------");
      Serial.print("END DEVICE ADDRESS :");
      Serial.println(DestAddress1, HEX);
      Serial.print("HUMIDITY :");
      Serial.println(humidity);
      Serial.print("Int Temperature :");
      Serial.println(Itemp);
      Serial.print("WaterProof Temp :");
      Serial.println(Wtemp);
      Serial.print("SOIL :");
      Serial.println(soil);
      Serial.println("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

      String strAddr(DestAddress1, HEX);

      strAddr.toUpperCase();
      Serial.println(strAddr);

      String minutestr = "";
      if (minute() < 10) {
        minutestr = "0" + String(minute());
      }
      else {
        minutestr = String(minute());
      }

      String dtime = String(String(year()) + "-" + String(month()) + "-" + String(day()) + " " + String(hour()) + ":" + minutestr + ":00");

      Serial.print("COUNTERRRR:");
      Serial.println(Counter);

      record[Counter].Dtime = dtime;
      record[Counter].zbaddress = strAddr;
      record[Counter].Shumidity = humidity;
      record[Counter].itemp = Itemp;
      record[Counter].wtemp = Wtemp;
      record[Counter].soilm = soil;

      if (Counter == 2) {
        Counter = 0;
      } else {
        Counter = Counter + 1;
      }


      if (soil > 3000) {
        flagPotDev = true;
      } else {
        flagPotDev = false;
      }
      wakedup = false;
    }

    Serial.println("End Parse Data");

  } else {
    SensorsData = "EF";
    flagPotDev = false;

    Serial.println("EF");
    ///send debug string for fail
  }

  Serial.println("End Device Start");


  return flagPotDev;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// WAKE UP END DEVICE //////////////////////////////////////////////////////////
void WakeUpEndDevice(uint32_t DestAddress) {
  //  previousMillis = millis();

  Serial.println("Waking Up End Device...");
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
    // Serial.print(" ApiID:");
    //  Serial.println(xbee.getResponse().getApiId());
    // should be a znet tx status
    if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
      //     Serial.print("ZB_TX_STATUS_RESPONSE ApiID:");
      //    Serial.println(xbee.getResponse().getApiId());
      xbee.getResponse().getZBTxStatusResponse(txStatus);

      // get the delivery status, the fifth byte
      if (txStatus.getDeliveryStatus() == SUCCESS) {
        //    Serial.println("END DEVICE GOT THE MESSAGE");
        wakedup = true;
        //        unsigned long currentMillis = millis();
        //        Serial.print("Send packet data FOR WAKE UP: "); //360 sec
        //        Serial.print(currentMillis - previousMillis); //360 sec
        //        Serial.println(" msec"); //360 sec
      } else {
        // the remote XBee did not receive our packet. is it powered on?
        //    Serial.println("END DEVICE DID NOT GET THE MESSAGE");
      }
    }
  } else if (xbee.getResponse().isError()) {

    //   Serial.print("ERROR READING PACKET.ERROR CODE:");
    //   Serial.println(xbee.getResponse().getErrorCode());

    //nss.print("Error reading packet.  Error code: ");
    //nss.println(xbee.getResponse().getErrorCode());
  } else {
    // local XBee did not provide a timely TX Status Response -- should not happen
    //  Serial.println("local XBee did not provide a timely TX Status Response -- should not happen");
  }

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////   Read End Device Data ///////////////////////////////////////////////////

String ReadEndDeviceData() {

  // previousMillis = millis();
  String Sample;
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
        Sample += (char)rx.getData(i);
      }
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
      Sample = "M";

    } else {

      Sample = "O";
    }
  } else if (xbee.getResponse().isError()) {
    //  Serial.println("ERROR READING PACKET");
    Sample = "F";

  }
  return  Sample;

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



