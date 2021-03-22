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

#define LEDPIN LED_BUILTIN
#define MAX_DEPTH 3

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

const char *filename = "config";  // <- SD library uses 8.3 filenames
Config conf{};                         // <- global configuration object
EEPROM eeprom(0x50, I2C_DEVICESIZE_24LC04);
TFT_eSPI tft{};       // Invoke custom library
RtcDS1307<TwoWire> rtc(Wire);
#define countof(a) (sizeof(a) / sizeof(a[0])) // RTC thing, not sure what it does. INSPECT!!!

String getDateTime(const RtcDateTime& dt) {
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
    return String(datestring);
}

String getDate(const RtcDateTime& dt) {
    char datestring[11];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%04u/%02u/%02u"),
            dt.Year(),
            dt.Month(),
            dt.Day() );
    return String(datestring);
}

String getTime(const RtcDateTime& dt) {
    char datestring[9];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u:%02u:%02u"),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return String(datestring);
}

void setDateTime(uint16_t* o_date, uint16_t* o_time) {
    RtcDateTime now = rtc.GetDateTime();
    *o_date = FAT_DATE(now.Year(), now.Month(), now.Day());
    *o_time = FAT_DATE(now.Hour(), now.Minute(), now.Second());
}

#define textScale 4
#define defaultTextW 6
#define defaultTextH 9

const int charW = defaultTextW * textScale;
const int charH = defaultTextH * textScale;

const panel panels[] MEMMODE = {{0, 0, TFT_WIDTH / defaultTextW,TFT_HEIGHT / defaultTextH}};
navNode* nodes[sizeof(panels) / sizeof(panel)]; //navNodes to store navigation status
panelsList pList(panels, nodes, 1); //a list of panels and nodes
idx_t eSpiTops[MAX_DEPTH]={0};
TFT_eSPIOut eSpiOut(tft,colors,eSpiTops,pList,charW,charH+1);

class Measurement {
public:
    #if DEBUG_MEASUREMENT == 1
        static uint32_t raw_voltage(uint32_t pin) { return analogRead(pin); }
        static float voltage(float raw_voltage) {
            auto result = conf.reference_voltage / conf.resolution * raw_voltage;
            return result > 0 ? result : 0;
        }
        static float converted_voltage(float voltage) {
            auto result = voltage * (conf.R1 + conf.R2) / conf.R2 + conf.voltage_calibration;
            return result > 0 ? result : 0;
        }
        static uint32_t raw_current_voltage(uint32_t pin) { return analogRead(pin); }
        static long uncalibrated_raw_current_voltage(uint32_t raw_current_voltage, const Config& conf) {
            auto result = (long)(raw_current_voltage - conf.current_calibration);
            return result > 0 ? result : 0;
        }
        static float current_voltage(uint32_t raw_current_voltage, const Config& conf) {
            auto result = conf.reference_voltage / conf.resolution * (long)(raw_current_voltage - conf.current_calibration) * 182400 / 120300;
            return result > 0 ? result : 0;
        }
        static float current(float current_voltage, const Config& conf) { return current_voltage / conf.sensitivity; }
        static float consumption(const float& power, const Config& conf) { return power / conf.measurement_delay / 1000 * 3600; }
    #endif

    static float measure_voltage(uint32_t pin, const Config& conf) {
        auto raw_voltage = analogRead(pin);
        float voltage = conf.reference_voltage / conf.resolution * raw_voltage;
        // voltage *= (conf.R1 + conf.R2) / conf.R2;
        return voltage >= 0 ? voltage : 0;
    }

    static float measure_converted_voltage(float voltage, const Config& conf) {
        float result = voltage * (conf.R1 + conf.R2) / conf.R2 + conf.voltage_calibration;
        return result >= 0 ? result : 0;
    }

    static float measure_current(uint32_t pin, const Config& conf) {
        unsigned int raw_current_voltage = analogRead(pin);
        float current_voltage = conf.reference_voltage / conf.resolution * raw_current_voltage * 182400 / 120300 - conf.current_calibration;
        float current = current_voltage / conf.sensitivity;
        return current > 0 ? current : 0;
    }

