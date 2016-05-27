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

// APN information obrained from your network provider
#define GPRS_APN       "internet.vodafone.gr" // replace with your GPRS APN
#define GPRS_LOGIN     ""    // replace with your GPRS login
#define GPRS_PASSWORD  "" // replace with your GPRS password

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
int MaxSoil = 500;

//////////////SERVER INFO ////////////////
char server[] = "78.46.70.93"; // the base URL
int port = 8080; // the port, 80 for HTTP
union ZBAddressConverter {
  uint32_t ZBInteger;
  uint8_t ZBArray[4];
};

void setup() {
  bool parse = false;
  bool config = false;
  Serial.begin(9600);
  Serial1.begin(9600);
  xbee.begin(Serial1);
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

  //  // connection state
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


  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["zbaddress"] ="40E7CC39"
  root["humidity"] = humidity;
  root["itemp"] = Itemp;
  root["wtemp"] = Wtemp;
  root["soil"] = soil;
  root["mdate"]="24/5/2016";

  if (client.connect(server, port)) {
    Serial.println("connected");
    client.println("POST /WebStart/embedded HTTP/1.0");
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




  pinMode(Valve1pin, OUTPUT);
  pinMode(Valve2pin, OUTPUT);
  pinMode(Valve3pin, OUTPUT);

  onAlarm = Alarm.alarmOnce(16, 57, 0, WakeUpAndTakeMeasures);

}
void loop() {

  Alarm.delay(1000);
  ////Na FTIAKSW TO flag gia 2 wres
  if (flag2wres) {
    Serial.println("2 Hours Flag/9am");
    // Serial.println(flag2wres);
    int wres;
    int lepta;
    int deftera;
    if (hour() >= 9) {
      Serial.println("9 AM");
      wres = 19;
      lepta = 0;
      deftera = 0;
    } else if (wres = 23) {
      wres = 1;
      lepta = 0;
      deftera = 0;

    } else if (wres = 24) {
      wres = 2;
      lepta = 0;
      deftera = 0;

    } else {
      wres = hour() + 2;
      lepta = 0;
      deftera = 0;
    }

    Serial.print("Take Measures at ");
    Serial.print(wres);
    Serial.print(" Hours ");
    Serial.print(lepta);
    Serial.println(" Minutes ");

    Alarm.alarmOnce(wres, lepta, deftera, WakeUpAndTakeMeasures);
    flag2wres = false;
  }
  if (flag10lepta) {
    Serial.println("10 minutes Check");
    //  Serial.println(flag10lepta);
    int wres = hour();
    int lepta = minute();
    int deftera = 0;

    if (lepta >= 50) {

      wres = wres + 1;
      lepta = lepta - 50;
    } else {
      lepta = lepta + 10;
    }

    Serial.print("Take Measures at ");
    Serial.print(wres);
    Serial.print(" : ");
    Serial.println(lepta);


    Alarm.alarmOnce(wres, lepta, deftera, WakeUpAndTakeMeasures);
    Serial.println("..........Process Completed..........");
    flag10lepta = false;
  }

  if (dev1flag) {
    ////////potise device 1
    digitalWrite(Valve1pin, HIGH);
  } else {
    digitalWrite(Valve1pin, LOW);
  }
  if (dev2flag) {
    ////////potise device 2
    digitalWrite(Valve2pin, HIGH);
  } else {
    digitalWrite(Valve2pin, LOW);
  }
  if (dev3flag) {
    ////////potise device 3
    digitalWrite(Valve3pin, HIGH);
  } else {
    digitalWrite(Valve3pin, LOW);
  }

}

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

  //  unsigned long paredwseFlag = millis();

  if (Dev(DestAddress1)) {
    dev1flag = true;
  } else {
    dev1flag = false;
  }
  unsigned long kathisterisi1 = millis();
  while (millis() - kathisterisi1 < 7000) {}
  if (Dev(DestAddress2)) {
    dev2flag = true;
  } else {
    dev2flag = false;
  }
  unsigned long kathisterisi = millis();
  while (millis() - kathisterisi < 7000) {}

  if (Dev(DestAddress3)) {
    dev3flag = true;
  } else {
    dev3flag = false;
  }

  //////////////GENERIC FLAGS FROM DATABASE /////////////////////////////////
  if (dev3flag || dev2flag || dev1flag) {
    flag10lepta = true;
  } else {
    flag2wres = true;
  }

}

boolean Dev(uint32_t DestAddress1) {
  boolean flagPotDev;

  previousMillis = millis();
  WakeUpEndDevice(DestAddress1);
  // Serial.println(wakedup);
  // Serial.println(previousMillis);
  String SensorsData;
  Serial.println(wakedup);
  if (wakedup) {

    while (millis() - previousMillis < 5000) {}
    Serial.println("Reading End Device Sensor Data...");
    SensorsData = ReadEndDeviceData();
    //  Serial.print("RETURNED :");
    //   Serial.println(SensorsData);

    if (SensorsData == "M") {

      /// MODEM STATUS

    } else if (SensorsData == "F") {
      wakedup = false;
      flagPotDev = false;
    } else if (SensorsData == "O") {
      wakedup = false;
      flagPotDev = false;

      //  Serial.println("eeeeee");
    } else {
      //  Serial.print("RETURNED111 :");
      //   Serial.println(SensorsData);

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




      Serial.println("-----------");
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
      Serial.println("@@@@@@@@@@@@@");


      ///   SEND VIA POST TO SERVER ///////////////////////////////////////////
      //////  PREPARE JSON ////////////////////////////////

      String str(DestAddress1, HEX);

      Serial.println(str);

      //      StaticJsonBuffer<200> jsonBuffer;
      //      JsonObject& root = jsonBuffer.createObject();
      //      root[""] =
      //      root["humidity"] = humidity;
      //      root["itemp"] = Itemp;
      //      root["wtemp"] = Wtemp;
      //      root["soil"] = soil;
      //
      //      if (client.connect(server, port)) {
      //        Serial.println("connected");
      //        client.println("POST /WebStart/embedded HTTP/1.0");
      //        client.println("Host: 78.46.70.93");
      //        client.println("User-Agent: Arduino/1.0");
      //        client.println("Connection: close");
      //        client.println("Content-Type: application/json;charset=UTF-8");
      //        client.print("Content-Length: ");
      //        client.println(root.measureLength());
      //        client.println();
      //
      //        root.printTo(client);
      //
      //      } else
      //      {
      //        Serial.println("NOT CONECTED");
      //      }




      if (soil > 3000) {
        flagPotDev = true;
      } else {
        flagPotDev = false;
      }
      wakedup = false;
    }



  } else {
    SensorsData = "EF";
    flagPotDev = false;
    ///send debug string for fail
  }


  return flagPotDev;
}

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

      //   Serial.print("DATA SENSORS");
      //   Serial.println(Sample);



      //      unsigned long currentMillis = millis();
      //      Serial.print("SENSORS DATA FROM END DEVICE: "); //360 sec
      //      Serial.print(currentMillis - previousMillis); //360 sec
      //      Serial.println(" msec"); //360 sec





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
  //  Serial.print("APO THN READ END DEVICE FUNCTION");
  //  Serial.println(Sample);
  return  Sample;

}

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




