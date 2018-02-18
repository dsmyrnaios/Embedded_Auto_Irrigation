
#include <XBee.h>
#include <DHT.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <SoftwareSerial.h>


#define DHTTYPE DHT22 //DHT11, DHT21, DHT22
#define DHTPIN 4     // pin6 DIGITAL HUMIDITY-TEMPERATURE PIN

///////  Sensors Power Pins ///////////
int powerDigitalHumidityPin = 7; //pin13
int powerWaterProofPin = 8; //pin14
int powerSoilHumidity = 9; //pin14

//////   Sensors Data Pins ///////////
int dataWaterProofPin = A0; // pin23//////////  WATERPROOF ANALOG SENSOR TEMPERATURE
int dataSoilHumidityPin = A2;   //pin25/////////   ANALOG SOIL HUMIDITY


/*/////////////////////////////////////////////////////// BYTE ARRAY SENSOR DATA //////////////////////////////////////////////////////////////
  ////////////////////////////  Internal Humidity       Internal Temperature         WaterProof Temperature     Soil             //////////////
  ///////////////////////////     [0][1][2][3]            [4][5][6][7]                   [8][9][10][11]         [12][13]        ///////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse(); /////////RX GET DATA
ModemStatusResponse msr = ModemStatusResponse();////////////TX STATUS RESPONSE
DHT dht(DHTPIN, DHTTYPE);

/// Software Serial Pins for Zigbee S2 Use ////
uint8_t ssRX = 10;
uint8_t ssTX = 11;
SoftwareSerial nss(ssRX, ssTX);


/////////////////////////////////////////////////////  SETUP /////////////////////////////////////////////////////
void setup() {
  //////////   
  pinMode(A0, INPUT);   ///////////  WATERPROOF ANALOG SENSOR TEMPERATURE
  pinMode(A2, INPUT);   //////////   ANALOG SOIL HUMIDITY
  pinMode(2, INPUT); //Rx Interupt from Zigbee
  pinMode(powerDigitalHumidityPin, OUTPUT); 
  pinMode(powerWaterProofPin, OUTPUT); 
  pinMode(powerSoilHumidity, OUTPUT); 
  
  ////////ZIGBEE INIT
  dht.begin();         ///////DHT-22 DIGITAL SENSOR INIT
  Serial.begin(9600);
  nss.begin(9600);
  xbee.begin(nss);
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


//////////////////////////////////////////////////////   LOOP   ///////////////////////////////////////////////////////////

int timesOfWakeUp = 0;
void loop() {
  digitalWrite(powerDigitalHumidityPin, HIGH);
  digitalWrite(powerWaterProofPin, HIGH);
  digitalWrite(powerSoilHumidity, HIGH);  
  
  unsigned long previousMillis = millis();
  Serial.println("Reading Sensors...");
  Serial.println("---------------------------");
  String SData = readSensors();
  Serial.println("");
  Serial.print("DATA SENSORS STRING :");
  Serial.println(SData);

  digitalWrite(powerDigitalHumidityPin, LOW);
  digitalWrite(powerWaterProofPin, LOW);
  digitalWrite(powerSoilHumidity, LOW);  

  SendMeasures(SData);
  timesOfWakeUp++;
  Serial.print("Awake: ");
  Serial.print(timesOfWakeUp);
  Serial.print(" times ");
  Serial.print(" for ");
  Serial.print(millis() - previousMillis);
  Serial.println(" sec");
  Serial.println("/---------------------------/");
  delay(500);
  
  enterSleep();
}

//////////////////////////////////////  READ SENSORS //////////////////////////////////////////////////////////////////////////////////////////
String readSensors() {

  // previousMillis = millis();
  /////////////////////////////////////////////////
  delay(2000); // wait for 2 seconds

  //////////////////////////////////////Digital Humidity-Temperature/////////////////////
  //Read Humidity
  float digitalHumidity  = dht.readHumidity();

  // Read temperature as Celsius
  float digitalTemperature = dht.readTemperature();


  // Check if any reads failed and exit early (to try again).
  if (isnan(digitalHumidity) || isnan(digitalTemperature)) {
    Serial.println("Failed to read from DHT sensor!");
    digitalHumidity = 0;
    digitalTemperature = 0;
  }else{
    Serial.print("Humidity: ");
    Serial.print(digitalHumidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(digitalTemperature);
    Serial.println(" *C ");
  }
  

  /////////////////////////////WaterProof Analog Temperature//////////

  delay(1000);
  int waterProof = analogRead(dataWaterProofPin);
  delay(10);
  waterProof = analogRead(dataWaterProofPin);
  delay(10);
  float waterProofTemperatureC  = ( 5.0 * waterProof * 100.0) / 1024.0 ;  //converting from 10 mv per degree wit 500 mV offset

  //  //to degrees ((voltage - 500mV) times 100)
  //  Serial.print("Voltage: ");
  //  Serial.println(oo * 5.0 / 1023.0);

  if (isnan(waterProofTemperatureC)) {
    Serial.println("Failed to read from WaterProof Sensor");
    waterProofTemperatureC = 0;    
  }else{
    Serial.print("WaterProof Temperature : ");
    Serial.print(waterProofTemperatureC);
    Serial.println(" Degrees C");
  }  


  /////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////Soil Humidity/////////////////////
  //
  delay(1000);
  int soilHumidity = analogRead(dataSoilHumidityPin);
  delay(10);
  soilHumidity = analogRead(dataSoilHumidityPin);
  delay(10);
  if (isnan(soilHumidity)) {
    Serial.println("Failed to read from Soil Humidity Sensor");
    soilHumidity = 0;    
  }else{
    Serial.print("Soil Humidity ANA: ");
    Serial.println(soilHumidity);
  } 
  

  //
  //
  //   /* constraint function will limit the values we get so we can work better with map
  // * since I only need values in the bottom limit of 300, will make it the outer limit and 1023 the other limit
  // */
  //  temp = constrain (temp, 300, 1023);
  // print the values returned by the sensor
  //Serial.println(sensorValue);
  // create a map with the values
  // You can think we have the 100% to 300 and 0 % to 1023 wrong, but no !
  // 300 - or 100 % - is the target value for moisture value in the soil
  // 0% of humidity - or moisture - is when the soil is dry
  // temp = map (temp, 300, 1023, 100, 0);
  //Serial.println(temp);

  /////////////////////////////////////////////////////////

  //  unsigned long currentMillis = millis();
  //  Serial.print("Read SENSORS: "); //360 sec
  //  Serial.print(currentMillis - previousMillis); //360 sec
  //  Serial.println(" msec"); //360 sec

  return String(String(digitalHumidity) + "," + String(digitalTemperature) + "," + String(waterProofTemperatureC) + "," + String(soilHumidity));

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