    static float measure_power(const float& voltage, const float& current) {
        return voltage * current;
    }

    static float measure_consumption(const float& power, const Config& conf) {
        return power / conf.measurement_delay / 1000 * 3600;
    }

public:
    #if DEBUG_MEASUREMENT == 0
    template<typename O>
    static void print(O& out, const RtcDateTime& now, size_t index) {
        out.setCursor(0, index);
        out.print(getTime(now));
        #if SERIAL_DEBUG == 1
        Serial.println(getTime(now));
        Serial.println();
        #endif
        tft.setTextColor(White, Black);
    }

    template<typename O, typename T, typename... Args>
    static void print(O& out, const RtcDateTime& now, size_t index, T& arg, Args... args) {
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
        print(out, now, index + 1, args...);
    }
    #endif

    #if DEBUG_MEASUREMENT == 1
    template<typename O>
    static void print(O& out, const RtcDateTime& now, size_t index) {
        out.setCursor(0, index);
        out.print(getTime(now));
        #if SERIAL_DEBUG == 1
        Serial.println(getTime(now));
        Serial.println();
        #endif
        tft.setTextColor(White, Black);
    }

    template<typename O, typename T, typename... Args>
    static void print(O& out, const RtcDateTime& now, size_t index, T& arg, Args... args) {
        out.setCursor(0, index);
        out.print(String(arg));
        #if SERIAL_DEBUG == 1
        Serial.println(String(arg));
        #endif
        tft.setTextColor(White, Black);
        print(out, now, index + 1, args...);
    }
    #endif
};

template<typename O>
void init_SD(size_t count, O& tft_out) {
    // Initialize SD library
    for (byte i{}; i < count; i++) {
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
    init_SD(1, eSpiOut);
    
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
        SD.end();
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
    conf.current_calibration = doc["current_calibration"] | conf.current_calibration;
    conf.voltage_calibration = doc["voltage_calibration"] | conf.voltage_calibration;
    conf.intended_voltage = doc["intended_voltage"] | conf.intended_voltage;
    conf.current_correction = doc["current_correction"] | conf.current_correction;

    file.close();
    SD.end();
    return true;
}

// Saves the configuration to a file
void save_config_to_SD(const char *filename, const Config &conf) {
    init_SD(1, eSpiOut);
    // Delete existing file, otherwise the configuration is appended to the file
    SD.remove(filename);

    SdFile::dateTimeCallback(setDateTime);
    // Open file for writing
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println(F("Failed to create file"));
        return;
    }

    // Allocate a temporary JsonDocument
    // Use arduinojson.org/assistant to compute your file capacity the capacity.
    StaticJsonDocument<512> doc;

    // Set the values in the document
    doc["conf_in_eeprom"] | conf.conf_in_eeprom;
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
    doc["reference_voltage"] = conf.reference_voltage;
    doc["current_calibration"] = conf.current_calibration;
    doc["voltage_calibration"] = conf.voltage_calibration;
    doc["intended_voltage"] = conf.intended_voltage;
    doc["current_correction"] = conf.current_correction;

    // Serialize JSON to file with spaces and line-breaks
    if (serializeJsonPretty(doc, file) == 0) {
        Serial.println(F("Failed to write to file"));
    }

    file.close();
    SD.end();
}

// Prints the content of a file to the Serial
void printFile(const char *filename) {
    init_SD(1, eSpiOut);
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
    SD.end();
}

void load_config_from_EEPROM(EEPROM& eeprom, Config& conf) {
    eeprom.get(0, conf);
}

void update_config_to_EEPROM(EEPROM& eeprom, const Config& conf) {
    eeprom.put(0, conf);
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
result calibrate_voltage();
result load_date(eventMask e);
result load_time(eventMask e);

//custom field print
//implementing a customized menu component
//this numeric field prints formatted number with leading zeros
template<typename T>
class timeField : public menuField<T> {
public:
    timeField(constMEM menuFieldShadow<T>& shadow):menuField<T>(shadow) {}
    Used printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) {
        menuField<T>::reflex=menuField<T>::target();
        prompt::printTo(root,sel,out,idx,len);
        bool ed=this==root.navFocus;
        // out.print((root.navFocus==this&&sel)?(menuField<T>::tunning?'>':':'):' ');
        out.setColor(valColor,sel,menuField<T>::enabled,ed);
        char buffer[]="      ";
        sprintf(buffer, "%03d", menuField<T>::reflex);
        out.print(buffer);
        out.setColor(unitColor,sel,menuField<T>::enabled,ed);
        print_P(out,menuField<T>::units(),len);
        return len;
    }
};

