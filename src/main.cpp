/********************
Sept. 2014 Rui Azevedo - ruihfazevedo(@rrob@)gmail.com

menu output to standard arduino LCD (LiquidCrystal)
output: LCD
input: encoder and Serial
www.r-site.net
***/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <menu.h>
// #include <menuIO/liquidCrystalOut.h>
#include <menuIO/serialOut.h>
#include <menuIO/TFT_eSPIOut.h>
// #include <menuIO/utftOut.h>
#include <menuIO/chainStream.h>
// #include <menuIO/urtouchIn.h>
#include <menuIO/serialIn.h>
#include <menuIO/keyIn.h>

#include "TFT_eSPI_touchIn.h"

#define LEDPIN LED_BUILTIN
#define MAX_DEPTH 2

#define CURRENT_INPUT_PIN A6
#define VOLTAGE_INPUT_PIN A5

const float reference_voltage = 3.11;
float calibration_voltage=0.39;

unsigned long last_measurement_time = 0;

#define Black RGB565(0,0,0)
#define Red	RGB565(255,0,0)
#define Green RGB565(0,255,0)
#define Blue RGB565(0,0,255)
#define Gray RGB565(128,128,128)
#define LighterRed RGB565(255,150,150)
#define LighterGreen RGB565(150,255,150)
#define LighterBlue RGB565(150,150,255)
#define DarkerRed RGB565(150,0,0)
#define DarkerGreen RGB565(0,150,0)
#define DarkerBlue RGB565(0,0,150)
#define Cyan RGB565(0,255,255)
#define Magenta RGB565(255,0,255)
#define Yellow RGB565(255,255,0)
#define White RGB565(255,255,255)

const colorDef<uint16_t> colors[6] MEMMODE={
  {{(uint16_t)Black,(uint16_t)Black}, {(uint16_t)Black, (uint16_t)Blue,  (uint16_t)Blue}},//bgColor
  {{(uint16_t)Gray, (uint16_t)Gray},  {(uint16_t)White, (uint16_t)White, (uint16_t)White}},//fgColor
  {{(uint16_t)White,(uint16_t)Black}, {(uint16_t)Yellow,(uint16_t)Yellow,(uint16_t)Red}},//valColor
  {{(uint16_t)White,(uint16_t)Black}, {(uint16_t)White, (uint16_t)Yellow,(uint16_t)Yellow}},//unitColor
  {{(uint16_t)White,(uint16_t)Gray},  {(uint16_t)Black, (uint16_t)Blue,  (uint16_t)White}},//cursorColor
  {{(uint16_t)White,(uint16_t)Yellow},{(uint16_t)Blue,  (uint16_t)Red,   (uint16_t)Red}},//titleColor
};

struct Config {
    // char hostname[64];
    unsigned int n_calibrations;
    float max_voltage;
    float min_voltage;
    float max_current;
    float min_current;
    unsigned int measurement_delay;
    float sensitivity;
    float R1;
    float R2;
    int resolution;
};

const char *filename = "/config.txt";  // <- SD library uses 8.3 filenames
Config config_obj;                         // <- global configuration object

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
// UTFT tft();

// Loads the configuration from a file and sets default values
void loadConfiguration(const char *filename, Config &config_obj) {
    // Open file for reading
    File file = SD.open(filename);

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<512> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println(F("Failed to read file, using default configuration"));
    }

    // Copy values from the JsonDocument to the Config
    config_obj.n_calibrations = doc["n_calibrations"] | 100;
    config_obj.max_voltage = doc["max_voltage"] | 15;
    config_obj.min_voltage = doc["min_voltage"] | 13;
    config_obj.max_current = doc["max_current"] | 5;
    config_obj.min_current = doc["min_current"] | 4;
    config_obj.measurement_delay = doc["measurement_delay"] | 1000;
    config_obj.sensitivity = doc["sensitivity"] | 0.185;
    config_obj.R1 = doc["R1"] | 9950;
    config_obj.R2 = doc["R2"] | 2550;
    config_obj.resolution = doc["resolution"] | 4096;
    // strlcpy(config_obj.hostname,                  // <- destination
    //         doc["hostname"] | "example.com",  // <- source
    //         sizeof(config_obj.hostname));         // <- destination's capacity

    // Close the file (Curiously, File's destructor doesn't close the file)
    file.close();
}

// Saves the configuration to a file
void saveConfiguration(const char *filename, const Config &config_obj) {
  // Delete existing file, otherwise the configuration is appended to the file
  SD.remove(filename);

  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

    // Set the values in the document
    doc["n_calibrations"] = config_obj.n_calibrations;
    doc["max_voltage"] = config_obj.max_voltage;
    doc["min_voltage"] = config_obj.min_voltage;
    doc["max_current"] = config_obj.max_current;
    doc["min_current"] = config_obj.min_current;
    doc["measurement_delay"] = config_obj.measurement_delay;
    doc["sensitivity"] = config_obj.sensitivity;
    doc["R1"] = config_obj.R1;
    doc["R2"] = config_obj.R2;
    doc["resolution"] = config_obj.resolution;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}

