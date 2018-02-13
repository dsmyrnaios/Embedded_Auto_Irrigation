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
  boolean dataErrorFlag;
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
  char* PINNUMBER      = (char*)"0276";                      //TODO on keypad 8492
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
  String DtimeIrrigation;  
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
const long userRequestInterval          = 120000;   //interval for getting data from user 2 mins in milliseconds
const long userRequestAlgorithmInterval = 500000;   //interval for getting data for automatic algorithm 12 hours in milliseconds
const long damageInterval               = 1800000;  //interval for autoamtic watering on damage



//////////////////////--------------- Keypad shield const values ------------------//////////////////////
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int blinkX = 0;
int blinkY = 0;



//////////////////////////////////////////
const byte Rows= 4; //number of rows on the keypad i.e. 4
const byte Cols= 3; //number of columns on the keypad i,e, 3

//we will definne the key map as on the key pad:

char keymap[Rows][Cols]=
{
{'1', '2', '3'},
{'4', '5', '6'},
{'7', '8', '9'},
{'*', '0', '#'}
};

//  a char array is defined as it can be seen on the above


//keypad connections to the arduino terminals is given as:

byte rPins[Rows]= {34,35,36,37}; //Rows 0 to 3
byte cPins[Cols]= {38,39,40}; //Columns 0 to 2

//////////////////////--------------- APN information obrained from your network provider------------------//////////////////////
//#define PINNUMBER      "8492" //TODO on keypad
//#define GPRS_APN       "internet.vodafone.gr" // replace with your GPRS APN
//#define GPRS_LOGIN     ""    // replace with your GPRS login
//#define GPRS_PASSWORD  "" // replace with your GPRS password