uint16_t year = 2021;
uint16_t month = 1;
uint16_t day = 1;

uint16_t hour = 12;
uint16_t minute = 0;
uint16_t second = 0;

PADMENU(date_m,"",load_date,anyEvent,noStyle
  ,altFIELD(timeField,year,"","/",1900,3000,1,0,doNothing,anyEvent,noStyle)
  ,altFIELD(timeField,month,"","/",1,12,1,0,doNothing,anyEvent,wrapStyle)
  ,altFIELD(timeField,day,"","",1,31,1,0,doNothing,anyEvent,wrapStyle)
);

PADMENU(time_m,"",load_time,anyEvent,noStyle
  ,altFIELD(timeField,hour,"",":",0,23,1,0,doNothing,anyEvent,noStyle)
  ,altFIELD(timeField,minute,"",":",0,59,1,0,doNothing,anyEvent,wrapStyle)
  ,altFIELD(timeField,second,"","",0,59,1,0,doNothing,anyEvent,wrapStyle)
);

MENU(settingsMenu,"Settings",Menu::doNothing,Menu::anyEvent,Menu::wrapStyle
  ,FIELD(conf.max_voltage,"Max"," V",0,30,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.min_voltage,"Min"," V",0,30,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.max_current,"Max"," A",0,10,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.min_current,"Min"," A",0,10,1,0.01,doNothing,noEvent,noStyle)
  ,FIELD(conf.measurement_delay,"Delay"," ms",100,60000,1000,100,doNothing,noEvent,noStyle)
  ,FIELD(conf.intended_voltage,"Calibration Voltage"," V",0,16,1,0,calibrate_voltage,exitEvent,noStyle)
  ,SUBMENU(date_m)
  ,SUBMENU(time_m)
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

size_t get_calibrated_ref_value(unsigned int n_calibrations) {
    tft.println("Calibrating...");
    Serial.println("Calibrating...");

    size_t value = 0;
    delay(1000);

    for (size_t i = 0; i < n_calibrations; i++)
    {
        value  += analogRead(CURRENT_INPUT_PIN);
        delay(60);
    }
    
    auto avg_value = value / n_calibrations;
    eSpiOut.print(avg_value);
    Serial.println(avg_value);
    Serial.println("Done!");
    delay(2000);
    return avg_value;
}

struct MyMeasurement {
    #if DEBUG_MEASUREMENT == 0
        const Quantity& voltage;
        const EvaluatableQuantity& converted_voltage;
        const EvaluatableQuantity& current;
        const Quantity& power;
        const Quantity& consumption;
    #endif
        #if DEBUG_MEASUREMENT == 1
        uint32_t raw_voltage;
        float voltage;
        float converted_voltage;
        uint32_t raw_current_voltage;
        long uncalibrated_raw_current_voltage;
        float current_voltage;
        float current;
        float power;
        float consumption;
    #endif
    const RtcDateTime& now;

public:
    #if DEBUG_MEASUREMENT == 0
    MyMeasurement(const Quantity& voltage,
                  const EvaluatableQuantity& converted_voltage,
                  const EvaluatableQuantity& current,
                  const Quantity& power,
                  const Quantity& consumption,
                  const RtcDateTime& now) :
                  voltage{ voltage },
                  converted_voltage{ converted_voltage },
                  current{ current },
                  power{ power },
                  consumption{ consumption },
                  now{ now } {}
    #endif
    #if DEBUG_MEASUREMENT == 1
    MyMeasurement(const RtcDateTime& now) : raw_voltage{ Measurement::raw_current_voltage(VOLTAGE_INPUT_PIN) },
                      voltage{ Measurement::voltage(raw_voltage) },
                      converted_voltage{ Measurement::converted_voltage(voltage) },
                      raw_current_voltage{ Measurement::raw_current_voltage(CURRENT_INPUT_PIN) },
                      uncalibrated_raw_current_voltage{ Measurement::uncalibrated_raw_current_voltage(raw_current_voltage, conf) },
                      current_voltage{ Measurement::current_voltage(raw_current_voltage, conf) },
                      current{ Measurement::current(current_voltage, conf) },
                      power{ Measurement::measure_power(converted_voltage, current) },
                      consumption{ Measurement::consumption(power, conf) },
                      now{ now } {}
    #endif

    #if DEBUG_MEASUREMENT == 0
    bool log_to_SD(const String& filename) {
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
        
        String final_filename = String(now.Day()) + "-" + String(now.Month()) + ".csv";
        Serial.println(final_filename);
        String log = "";
        if (SD.exists(final_filename) == false) {
            log = "Time [HH:MM:SS],Voltage [V],State [None=0|Low|Fine|High],Current [A],State [None=0|Low|Fine|High],Power [W],Consumption [Wh]\n";
        }
        
        SdFile::dateTimeCallback(setDateTime);
        File file = SD.open(final_filename, FILE_WRITE);
        if (!file) {
            Serial.println("ERROR: Failed to log the measurement. Write error: " + String(file.getWriteError()));
            return false;
        }

        log += (getTime(now) + "," +
                String(converted_voltage.get_value()) + "," +
                String(converted_voltage.get_eval()) + "," +
                String(current.get_value()) + "," +
                String(current.get_eval()) + "," +
                String(power.get_value()) + "," + 
                String(consumption.get_value()));
        
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
    #endif

    #if DEBUG_MEASUREMENT == 1
    bool log_to_SD(const String& filename) {
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
        
        String final_filename = String(now.Day()) + "-" + String(now.Month()) + "-d" + ".csv";
        String log = "";
        if (SD.exists(final_filename) == false) {
            log = "Time [HH:MM:SS],Raw Voltage,Voltage [V],Converted Voltage [V],Raw Current,Current Voltage [V],Current [A],Power [W],Consumption [Wh]\n";
        }
        
        SdFile::dateTimeCallback(setDateTime);
        File file = SD.open(final_filename, FILE_WRITE);
        if (!file) {
            Serial.println(F("ERROR: Failed to log the measurement"));
            return false;
        }

        log += (getTime(now) + "," +
                String(raw_voltage) + "," +
                String(voltage) + "," +
                String(converted_voltage) + "," +
                String(converted_voltage) + "," +
                String(raw_current_voltage) + "," +
                String(current_voltage) + "," +
                String(current) + "," +
                String(power) + "," + 
                String(consumption));
        
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
    #endif
};

bool is_calibrated = false;

bool measure() {
    if (millis() - last_measurement_time > conf.measurement_delay) {
        last_measurement_time = millis();
        #if DEBUG_MEASUREMENT == 0
        Quantity voltage{ Measurement::measure_voltage(VOLTAGE_INPUT_PIN, conf), "V" };
        EvaluatableQuantity converted_voltage{ Measurement::measure_converted_voltage(voltage.get_value(), conf), "V", conf.min_voltage, conf.max_voltage };
        EvaluatableQuantity current{ Measurement::measure_current(CURRENT_INPUT_PIN, conf), "A", conf.min_current, conf.max_current };
        Quantity power{ Measurement::measure_power(converted_voltage.get_value(), current.get_value()), "W" };
        Quantity consumption{ Measurement::measure_consumption(power.get_value(), conf), "Wh" };
        RtcDateTime now{ rtc.GetDateTime() };
        MyMeasurement measurement{ voltage, converted_voltage, current, power, consumption, now };
        #endif
        #if DEBUG_MEASUREMENT == 1
        RtcDateTime now{ rtc.GetDateTime() };
        MyMeasurement measurement{ now };
        #endif
        auto result = measurement.log_to_SD("log");
        #if DEBUG_MEASUREMENT == 1
        Measurement::print(eSpiOut, measurement.now, 0, measurement.raw_voltage,
                                                        measurement.voltage,
                                                        measurement.converted_voltage,
                                                        measurement.raw_current_voltage,
                                                        measurement.uncalibrated_raw_current_voltage,
                                                        measurement.current_voltage,
                                                        measurement.current,
                                                        measurement.power,
                                                        measurement.consumption);
        #endif
        #if DEBUG_MEASUREMENT == 0
        Measurement::print(eSpiOut, measurement.now, 0, measurement.converted_voltage,
                                                        measurement.current,
                                                        measurement.power,
                                                        measurement.consumption);
        #endif
        if (result == false) {
            eSpiOut.setCursor(0, 9);
            eSpiOut.print("Log failed");
        }
        tft.endWrite();
        return result;
    }
    return false;
}

result eject_SD() {
    SD.end();
    Serial.println();
    Serial.println();
    Serial.println("Card unmounted");
    return proceed;
}

result mount_SD() {
    init_SD(10, eSpiOut);
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

result calibrate_voltage() {
    float total = 0;
    for (byte i = 0; i < 10; i++) {
        Quantity voltage{ Measurement::measure_voltage(VOLTAGE_INPUT_PIN, conf), "V" };
        Quantity converted_voltage{ Measurement::measure_converted_voltage(voltage.get_value(), conf), "V" };
        total += converted_voltage.get_value();
    }
    conf.voltage_calibration = conf.intended_voltage - total / 10;
    eSpiOut.print(conf.voltage_calibration);
    return proceed;
}

result load_date(eventMask e) {
    auto now = rtc.GetDateTime();
    if (e == eventMask::enterEvent) {
        year = now.Year();
        month = now.Month();
        day = now.Day();
        return proceed;
    }
    if (e == eventMask::exitEvent) {
        now = RtcDateTime(year, month, day, now.Hour(), now.Minute(), now.Second());
        rtc.SetDateTime(now);
    }
    return proceed;
}

result load_time(eventMask e) {
    auto now = rtc.GetDateTime();
    if (e == eventMask::enterEvent) {
        hour = now.Hour();
        minute = now.Minute();
        second = now.Second();
        return proceed;
    }
    if (e == eventMask::exitEvent) {
        now = RtcDateTime(now.Year(), now.Month(), now.Day(), hour, minute, second);
        rtc.SetDateTime(now);
    }
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
    pinMode(CURRENT_INPUT_PIN, INPUT);
    pinMode(VOLTAGE_INPUT_PIN, INPUT);
    analogReadResolution(12);
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

    rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    Serial.println(getDateTime(compiled));
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

    conf.current_calibration = get_calibrated_ref_value(conf.n_calibrations) + conf.current_correction; // Calibrate reference voltage
    Quantity voltage{ Measurement::measure_voltage(VOLTAGE_INPUT_PIN, conf), "V" };
    Quantity converted_voltage{ Measurement::measure_converted_voltage(voltage.get_value(), conf), "V" };
    conf.voltage_calibration = conf.intended_voltage - converted_voltage.get_value();
    eSpiOut.print(conf.voltage_calibration);
    delay(2000);
    eSpiOut.clear();

    Timer1_Index = attachDueInterrupt(1000, timerIsr, "Timer1");

    nav.showTitle = false;
    // nav.timeOut = 300;  // Set the number of seconds of "inactivity" to auto enter idle state
    nav.idleOn();
    // nav.idleTask = measure;
    
    // Timer1.initialize(1000);
    // Timer1.attachInterrupt(timerIsr);
    // nav.idleOn(measure);//this menu will start on idle state, press select to enter menu
}

bool blink(size_t timeOn, size_t timeOff, bool logged) { return millis() % (timeOn + timeOff) < timeOn && logged; }

void loop() {
    nav.poll();
    // digitalWrite(LEDPIN, blink(timeOn,timeOff));
    if (nav.sleepTask) {
        auto result = measure();
        digitalWrite(LEDPIN, blink(100, conf.measurement_delay - 100, result));
    }
}