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

#include <Wire.h>
#include <RtcDS1307.h>
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
#include "config.h"
#include "quantity.h"
#include "measurement.h"

#define LEDPIN LED_BUILTIN
#define MAX_DEPTH 2

#define CURRENT_INPUT_PIN A9
#define VOLTAGE_INPUT_PIN A2

#define SOFT_DEBOUNCE_MS 100

unsigned long last_measurement_time = 0;

// Encoder /////////////////////////////////////
#define encA 11
#define encB 12
//this encoder has a button here
#define encBtn 10
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

const char *filename = "/config.txt";  // <- SD library uses 8.3 filenames
Config conf{};                         // <- global configuration object
EEPROM eeprom(0x50, I2C_DEVICESIZE_24LC04);
TFT_eSPI tft{};       // Invoke custom library
RtcDS1307<TwoWire> rtc(Wire);
#define countof(a) (sizeof(a) / sizeof(a[0])) // RTC thing, not sure what it does. INSPECT!!!

void printDateTime(const RtcDateTime& dt) {
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

class Measurement : public MeasurementTemplate {
private:
    Quantity voltage;
    Quantity current;
    Quantity power;
    Quantity consumption;
    RtcDateTime now;

    float measure_voltage(uint32_t pin, const Config& conf) const override {
        unsigned int raw_voltage = analogRead(pin);
        float voltage = conf.reference_voltage / conf.resolution * raw_voltage;
        voltage *= (conf.R1 + conf.R2) / conf.R2;
        return voltage;
    }

    float measure_current(uint32_t pin, const Config& conf) const override {
        unsigned int raw_current_voltage = analogRead(pin);
        float current_voltage = conf.reference_voltage / conf.resolution * raw_current_voltage;
        float current = (current_voltage - conf.calibration_voltage) / conf.sensitivity;
        return current > 0 ? current : 0;
    }

    float measure_power(const float& voltage, const float& current) const override {
        return voltage * current;
    }

    float measure_consumption(const float& power, const Config& conf) const override {
        return power / conf.measurement_delay / 1000 * 3600;
    }

public:
    Measurement() : voltage(measure_voltage(VOLTAGE_INPUT_PIN, conf), "V"),
                    current(measure_current(CURRENT_INPUT_PIN, conf), "A"),
                    power(measure_power(voltage.get_value(), current.get_value()), "W"),
                    consumption(measure_consumption(power.get_value(), conf), "Wh"),
                    now(rtc.GetDateTime()) { };

    ~Measurement() { };

    template<typename O>
    void print(O& out, size_t index) {
        out.setCursor(0, index);
        out.print(/*String(now.Year(), DEC) + "/" + String(now.Month(), DEC) + "/" + String(now.Day(), DEC) + " " + */
                String(now.Hour(), DEC) + ":" + String(now.Minute(), DEC) + ":" + String(now.Second(), DEC));
        #if SERIAL_DEBUG == 1
        Serial.println(/*String(now.Year(), DEC) + "/" + String(now.Month(), DEC) + "/" + String(now.Day(), DEC) + " " + */
                String(now.Hour(), DEC) + ":" + String(now.Minute(), DEC) + ":" + String(now.Second(), DEC));
        Serial.println();
        #endif
        tft.setTextColor(White, Black);
    }

    template<typename O, typename T, typename... Args>
    void print(O& out, size_t index, T& arg, Args... args) {
        // out.setCursor(0, 0);
        // out.print(String(voltage.get_value()) + " " + String(voltage.get_unit()));
        // out.setCursor(0, 1);
        // out.print(String(current.get_value()) + " " + String(current.get_unit()));
        // out.setCursor(0, 2);
        // out.print(String(power.get_value()) + " " + String(power.get_unit()));
        // out.setCursor(0, 3);
        // out.print(String(consumption.get_value()) + " " + String(consumption.get_unit()));
        // out.setCursor(0, 4);
        // out.print(/*String(now.Year(), DEC) + "/" + String(now.Month(), DEC) + "/" + String(now.Day(), DEC) + " " + */
        //         String(now.Hour(), DEC) + ":" + String(now.Minute(), DEC) + ":" + String(now.Second(), DEC));
        switch (arg.get_eval()) {
        case Eval::Low:
            tft.setTextColor(Yellow, Black);
            break;
        case Eval::High:
            tft.setTextColor(Red, Black);
            break;
        default:
            break;
        }

        out.setCursor(0, index);
        out.print(String(arg.get_value()) + " " + String(arg.get_unit()));
        #if SERIAL_DEBUG == 1
        Serial.println(String(arg.get_value()) + " " + String(arg.get_unit()));
        #endif
        tft.setTextColor(White, Black);
        print(out, index + 1, args...);
    }

    void evaluate_measurements() {
        voltage.evaluate(conf.min_voltage, conf.max_voltage);
        current.evaluate(conf.min_current, conf.max_current);
    }

    Quantity& get_voltage() { return voltage; }
    Quantity& get_current() { return current; }
    Quantity& get_power() { return power; }
    Quantity& get_consumption() { return consumption; }
};

template<typename O>
void init_SD(O& tft_out) {
    // Initialize SD library
    for (byte i{}; i < 10; i++) {
        if(SD.begin(SD_CS) == false) {
            tft_out.clear();
            tft_out.setCursor(0, 0);
            tft_out.print("ERROR: Failed to mount SD");
            tft_out.setCursor(0, 1);
            tft_out.print("Try " + String(i) + "/10");
            #if SERIAL_DEBUG == 1
                Serial.println(F("ERROR: Failed to mount SD"));
                Serial.println("Try " + String(i) + "/10");
            #endif
        } else {
            tft_out.clear();
            tft_out.setCursor(0, 0);
            tft_out.print("INFO: SD card mounted successfully.");
            #if SERIAL_DEBUG == 1
                Serial.println("INFO: SD card mounted successfully.");
            #endif
            return;
        }
    }
    tft_out.clear();
    tft_out.setCursor(0, 0);
    tft_out.print("WARNING: Could not mount SD card. Proceeding to load configuration from EEPROM. Measured data WILL NOT BE SAVED!!!");
    #if SERIAL_DEBUG == 1
        Serial.println("WARNING: Could not mount SD card. Proceeding to load configuration from EEPROM. Measured data WILL NOT BE SAVED!!!");
    #endif
}

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

bool log_measurement_to_SD(Measurement& measurement, const String& filename) {
    #if INIT_SD == 1
        if(SD.begin(SD_CS) == false) {
            #if SERIAL_DEBUG == 1
                Serial.println(F("ERROR: Failed to mount SD"));
            #endif
            return false;
        } else {
            #if SERIAL_DEBUG == 1
                Serial.println("INFO: SD card mounted successfully.");
            #endif
        }
    #endif
    
    String log = "";
    if (SD.exists(filename) == false) {
        log = "Voltage [V],Current [A],Power [W],Consumption [Wh]\n";
    }
    
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println(F("ERROR: Failed to log the measurement"));
        return false;
    }

    log += (String(measurement.get_voltage().get_value()) + "," +
            String(measurement.get_current().get_value()) + "," +
            String(measurement.get_power().get_value()) + "," + 
            String(measurement.get_consumption().get_value()));
    
    file.println(log);  // Write the log to the file
    if (file.getWriteError() != 0) {
        Serial.println("ERROR: Failed to log the measurement. Write error: " + String(file.getWriteError()));
        file.close();
        return false;
    }

    file.close();
    Serial.println("INFO: Measurement saved successfully");

    #if INIT_SD == 1
        SD.end();
    #endif

    return true;
}

