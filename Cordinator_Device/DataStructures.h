// The type of data that we want to extract from the page
struct AlarmData {
  int frMinutes;
  int frHours;
  int toMinutes;
  int toHours;
};

/////////Send data post struct //////////////////////
//////////////////////////////////////////////////////
struct record_data {
  String Dtime;
  String zbaddress;
  float Shumidity;
  float itemp;
  float wtemp;
  int soilm;
};


struct measuringWateringFlags {
  uint32_t zbAddress;
  boolean irrigation;
  boolean measurement;
  boolean startIrrigationFlag = false;
  int fromHour;
  int fromminute;
  int untilHour;
  int untilMinute;
};

struct GsmData {
  char* PINNUMBER      = (char*)"8492";                  //TODO on keypad
  char* GPRS_APN       = (char*)"internet.vodafone.gr";  // replace with your GPRS APN
  char* GPRS_LOGIN     = (char*)"";                      // replace with your GPRS login
  char* GPRS_PASSWORD  = (char*)"";                      // replace with your GPRS password
  char* server         = (char*)"78.46.70.93";           // the base URL
  int port             = 8080;                           // the port, 80 for HTTP  
};

//typedef union {
//  uint32_t dword;
//  uint16_t word[2];
//  uint8_t  byte[4];
//} address ;

struct endDevice {  
  uint32_t zigbeeaddress;
  //char* zigbeeCharAddress;
  float mintemp;
  float maxtemp;
  float minhumidity;
  float maxhumidity;
  boolean measureflag;
  boolean wateringflag;   
  boolean devflag;
  int valvePin;
  char* Dtime;
};


const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char *results[4] = {"humidity", "itemp", "wtemp", "soil"};


const unsigned int devicedelay   = 8000;   // device delay for DeviceStart?
const unsigned int minpinNumber  = 23;     // min valve pint for the end devices
const unsigned int maxpinNumber  = 30;     // max valve pint for the end devices
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server
const size_t MAX_CONTENT_SIZE    = 512;    // max size of the HTTP response
const int MAX_DEVICES_INPUT      = 10;     //max size for enddevices
const int ARRAYSIZE = 4;
const long wateringInterval = 120000;      // interval at which to water (milliseconds)



//////////////////////--------------- APN information obrained from your network provider------------------//////////////////////
//#define PINNUMBER      "8492" //TODO on keypad
//#define GPRS_APN       "internet.vodafone.gr" // replace with your GPRS APN
//#define GPRS_LOGIN     ""    // replace with your GPRS login
//#define GPRS_PASSWORD  "" // replace with your GPRS password
