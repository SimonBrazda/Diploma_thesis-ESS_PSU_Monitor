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
#include <ClickEncoder.h>
// #include <RotaryEncoder.h>
#include <SAMDUETimerInterrupt.h>
// #include <streamFlow.h>
// #include <TimerOne.h>
#include <menuIO/clickEncoderIn.h>
#include <menuIO/serialIO.h>
#include <menuIO/chainStream.h>
#include <menuIO/TFT_eSPIOut.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#include <menuIO/keyIn.h>

#include "TFT_eSPI_touchIn.h"
#include "EEPROM_vars.h"

#define LEDPIN LED_BUILTIN
#define MAX_DEPTH 2

#define CURRENT_INPUT_PIN A6
#define VOLTAGE_INPUT_PIN A5

#define SOFT_DEBOUNCE_MS 100

unsigned long last_measurement_time = 0;

// Encoder /////////////////////////////////////
#define encA 25
#define encB 24
//this encoder has a button here
#define encBtn 22
#define encSteps 4

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
    uint8_t conf_in_eeprom = 0;
    unsigned int n_calibrations = 40;
    float max_voltage = 15.00F;
    float min_voltage = 11.50F;
    float max_current = 10.00F;
    float min_current = 0.10F;
    unsigned int measurement_delay = 1000;
    float sensitivity = 0.185F;
    unsigned int R1 = 180000;
    unsigned int R2 = 47000;
    unsigned short resolution = 4096;
    float reference_voltage = 3.30F;
    float calibration_voltage = 0.39F;

    void print() {
        Serial.println("conf_in_eeprom " + String(conf_in_eeprom));
        Serial.println("n_calibrations " + String(n_calibrations));
        Serial.println("max_voltage " + String(max_voltage));
        Serial.println("min_voltage " + String(min_voltage));
        Serial.println("max_current " + String(max_current));
        Serial.println("min_current " + String(min_current));
        Serial.println("measurement_delay " + String(measurement_delay));
        Serial.println("sensitivity " + String(sensitivity));
        Serial.println("R1 " + String(R1));
        Serial.println("R2 " + String(R2));
        Serial.println("resolution " + String(resolution));
        Serial.println("reference_voltage " + String(reference_voltage));
        Serial.println("calibration_voltage " + String(calibration_voltage));
        Serial.println("\tDone...");
    }
};

const char *filename = "/config.txt";  // <- SD library uses 8.3 filenames
Config conf{};                         // <- global configuration object
EEPROM eeprom(0x50, I2C_DEVICESIZE_24LC04);
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

// Loads the configuration from a file and sets default values
bool load_config_from_SD(const char *filename, Config &conf) {
    // Open file for reading
    File file = SD.open(filename);

    // Allocate a temporary JsonDocument
    // Use arduinojson.org/v6/assistant to compute your file's capacity.
    StaticJsonDocument<512> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println(F("Failed to read file"));
        Serial.println(error.c_str());
        file.close();
        return false;
    }

    // Copy values from the JsonDocument to the Config
    conf.conf_in_eeprom = doc["conf_in_eeprom"] | conf.conf_in_eeprom;
    conf.n_calibrations = doc["n_calibrations"] | conf.n_calibrations;
    conf.max_voltage = doc["max_voltage"] | conf.max_voltage;
    conf.min_voltage = doc["min_voltage"] | conf.min_voltage;
    conf.max_current = doc["max_current"] | conf.max_current;
    conf.min_current = doc["min_current"] | conf.min_current;
    conf.measurement_delay = doc["measurement_delay"] | conf.measurement_delay;
    conf.sensitivity = doc["sensitivity"] | conf.sensitivity;
    conf.R1 = doc["R1"] | conf.R1;
    conf.R2 = doc["R2"] | conf.R2;
    conf.resolution = doc["resolution"] | conf.resolution;
    conf.reference_voltage = doc["reference_voltage"] | conf.reference_voltage;
    conf.calibration_voltage = doc["calibration_voltage"] | conf.calibration_voltage;

    file.close();
    return true;
}

