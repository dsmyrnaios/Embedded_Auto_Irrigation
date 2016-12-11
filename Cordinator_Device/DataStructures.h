// Automated Algorithm Period-Time(Alarm Library)
struct AlarmData {
  int frMinutes;
  int frHours;
  int toMinutes;
  int toHours;
};

/////////End Devices Measurements Send To Server //////////////////////
//////////////////////////////////////////////////////
struct record_data {
  String Dtime;
  String zbaddress;
  float Shumidity;
  float itemp;
  float wtemp;
  int soilm;
};

// Struct stores USER Data Request TO Embeeded //////////
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
  char* PINNUMBER      = (char*)"";                      //TODO on keypad 8492
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

/// Struct for AUTOMATED ALGORITHM ///////
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
  char* Dtime[19];  
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
const int MAX_DEVICES_INPUT      = 10;     // max size for enddevices
const int ARRAYSIZE = 4;
const long userRequestInterval          = 300000;    // interval getting data from user 2 mins in milliseconds
const long userRequestAlgorithmInterval = 120000; //43200000; // interval getting data for automatic algorithm 12 hours in milliseconds

//////////////////////--------------- Keypad shield const values ------------------//////////////////////
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int blinkX = 0;
int blinkY = 0;

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//////////////////////--------------- APN information obrained from your network provider------------------//////////////////////
//#define PINNUMBER      "8492" //TODO on keypad
//#define GPRS_APN       "internet.vodafone.gr" // replace with your GPRS APN
//#define GPRS_LOGIN     ""    // replace with your GPRS login
//#define GPRS_PASSWORD  "" // replace with your GPRS password