///////////////////////////////////////////////  SLEEP NOW /////////////////////////////////////////////////////////////////////
void enterSleep(void)
{
  Serial.println("Sleeping Mode");
  /* Setup pin2 as an interrupt and attach handler. */
  attachInterrupt(0, pin2Interrupt, LOW);
  delay(100);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  sleep_enable();

  sleep_mode();

  /* The program will continue from here. */

  /* First thing to do is disable sleep. */
  sleep_disable();
  Serial.println("Waking up");
}
////////////////////////////////////////////////// PIN2 INTERRUPT   /////////////////////////////////////////////////////////////////////////
void pin2Interrupt(void)
{
  /* This will bring us back from sleep. */

  /* We detach the interrupt to stop it from
     continuously firing while the interrupt pin
     is low.
  */




  detachInterrupt(0);
}
////////////////////////////////////////////////// SEND MEASURES     ////////////////////////////////////////////////////////////////////

void SendMeasures(String Measures) {

  // previousMillis = millis();

  byte byteDataSend[Measures.length() + 1];
  Measures.getBytes(byteDataSend, Measures.length() + 1);

  //////// Convert String to byteArray

  XBeeAddress64 addr64 = XBeeAddress64();
  addr64.setMsb(0X0013A200);//XXXXX -> Msb address of router/end node
  addr64.setLsb(0X40E7CC41);//XXXXX -> Lsb address of router/end node
  ZBTxRequest zbTx = ZBTxRequest(addr64, byteDataSend, sizeof(byteDataSend));
  ZBTxStatusResponse txStatus = ZBTxStatusResponse();
  xbee.send(zbTx);

  // after sending a tx request, we expect a status response
  // wait up to half second for the status response
//  if (xbee.readPacket(500)) {
//    // got a response!
//    //  Serial.print(" ApiID:");
//    //  Serial.println(xbee.getResponse().getApiId());
//    // should be a znet tx status
//    if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
//      // Serial.print("ZB_TX_STATUS_RESPONSE ApiID:");
//      //  Serial.println(xbee.getResponse().getApiId());
//      xbee.getResponse().getZBTxStatusResponse(txStatus);
//
//      //  get the delivery status, the fifth byte
//      if (txStatus.getDeliveryStatus() == SUCCESS) {
//        Serial.println("");
//        Serial.println("COORDINATOR GOT THE MESSAGE");
//
//        //        unsigned long currentMillis = millis();
//        //
//        //        Serial.print("Send packet DATA SENSORS: "); //360 sec
//        //        Serial.print(currentMillis - previousMillis); //360 sec
//        //        Serial.println(" msec"); //360 sec
//      } else {
//        // the remote XBee did not receive our packet. is it powered on?
//        Serial.println("COORDINATOR DID NOT GET THE MESSAGE");
//      }
//    }
//  } else if (xbee.getResponse().isError()) {
//    Serial.print("ERROR READING PACKET.ERROR CODE:");
//    Serial.println(xbee.getResponse().getErrorCode());
//
//    //Serial.print("Error reading packet.  Error code: ");
//    //Serial.println(xbee.getResponse().getErrorCode());
//  } else {
//    // local XBee did not provide a timely TX Status Response -- should not happen
//    Serial.println("local XBee did not provide a timely TX Status Response -- should not happen");
//  }

  delay(500);

}