unsigned int timeOn=1000;
unsigned int timeOff=1000;

// result doMeasure(eventMask e, prompt &item);
result showEvent(eventMask e,navNode& nav,prompt& item);
result eject_SD();
result mount_SD();
result load_config();
result save_config();
result print_config();
result dump_EEPROM();

MENU(settingsMenu,"Settings",Menu::doNothing,Menu::anyEvent,Menu::wrapStyle
  ,FIELD(conf.max_voltage,"Max"," V",0,30,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.min_voltage,"Min"," V",0,30,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.max_current,"Max"," A",0,10,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.min_current,"Min"," A",0,10,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.measurement_delay,"Delay"," ms",100,60000,1000,100,doNothing,noEvent,noStyle)
  ,EXIT("<Back")
);

MENU(mainMenu, "Main Menu", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
//    ,FIELD(timeOn,"On","ms",0,1000,10,1, Menu::doNothing, Menu::noEvent, Menu::noStyle)
//    ,FIELD(timeOff,"Off","ms",0,10000,10,1,Menu::doNothing, Menu::noEvent, Menu::noStyle)
//    ,OP("Measure",doMeasure,enterEvent)
    ,EXIT("Measure")
    ,SUBMENU(settingsMenu)
    ,OP("Eject SD",eject_SD,Menu::enterEvent)
    ,OP("Mount SD",mount_SD,Menu::enterEvent)
    ,OP("Load Config",load_config,Menu::enterEvent)
    ,OP("Save Config",save_config,Menu::enterEvent)
    ,OP("Print Config",print_config,Menu::enterEvent)
    ,OP("Dump EEPROM",dump_EEPROM,Menu::enterEvent)
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


// result measure(menuOut& o,idleEvent e) {
//     nav.idleChanged = true;

//     if (millis() - last_measurement_time > conf.measurement_delay)
//     {
//         last_measurement_time = millis();
//         Measurement measurement;
//         measurement.evaluate_measurements();
//         measurement.print(o, 0, measurement.get_voltage(), measurement.get_current(), measurement.get_power(), measurement.get_consumption());
//         tft.endWrite();
//         log_measurement_to_SD(measurement, "log.txt");
//     }
//     return proceed;
// }

void measure() {
    if (millis() - last_measurement_time > conf.measurement_delay)
    {
        last_measurement_time = millis();
        Measurement measurement;
        measurement.evaluate_measurements();
        auto result = log_measurement_to_SD(measurement, "log.txt");
        measurement.print(eSpiOut, 0, measurement.get_voltage(), measurement.get_current(), measurement.get_power(), measurement.get_consumption());
        if (result == false) {
            eSpiOut.setCursor(0, 8);
            eSpiOut.print("Log failed");
        }
        tft.endWrite();
    }
}

// result doMeasure(eventMask e, prompt &item) {
//     nav.idleOn(measure);
//     return proceed;
// }

result eject_SD() {
    SD.end();
    Serial.println();
    Serial.println();
    Serial.println("Card unmounted");
    return proceed;
}

result mount_SD() {
    init_SD(eSpiOut);
    return proceed;
}

result load_config() {
    load_config_from_SD(filename, conf);
    return proceed;
}

result save_config() {
    save_config_to_SD(filename, conf);
    update_config_to_EEPROM(eeprom, conf);
    return proceed;
}

result print_config() {
    Serial.println();
    Serial.println();
    conf.print();
    return proceed;
}

result dump_EEPROM() {
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

// Sd2Card card;
// SdVolume volume;
// SdFile root;

void setup() {
    pinMode(LEDPIN, OUTPUT);
    pinMode(encBtn,INPUT_PULLUP);
    // pinMode(10, OUTPUT); // change this to 53 on a mega  // don't follow this!!
    // digitalWrite(10, HIGH); // Add this line

    #if SERIAL_DEBUG == 1
    // Initilize communication over serial line
    Serial.begin(9600);
    while(!Serial);
    #endif

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

    // Initialize SD library
    init_SD(eSpiOut);

//     Serial.print("\nInitializing SD card...");

//   // we'll use the initialization code from the utility libraries
//   // since we're just testing if the card is working!
//   if (!card.init(SPI_HALF_SPEED, SD_CS)) {
//     Serial.println("initialization failed. Things to check:");
//     Serial.println("* is a card inserted?");
//     Serial.println("* is your wiring correct?");
//     Serial.println("* did you change the chipSelect pin to match your shield or module?");
//     while (1);
//   } else {
//     Serial.println("Wiring is correct and a card is present.");
//   }

//   // print the type of card
//   Serial.println();
//   Serial.print("Card type:         ");
//   switch (card.type()) {
//     case SD_CARD_TYPE_SD1:
//       Serial.println("SD1");
//       break;
//     case SD_CARD_TYPE_SD2:
//       Serial.println("SD2");
//       break;
//     case SD_CARD_TYPE_SDHC:
//       Serial.println("SDHC");
//       break;
//     default:
//       Serial.println("Unknown");
//   }

//   // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
//   if (!volume.init(card)) {
//     Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
//     while (1);
//   }

//   Serial.print("Clusters:          ");
//   Serial.println(volume.clusterCount());
//   Serial.print("Blocks x Cluster:  ");
//   Serial.println(volume.blocksPerCluster());

//   Serial.print("Total Blocks:      ");
//   Serial.println(volume.blocksPerCluster() * volume.clusterCount());
//   Serial.println();

//   // print the type and size of the first FAT-type volume
//   uint32_t volumesize;
//   Serial.print("Volume type is:    FAT");
//   Serial.println(volume.fatType(), DEC);

//   volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
//   volumesize *= volume.clusterCount();       // we'll have a lot of clusters
//   volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
//   Serial.print("Volume size (Kb):  ");
//   Serial.println(volumesize);
//   Serial.print("Volume size (Mb):  ");
//   volumesize /= 1024;
//   Serial.println(volumesize);
//   Serial.print("Volume size (Gb):  ");
//   Serial.println((float)volumesize / 1024.0);

//   Serial.println("\nFiles found on the card (name, date and size in bytes): ");
//   root.openRoot(volume);

//   // list all files in the card with date and size
//   root.ls(LS_R | LS_DATE | LS_SIZE);

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
    SD.end();

    rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();

    if (!rtc.IsDateTimeValid()) {
        if (rtc.LastError() != 0) {
            // we have a communications error
            // see https://www.arduino.cc/en/Reference/WireEndTransmission for 
            // what the number means
            Serial.print("RTC communications error = ");
            Serial.println(rtc.LastError());
        } else {
            // Common Causes:
            //    1) first time you ran and the device wasn't running yet
            //    2) the battery on the device is low or even missing

            Serial.println("RTC lost confidence in the DateTime!");
            // following line sets the RTC to the date & time this sketch was compiled
            // it will also reset the valid flag internally unless the Rtc device is
            // having an issue

            rtc.SetDateTime(compiled);
        }
    }

    if (!rtc.GetIsRunning()) {
        Serial.println("RTC was not actively running, starting now");
        rtc.SetIsRunning(true);
    }

    RtcDateTime now = rtc.GetDateTime();
    if (now < compiled) {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        rtc.SetDateTime(compiled);
    }
    else if (now > compiled) {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    rtc.SetSquareWavePin(DS1307SquareWaveOut_Low); 

    Serial.println("INFO: Use keys '+', '-', '*', '/'");
    Serial.println("to control the menu navigation");
    Serial.flush();

    conf.calibration_voltage = get_calibrated_ref_voltage(conf.n_calibrations); // Calibrate reference voltage
    eSpiOut.clear();

    Timer1_Index = attachDueInterrupt(1000, timerIsr, "Timer1");

    nav.showTitle = false;
    nav.timeOut = 300;  // Set the number of seconds of "inactivity" to auto enter idle state
    nav.idleOn();
    // nav.idleTask = measure;
    
    // Timer1.initialize(1000);
    // Timer1.attachInterrupt(timerIsr);
    // nav.idleOn(measure);//this menu will start on idle state, press select to enter menu
}

bool blink(int timeOn,int timeOff) {return millis()%(unsigned long)(timeOn+timeOff)<(unsigned long)timeOn;}

void loop() {
    nav.poll();
    digitalWrite(LEDPIN, blink(timeOn,timeOff));
    if (nav.sleepTask) {
        measure();
    }
}