// Saves the configuration to a file
void save_config_to_SD(const char *filename, const Config &conf) {
    // Delete existing file, otherwise the configuration is appended to the file
    SD.remove(filename);

    // Open file for writing
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println(F("Failed to create file"));
        return;
    }

    // Allocate a temporary JsonDocument
    // Use arduinojson.org/assistant to compute your file capacity the capacity.
    StaticJsonDocument<256> doc;

    // Set the values in the document
    doc["n_calibrations"] = conf.n_calibrations;
    doc["max_voltage"] = conf.max_voltage;
    doc["min_voltage"] = conf.min_voltage;
    doc["max_current"] = conf.max_current;
    doc["min_current"] = conf.min_current;
    doc["measurement_delay"] = conf.measurement_delay;
    doc["sensitivity"] = conf.sensitivity;
    doc["R1"] = conf.R1;
    doc["R2"] = conf.R2;
    doc["resolution"] = conf.resolution;

  // Serialize JSON to file with spaces and line-breaks
  if (serializeJsonPretty(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

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

    file.close();
}

void load_config_from_EEPROM(EEPROM& eeprom, Config& conf) {
    eeprom.get(0, conf);
}

void update_config_to_EEPROM(EEPROM& eeprom, const Config& conf) {
    eeprom.put(0, conf);
}

unsigned int timeOn=1000;
unsigned int timeOff=1000;

result doMeasure(eventMask e, prompt &item);
result showEvent(eventMask e,navNode& nav,prompt& item);
result saveConfig();
result printConfig();
result dumpEEPROM();

MENU(settingsMenu,"Settings",Menu::doNothing,Menu::anyEvent,Menu::wrapStyle
  ,FIELD(conf.max_voltage,"Max"," V",0,30,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.min_voltage,"Min"," V",0,30,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.max_current,"Max"," A",0,10,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.min_current,"Min"," A",0,10,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.measurement_delay,"Delay"," ms",100,60000,1000,100,doNothing,noEvent,noStyle)
  ,EXIT("<Back")
);

MENU(mainMenu, "Main Menu", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
//   ,FIELD(timeOn,"On","ms",0,1000,10,1, Menu::doNothing, Menu::noEvent, Menu::noStyle)
//   ,FIELD(timeOff,"Off","ms",0,10000,10,1,Menu::doNothing, Menu::noEvent, Menu::noStyle)
  ,OP("Measure",doMeasure,enterEvent)
  ,SUBMENU(settingsMenu)
  ,OP("Save Config",saveConfig,Menu::enterEvent)
  ,OP("Print Config",printConfig,Menu::enterEvent)
  ,OP("Dump EEPROM",dumpEEPROM,Menu::enterEvent)
  ,EXIT("<Back")
);

// Declare the clickencoder
// Disable doubleclicks in setup makes the response faster.  See: https://github.com/soligen2010/encoder/issues/6
ClickEncoder clickEncoder = ClickEncoder(encA, encB, encBtn, encSteps);
ClickEncoderStream encStream(clickEncoder, 1);

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
MENU_INPUTS(in,&inSerial,&touch,&encStream/*&joystickBtns*/);

void timerIsr() {clickEncoder.service();}

// MENU_OUTPUTS(out,MAX_DEPTH
//   ,SERIAL_OUT(Serial)
//   ,TFT_eSPI_OUT(tft,colors,charW,charH,{0,0,TFT_WIDTH / defaultTextW,TFT_HEIGHT / defaultTextH}) // {x, y, n, m} - x is padding on x axes, y is padding on y axes, n is number of drawned characters on x, m is number of menu items drawned
// //   ,LIQUIDCRYSTAL_OUT(lcd,{0,0,16,2})
// );

NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

float get_calibrated_ref_voltage(unsigned int n_calibrations) {
    tft.println("Calibrating...");
    Serial.println("Calibrating...");

    int value = 0;

    for (size_t i = 0; i < n_calibrations; i++)
    {
        value += analogRead(CURRENT_INPUT_PIN);
        delay(40);
    }
    
    float avg_value = value / (n_calibrations - 1);
    Serial.println("Done!");
    return conf.reference_voltage / conf.resolution * avg_value;
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
    float voltage = conf.reference_voltage / conf.resolution * raw_voltage;
    return voltage * (conf.R1 + conf.R2) / conf.R2;
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
    float current_voltage = conf.reference_voltage / conf.resolution * raw_current_voltage;
    float current = (current_voltage - conf.calibration_voltage) / conf.sensitivity;
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

    if (millis() - last_measurement_time > conf.measurement_delay)
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
    save_config_to_SD(filename, conf);
    update_config_to_EEPROM(eeprom, conf);
    return proceed;
}

result printConfig() {
    Serial.println();
    Serial.println();
    printFile(filename);
    conf.print();
    return proceed;
}

result dumpEEPROM() {
    eeprom.dumpEEPROM(0, 256);
    return proceed;
}

uint16_t Timer1_Index = 0;

uint16_t attachDueInterrupt(double microseconds, timerCallback callback, const char* TimerName) {
    DueTimerInterrupt dueTimerInterrupt = DueTimer.getAvailable();

    dueTimerInterrupt.attachInterruptInterval(microseconds, callback);

    uint16_t timerNumber = dueTimerInterrupt.getTimerNumber();

    // Serial.print(TimerName);
    // Serial.print(F(" attached to Timer("));
    // Serial.print(timerNumber);
    // Serial.println(F(")"));

    return timerNumber;
}

void setup() {
    pinMode(LEDPIN, OUTPUT);
    pinMode(encBtn,INPUT_PULLUP);

    // Initilize communication over serial line
    Serial.begin(9600);
    while(!Serial);

    // Initialize SD library
    if (SD.begin(SD_CS) == false) {
        Serial.println(F("WARNING: Failed to initialize SD library"));
        delay(100);
    }

    // Initialize EEPROM
    eeprom.begin();
    if (eeprom.isConnected() == false) {
        Serial.println(F("ERROR: Can't find eeprom"));
        Serial.println(F("WARNING: Loading default configuration"));
    }

    // Load conf from SD card
    Serial.println(F("Loading configuration..."));
    if(load_config_from_SD(filename, conf) == false) { // If loading from SD fails...
        uint8_t eeprom_set = 0;
        eeprom.get(0, eeprom_set); // Read from EEPROM if config was ever written to it
        if (eeprom_set == 1) {
            load_config_from_EEPROM(eeprom, conf); // If it was, load it
        }
        else {
            conf.conf_in_eeprom = 1; // If it wasn't, set it as it was
        }
    }
    update_config_to_EEPROM(eeprom, conf); // Save config to EEPROM

    conf.print();

    Serial.println("INFO: Use keys '+', '-', '*', '/'");
    Serial.println("to control the menu navigation");
    Serial.flush();

    // Initialize TFT LCD display
    tft.init();
    tft.setRotation(2);
    // gfx.setTextSize(textScale);
    tft.setTextWrap(true);
    tft.setTextSize(4);
    tft.fillScreen(Black);
    tft.setTextColor(White, Black);

    // Touch screen calibration
    uint16_t calData[5] = { 102, 3712, 67, 3863, 2 };
    tft.setTouch(calData);

    nav.showTitle = false;
    nav.idleTask = measure;
    conf.calibration_voltage = get_calibrated_ref_voltage(conf.n_calibrations); // Calibrate reference voltage
    Timer1_Index = attachDueInterrupt(1000, timerIsr, "Timer1");
    // Timer1.initialize(1000);
    // Timer1.attachInterrupt(timerIsr);
    nav.idleOn(measure);//this menu will start on idle state, press select to enter menu
}

bool blink(int timeOn,int timeOff) {return millis()%(unsigned long)(timeOn+timeOff)<(unsigned long)timeOn;}

void loop() {
    nav.poll();
    digitalWrite(LEDPIN, blink(timeOn,timeOff));
}