// Prints the content of a file to the Serial
void printFile(const char *filename) {
  // Open file for reading
  File file = SD.open(filename);
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
}

// LCD /////////////////////////////////////////
// LiquidCrystal lcd(8, 7, 5, 6, 3, 2);  

unsigned int timeOn=1000;
unsigned int timeOff=1000;

result doMeasure(eventMask e, prompt &item);
result showEvent(eventMask e,navNode& nav,prompt& item);
result saveConfig();
result printConfig();

MENU(settingsMenu,"Settings",doNothing,anyEvent,noStyle
//   ,OP("Volts",showEvent,anyEvent)
//   ,OP("Amps",showEvent,anyEvent)
  ,FIELD(config_obj.max_voltage,"Max"," V",0,30,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(config_obj.min_voltage,"Min"," V",0,30,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(config_obj.max_current,"Max"," A",0,10,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(config_obj.min_current,"Min"," A",0,10,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(config_obj.measurement_delay,"Delay"," ms",100,60000,1000,100,doNothing,noEvent,noStyle)
  ,EXIT("<Back")
);

MENU(mainMenu, "Blink menu", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
  ,FIELD(timeOn,"On","ms",0,1000,10,1, Menu::doNothing, Menu::noEvent, Menu::noStyle)
  ,FIELD(timeOff,"Off","ms",0,10000,10,1,Menu::doNothing, Menu::noEvent, Menu::noStyle)
  ,OP("Measure",doMeasure,enterEvent)
  ,SUBMENU(settingsMenu)
  ,OP("Save Config",saveConfig,Menu::enterEvent)
  ,OP("Print Config",printConfig,Menu::enterEvent)
  ,EXIT("<Back")
);

// keyMap joystickBtn_map[]={
//  {-BTN_ENTER, defaultNavCodes[enterCmd].ch} ,
//  {-BTN_UP, defaultNavCodes[upCmd].ch} ,
//  {-BTN_DOWN, defaultNavCodes[downCmd].ch}  ,
//  {-BTN_BACK, defaultNavCodes[escCmd].ch},
// };
// keyIn<4> joystickBtns(joystickBtn_map);

#define textScale 6
#define defaultTextW 6
#define defaultTextH 9

const int charW = defaultTextW * textScale;
const int charH = defaultTextH * textScale;

const panel panels[] MEMMODE = {{0, 0, TFT_WIDTH / defaultTextW,TFT_HEIGHT / defaultTextH}};
navNode* nodes[sizeof(panels) / sizeof(panel)]; //navNodes to store navigation status
panelsList pList(panels, nodes, 1); //a list of panels and nodes
idx_t eSpiTops[MAX_DEPTH]={0};
TFT_eSPIOut eSpiOut(tft,colors,eSpiTops,pList,charW,charH+1);

idx_t serialTops[MAX_DEPTH]={0};
serialOut outSerial(Serial,serialTops);

MENU_OUTLIST(out,&eSpiOut,&outSerial);
// menuOut* constMEM outputs[] MEMMODE={&serial,&eSpiOut};//list of output devices
// outputsList out(outputs,sizeof(outputs)/sizeof(menuOut*));//outputs list controller

extern navRoot nav;
// URTouch utouch(TFT_SCLK, TOUCH_CS, TFT_MOSI, TFT_MISO, 2);
TFT_eSPI_touchIn touch(tft, nav, eSpiOut);
serialIn inSerial(Serial);
MENU_INPUTS(in,&inSerial,&touch/*&joystickBtns*/);

// MENU_OUTPUTS(out,MAX_DEPTH
//   ,SERIAL_OUT(Serial)
//   ,TFT_eSPI_OUT(tft,colors,charW,charH,{0,0,TFT_WIDTH / defaultTextW,TFT_HEIGHT / defaultTextH}) // {x, y, n, m} - x is padding on x axes, y is padding on y axes, n is number of drawned characters on x, m is number of menu items drawned
// //   ,LIQUIDCRYSTAL_OUT(lcd,{0,0,16,2})
// );

NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

void psu_init() {
    tft.println("Initializing....");
    Serial.println("Initializing....");
    int buffer[config_obj.n_calibrations];
    int value = 0;

    for (size_t i = 0; i < config_obj.n_calibrations; i++)
    {
        buffer[i] = analogRead(CURRENT_INPUT_PIN);
        value += buffer[i];
        delay(100);
    }
    
    float avg_value = value / (config_obj.n_calibrations - 1);

    calibration_voltage = reference_voltage / config_obj.resolution * avg_value;
    // tft.clear();
    Serial.println("Done!");
    delay(1000);
}

struct Quantity
{
    virtual float measure();
    virtual float get_value() const;
    virtual ~Quantity() = 0;
};

Quantity::~Quantity() {}

class Voltage : public Quantity
{
private:
    float value;
public:
    Voltage();
    ~Voltage() = default;
    float measure() override;
    float get_value() const override;
};

Voltage::Voltage(/* args */)
{
    value = this->measure();
}

float Voltage::measure() {
    float raw_voltage = analogRead(VOLTAGE_INPUT_PIN);
    float voltage = reference_voltage / config_obj.resolution * raw_voltage;
    return voltage * (config_obj.R1 + config_obj.R2) / config_obj.R2;
}

float Voltage::get_value() const {
    return value;
}

class Current : public Quantity
{
private:
    float value;
public:
    Current();
    ~Current() = default;
    float measure() override;
    float get_value() const override;
};

Current::Current()
{
    value = this->measure();
}

float Current::measure() {
    float raw_current_voltage = analogRead(CURRENT_INPUT_PIN);
    float current_voltage = reference_voltage / config_obj.resolution * raw_current_voltage;
    float current = (current_voltage - calibration_voltage) / config_obj.sensitivity;
    if (current > 0)
    {
        return current;
    }
    return 0;
}

float Current::get_value() const {
    return value;
}

class Power : public Quantity
{
private:
    float value = 0.0f;
public:
    Power() = default;
    ~Power();
    float measure() override;
    float get_value() const override;
};

// float Power::measure() {
//     if (value > 0)
//     {
//         return current;
//     }
//     return 0;
// }

// float Power::get_value() const {
//     return value;
// }


class Measurement
{
private:
    Voltage* voltage;
    Current* current;
public:
    Measurement();
    ~Measurement();
    Voltage* get_voltage() const;
    Current* get_current() const;
};

Measurement::Measurement()
{
    voltage = new Voltage();
    current = new Current();
}

Measurement::~Measurement()
{
    delete voltage;
    delete current;
}

Voltage* Measurement::get_voltage() const {
    return voltage;
}

Current* Measurement::get_current() const {
    return current;
}

result measure(menuOut& o,idleEvent e) {
    nav.idleChanged = true;

    if (millis() - last_measurement_time > config_obj.measurement_delay)
    {
        last_measurement_time = millis();
        Measurement measurement{};
        // o.clear();
        o.setCursor(0, 0);
        o.print(String(measurement.get_voltage()->get_value()) + " V");
        o.setCursor(0, 1);
        o.print(String(measurement.get_current()->get_value()) + " A");
    }
    return proceed;
}

result doMeasure(eventMask e, prompt &item) {
    nav.idleOn(measure);
    return proceed;
}

result saveConfig() {
    saveConfiguration(filename, config_obj);
    return proceed;
}

result printConfig() {
    Serial.println();
    Serial.println();
    printFile(filename);
    return proceed;
}

void setup() {
    pinMode(LEDPIN, OUTPUT);
    Serial.begin(9600);
    while(!Serial);

    // Initialize SD library

    // Nano, Uno = 4
    // Mega = 53
    // Due = 4, 10, 52
    const int chipSelect = 4;
    while (!SD.begin(chipSelect)) {
        Serial.println(F("Failed to initialize SD library"));
        delay(1000);
        break;
    }

    // Should load default config_obj if run for the first time
    Serial.println(F("Loading configuration..."));
    loadConfiguration(filename, config_obj);

    // Create configuration file
    Serial.println(F("Saving configuration..."));
    saveConfiguration(filename, config_obj);

    // Dump config_obj file
    Serial.println(F("Print config_obj file..."));
    printFile(filename);

    Serial.println("Menu 4.x");
    Serial.println("Use keys + - * /");
    Serial.println("to control the menu navigation");
    Serial.flush();
    // lcd.begin(16,2);
    //   joystickBtns.begin();

    tft.init();
    tft.setRotation(2);
    // gfx.setTextSize(textScale);//test scalling
    tft.setTextWrap(true);
    tft.setTextSize(4);
    tft.fillScreen(Black);
    tft.setTextColor(White, Black);

    // Use this calibration code in setup():
    uint16_t calData[5] = { 102, 3712, 67, 3863, 2 };
    tft.setTouch(calData);

    nav.showTitle = false;
    nav.idleTask = measure;
    psu_init();
    nav.idleOn(measure);//this menu will start on idle state, press select to enter menu
}

bool blink(int timeOn,int timeOff) {return millis()%(unsigned long)(timeOn+timeOff)<(unsigned long)timeOn;}

#define SOFT_DEBOUNCE_MS 100

uint16_t x, y;

void loop() {
    nav.poll();
    digitalWrite(LEDPIN, blink(timeOn,timeOff));
    // tft.getTouchRaw(&x, &y);
    // Serial.println("x: " + String(x) + ", y: " + String(y));
}