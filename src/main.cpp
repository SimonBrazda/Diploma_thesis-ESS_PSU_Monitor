// Title: A Module for Power Supply Analysis of Electronic Security Systems
// Author: Šimon Brázda
// E-mail: simonbrazda@seznam.cz
// Date: 07/2021
// Description:
// Hardware:
// Repository:

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// Ignore comments in json files
// Must be set before inclusion of ArduinoJson.h
#define ARDUINOJSON_ENABLE_COMMENTS 1

#include <Wire.h>
#include <RtcDS1307.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <menu.h>
#include <SAMDUETimerInterrupt.h>
#include <menuIO/clickEncoderIn.h>
#include <menuIO/serialIO.h>
#include <menuIO/chainStream.h>
#include <menuIO/TFT_eSPIOut.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#include <menuIO/keyIn.h>

#include "Config.h"
#include "TFT_eSPI_touchIn.h"
#include "EEPROM_vars.h"
#include "Quantity.h"
#include "EvaluatableQuantity.h"
#include "SerialBoth.h"
#include "DateTimeFormatter.h"
#include "Measurement.h"
#include "MyMeasurement.h"
#include "SdUtility.h"

unsigned long lastMeasurementTime = 0; // Time of last measurement in miliseconds
uint16_t Timer1_Index = 0;
uint16_t year, month, day, hour, minute, second;

// Color definitions for the menu system
const colorDef<uint16_t> colors[6] MEMMODE={
 // {{disabled normal,disabled selected},{enabled normal,enabled selected, enabled editing}}
    {{(uint16_t)Black,(uint16_t)Black}, {(uint16_t)Black, (uint16_t)Blue,  (uint16_t)Blue}}, //bgColor
    {{(uint16_t)Gray, (uint16_t)Gray},  {(uint16_t)White, (uint16_t)White, (uint16_t)White}}, //fgColor
    {{(uint16_t)White,(uint16_t)Black}, {(uint16_t)Yellow,(uint16_t)Yellow,(uint16_t)Red}}, //valColor
    {{(uint16_t)White,(uint16_t)Black}, {(uint16_t)White, (uint16_t)Yellow,(uint16_t)Yellow}}, //unitColor
    {{(uint16_t)White,(uint16_t)Gray},  {(uint16_t)Black, (uint16_t)Blue,  (uint16_t)White}}, //cursorColor
    {{(uint16_t)White,(uint16_t)Yellow},{(uint16_t)Blue,  (uint16_t)Red,   (uint16_t)Red}}, //titleColor
};

const char *filename = "config.cfg"; // SD library uses 8.3 filenames
Config conf{}; // global configuration object
EEPROM eeprom(0x50, I2C_DEVICESIZE_24LC04);
TFT_eSPI tft{}; // TFT LCD display
RtcDS1307<TwoWire> rtc(Wire); // RTC

// Setup of menu outputs ////////////////////////////////////////////////////////
const int charW = FONT_WIDTH * TEXT_SCALE; // Character width
const int charH = FONT_HEIGHT * TEXT_SCALE; // Character height

const panel panels[] MEMMODE = { { 0, 0, TFT_WIDTH / FONT_WIDTH, TFT_HEIGHT / FONT_HEIGHT } }; // Output dimensions in characters
navNode* nodes[sizeof(panels) / sizeof(panel)]; // navNodes to store navigation status
panelsList pList(panels, nodes, 1); // A list of panels and nodes
idx_t eSpiTops[MAX_DEPTH] = { 0 };
TFT_eSPIOut eSpiOut(tft, colors, eSpiTops, pList, charW, charH + 1); // TFT LCD display output device
idx_t serialTops[MAX_DEPTH] = { 0 };
#if DEBUG == 1
serialOut outSerial(Serial, serialTops);
serialOut outSerialUsb(SerialUSB, serialTops);
MENU_OUTLIST(out, &eSpiOut, &outSerial, &outSerialUsb);
#else
MENU_OUTLIST(out, &eSpiOut);
#endif
/////////////////////////////////////////////////////////////////////////////////

// Hardware interrupt
uint16_t attachDueInterrupt(double microseconds, timerCallback callback, const char* TimerName) {
    DueTimerInterrupt dueTimerInterrupt = DueTimer.getAvailable();
    dueTimerInterrupt.attachInterruptInterval(microseconds, callback);
    uint16_t timerNumber = dueTimerInterrupt.getTimerNumber();
    return timerNumber;
}

// Loads the configuration from a file on SD card and sets default values
bool LoadConfigFromSd(const char *filename, Config &conf) {
    // Mount SD card
    if (SdUtility::InitSd(1) == false) { return false; };
    
    // Open file for reading
    File file = SD.open(filename);

    // Allocate a temporary JsonDocument
    // Use arduinojson.org/v6/assistant to compute your file's capacity.
    StaticJsonDocument<512> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        #if DEBUG == 1
        SerialBoth::println("Failed to read file");
        SerialBoth::println(error.c_str());
        #endif
        file.close();
        SD.end();
        return false;
    }

    // Copy values from the JsonDocument to the Config
    conf.maxVoltage = doc["maxVoltage"] | conf.maxVoltage;
    conf.minVoltage = doc["minVoltage"] | conf.minVoltage;
    conf.maxCurrent = doc["maxCurrent"] | conf.maxCurrent;
    conf.minCurrent = doc["minCurrent"] | conf.minCurrent;
    conf.measurementDelay = doc["measurementDelay"] | conf.measurementDelay;
    conf.sensitivity = doc["sensitivity"] | conf.sensitivity;
    conf.voltageR1 = doc["voltageR1"] | conf.voltageR1;
    conf.voltageR2 = doc["voltageR2"] | conf.voltageR2;
    conf.currentR1 = doc["currentR1"] | conf.currentR1;
    conf.currentR2 = doc["currentR2"] | conf.currentR2;
    conf.resolution = doc["resolution"] | conf.resolution;
    conf.referenceVoltage = doc["referenceVoltage"] | conf.referenceVoltage;
    conf.currentCalibration = doc["currentCalibration"] | conf.currentCalibration;
    conf.currentVoltageCalibration = doc["currentVoltageCalibration"] | conf.currentVoltageCalibration;
    conf.voltageCalibration = doc["voltageCalibration"] | conf.voltageCalibration;
    conf.displayRotation = doc["displayRotation"] | conf.displayRotation;
    conf.timeToSleep = doc["timeToSleep"] | conf.timeToSleep;

    file.close();
    SD.end();
    return true;
}

// Saves the configuration to a file on SD card
bool SaveConfigToSd(const char *filename, const Config &conf) {
    // Mount SD card
    if (SdUtility::InitSd(1) == false) { return false; };

    // Delete existing file, otherwise the configuration is appended to the file
    SD.remove(filename);

    // Synchronize SD time to local time
    SdFile::dateTimeCallback(SdUtility::dateTime);

    // Open file for writing
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        #if DEBUG == 1
        SerialBoth::println("Failed to create file");
        #endif
        SD.end();
        return false;
    }

    // Allocate a temporary JsonDocument
    // Use arduinojson.org/assistant to compute your file capacity the capacity.
    StaticJsonDocument<512> doc;

    // Set the values in the document
    doc["maxVoltage"] = conf.maxVoltage;
    doc["minVoltage"] = conf.minVoltage;
    doc["maxCurrent"] = conf.maxCurrent;
    doc["minCurrent"] = conf.minCurrent;
    doc["measurementDelay"] = conf.measurementDelay;
    doc["sensitivity"] = conf.sensitivity;
    doc["voltageR1"] = conf.voltageR1;
    doc["voltageR2"] = conf.voltageR2;
    doc["currentR1"] = conf.currentR1;
    doc["currentR2"] = conf.currentR2;
    doc["resolution"] = conf.resolution;
    doc["referenceVoltage"] = conf.referenceVoltage;
    doc["currentCalibration"] = conf.currentCalibration;
    doc["currentVoltageCalibration"] = conf.currentVoltageCalibration;
    doc["voltageCalibration"] = conf.voltageCalibration;
    doc["displayRotation"] = conf.displayRotation;
    doc["timeToSleep"] = conf.timeToSleep;

    // Serialize JSON to file with spaces and line-breaks
    if (serializeJsonPretty(doc, file) == 0) {
        #if DEBUG == 1
        SerialBoth::println("Failed to write the config to the file");
        #endif
        file.close();
        SD.end();
        return false;
    }

    file.close();
    SD.end();
    return true;
}

// Prints content of a file from SD card to the Serial
#if DEBUG == 1
bool PrintSdFile(const char *filename) {
    // Mount SD card
    if (SdUtility::InitSd(1) == false) { return false; };

    // Open file for reading
    File file = SD.open(filename);
    if (!file) {
        SerialBoth::println("Failed to read file");
        SD.end();
        return false;
    }

    // Extract each characters one by one
    while (file.available()) {
        SerialBoth::print((char)file.read());
    }
    SerialBoth::println();

    file.close();
    SD.end();
    return true;
}
#endif

// Reads entire config from EEPROM to conf object
void ReadConfigFromEeprom(EEPROM& eeprom, Config& conf) {
    eeprom.get(10, conf);
}

// Updates config object into EEPROM (Overrides only varibles that changed)
void UpdateConfigToEeprom(EEPROM& eeprom, const Config& conf) {
    eeprom.put(10, conf);
}

// Declarations of menu functions
result ShowEvent(eventMask e,navNode& nav,prompt& item);
result EjectSd();
result MountSd();
result LoadConfig();
result SaveConfig();
result LoadDate(eventMask e);
result LoadTime(eventMask e);
result LoadDateTime(eventMask e);
result CalibrateTouch();
#if DEBUG == 1
result PrintConfig();
result DumpEeprom();
#endif

// Custom field print implementing a customized menu component
// This numeric field prints formatted date/time
template<typename T>
class TimeField : public menuField<T> {
public:
    TimeField(constMEM menuFieldShadow<T>& shadow):menuField<T>(shadow) {}
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

// Set date menu option
PADMENU(date_m,"",LoadDate,anyEvent,noStyle
  ,altFIELD(TimeField, year, "", "/", 1900, 3000, 1, 0,doNothing, anyEvent, noStyle)
  ,altFIELD(TimeField, month, "", "/", 1, 12, 1, 0, doNothing, anyEvent, wrapStyle)
  ,altFIELD(TimeField, day, "", "", 1, 31, 1, 0, doNothing, anyEvent, wrapStyle)
);

// Set time menu option
PADMENU(time_m,"",LoadTime,anyEvent,noStyle
  ,altFIELD(TimeField, hour, "", ":", 0, 23, 1, 0, doNothing, anyEvent, noStyle)
  ,altFIELD(TimeField, minute, "", ":", 0, 59, 1, 0, doNothing, anyEvent, wrapStyle)
  ,altFIELD(TimeField, second, "", "", 0, 59, 1, 0, doNothing, anyEvent, wrapStyle)
);

// Settings menu
MENU(settingsMenu,"Settings",LoadDateTime,anyEvent,wrapStyle
  ,FIELD(conf.maxVoltage, "Max", " V", 0, 30, 1, 0.01, doNothing, noEvent, noStyle)
  ,FIELD(conf.minVoltage, "Min", " V", 0, 30, 1, 0.01, doNothing, noEvent, noStyle)
  ,FIELD(conf.maxCurrent, "Max", " A", 0, 10, 1, 0.01, doNothing, noEvent, noStyle)
  ,FIELD(conf.minCurrent, "Min", " A", 0, 10, 1, 0.01, doNothing, noEvent, noStyle)
  ,FIELD(conf.measurementDelay, "Delay", " ms", 100, 60000, 1000, 100, doNothing, noEvent, noStyle) 
  ,SUBMENU(date_m)
  ,SUBMENU(time_m)
  ,OP("Calibrate Touch", CalibrateTouch, enterEvent)
  ,EXIT("<Back")
);

#if DEBUG == 0
// Main menu
MENU(mainMenu, "Main Menu", doNothing, noEvent, wrapStyle
    ,EXIT("Measure")
    ,SUBMENU(settingsMenu)
    ,OP("Eject SD", EjectSd, enterEvent)
    ,OP("Mount SD", MountSd, enterEvent)
    ,OP("Load Config", LoadConfig, enterEvent)
    ,OP("Save Config", SaveConfig, enterEvent)
);
#endif

#if DEBUG == 1
// Debug main menu
MENU(mainMenu, "Main Menu", doNothing, noEvent, wrapStyle
    ,EXIT("Measure")
    ,SUBMENU(settingsMenu)
    ,OP("Eject SD", EjectSd, enterEvent)
    ,OP("Mount SD", MountSd, enterEvent)
    ,OP("Load Config", LoadConfig, enterEvent)
    ,OP("Save Config", SaveConfig, enterEvent)
    ,OP("Print Config", PrintConfig, enterEvent)
    ,OP("Dump EEPROM", DumpEeprom, enterEvent)
);
#endif

// Menu navigation root object
extern navRoot nav;

// Setup of menu inputs ////////////////////////////////////////////////////////
// ClickEncoder decrlaration
ClickEncoder clickEncoder = ClickEncoder(ENCODER_A, ENCODER_B, ENCODER_BUTTON, ENCODER_STEPS);
ClickEncoderStream encStream(clickEncoder, 1);

TFT_eSPI_touchIn touch(tft, nav, eSpiOut); // Touch screen
#if DEBUG == 1
serialIn inSerial(Serial);
serialIn inSerialUSB(SerialUSB); // Native USB serial
MENU_INPUTS(in, &inSerial, &inSerialUSB, &touch, &encStream); // Concatenates input streams into a single one stream "in"
#else
MENU_INPUTS(in, &touch, &encStream); // Concatenates input streams into a single one stream "in"
#endif
////////////////////////////////////////////////////////////////////////////////

void timerIsr() {clickEncoder.service();}

// Menu navigation root object controls all navigation coordinating inputs and outputs lists along with a menu definition
// Controls each depth layer of the navigation process to a maximum of the defined MAX_DEPTH
NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);

// Measures voltage, current, power and consumption
bool measure() {
    if (millis() - lastMeasurementTime > conf.measurementDelay) {
        lastMeasurementTime = millis();
        #if DEBUG == 0
        EvaluatableQuantity voltage{ Measurement::MeasureVoltage(VOLTAGE_INPUT_PIN, conf), "V", conf.minVoltage, conf.maxVoltage };
        EvaluatableQuantity current{ Measurement::MeasureCurrent(CURRENT_INPUT_PIN, conf), "A", conf.minCurrent, conf.maxCurrent };
        Quantity power{ Measurement::Power(voltage.get_value(), current.get_value()), "W" };
        Quantity consumption{ Measurement::Consumption(power.get_value(), conf), "Wh" };
        RtcDateTime now{ rtc.GetDateTime() };
        MyMeasurement measurement{ voltage, current, power, consumption, now };

        Measurement::print(eSpiOut, measurement.now, 0, measurement.voltage,
                                                        measurement.current,
                                                        measurement.power,
                                                        measurement.consumption);
        #endif

        #if DEBUG == 1
        RtcDateTime now{ rtc.GetDateTime() };
        eSpiOut.setCursor(0, 0);
        eSpiOut.print(DateTimeFormatter::GetTime(now));
        EvaluatableQuantity voltage{ Measurement::DebugMeasureVoltage(eSpiOut, VOLTAGE_INPUT_PIN, conf), "V", conf.minVoltage, conf.maxVoltage };
        EvaluatableQuantity current{ Measurement::DebugMeasureCurrent(eSpiOut, CURRENT_INPUT_PIN, conf), "A", conf.minCurrent, conf.maxCurrent };
        Quantity power{ Measurement::Power(voltage.get_value(), current.get_value()), "W" };
        Quantity consumption{ Measurement::Consumption(power.get_value(), conf), "Wh" };
        eSpiOut.setCursor(0, 11);
        eSpiOut.print(String(power.get_value()) + " " + power.get_unit());
        eSpiOut.setCursor(0, 12);
        eSpiOut.print(String(consumption.get_value()) + " " + consumption.get_unit());
        MyMeasurement measurement{ voltage, current, power, consumption, now };
        #endif

        auto result = measurement.LogToSd();

        eSpiOut.setCursor(0, LAST_DISPLAY_LINE);
        if (result == false) {
            eSpiOut.print("Log failed");
        } else {
            eSpiOut.print("Log succeeded");
        }
        tft.endWrite();
        return result;
    }
    return false;
}

result EjectSd() {
    SD.end();
    eSpiOut.clear();
    eSpiOut.println("SD card unmounted");
    #if DEBUG == 1
    SerialBoth::println("SD card unmounted");
    #endif
    return proceed;
}

result MountSd() {
    eSpiOut.clear();
    if (SdUtility::InitSd(1)) {
        eSpiOut.println("SD card mounted");
    } else {
        eSpiOut.println("SD card failed to mount");
    }
    return proceed;
}

result LoadConfig() {
    eSpiOut.clear();
    if (LoadConfigFromSd(filename, conf)) {
        eSpiOut.println("Config loaded");
    } else {
        eSpiOut.println("Could not load the Config");
    }
    return proceed;
}

result SaveConfig() {
    eSpiOut.clear();
    if (SaveConfigToSd(filename, conf)) {
        eSpiOut.println("Config saved to SD card");
    } else {
        eSpiOut.println("Config could not be saved to SD card");
    }
    UpdateConfigToEeprom(eeprom, conf);
    return proceed;
}

result LoadDate(eventMask e) {
    auto now = rtc.GetDateTime();
    if (e == eventMask::exitEvent) {
        now = RtcDateTime(year, month, day, now.Hour(), now.Minute(), now.Second());
        rtc.SetDateTime(now);
    }
    return proceed;
}

result LoadTime(eventMask e) {
    auto now = rtc.GetDateTime();
    if (e == eventMask::enterEvent || e == eventMask::focusEvent) {
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

result LoadDateTime(eventMask e) {
    auto now = rtc.GetDateTime();
    if (e == eventMask::enterEvent) {
        year = now.Year();
        month = now.Month();
        day = now.Day();
        hour = now.Hour();
        minute = now.Minute();
        second = now.Second();
    }
    return proceed;
}

// Calibrates touch screen
result CalibrateTouch() {
    uint16_t calData[5];

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextSize(2);
    tft.println("Touch corners as indicated");
    tft.println();
    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    #if DEBUG == 1
    SerialBoth::print("Touch screen calibration data: { ");
    for (uint8_t i = 0; i < 5; i++) {
        SerialBoth::print(calData[i]);
        if (i < 4) { SerialBoth::print(", "); }
    }
    SerialBoth::println(" };");
    #endif

    tft.setTouch(calData);
    tft.println("Calibration complete!");
    delay(4000);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(TEXT_SCALE);
    return proceed;
}

#if DEBUG == 1
// Prints config to Serials
result PrintConfig() {
    SerialBoth::println();
    conf.print();
    return proceed;
}

// Dumps EEPROM memory to Serials
result DumpEeprom() {
    SerialBoth::println();
    SerialBoth::println();
    eeprom.dumpEeprom(0, 256);
    return proceed;
}
#endif

void setup() {
    // Initialize pin modes
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(ENCODER_BUTTON,INPUT_PULLUP);
    pinMode(CURRENT_INPUT_PIN, INPUT);
    pinMode(VOLTAGE_INPUT_PIN, INPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // Set resolution of A/D converter to 12 bits
    analogReadResolution(12);

    // Initilize communication over serial lines
    #if DEBUG == 1
    Serial.begin(9600);
    SerialUSB.begin(9600);
    while(!Serial);
    #endif

    // Initialize TFT LCD display
    tft.init();
    tft.setRotation(2);
    tft.setTextWrap(true);
    tft.setTextSize(TEXT_SCALE);
    tft.fillScreen(Black);
    tft.setTextColor(White, Black);

    // Calibrate touch screen
    // uint16_t calData[5] = { 102, 3712, 67, 3863, 2 };
    // tft.setTouch(calData);

    // Initialize EEPROM
    eeprom.begin();
    #if DEBUG == 1
    if (eeprom.isConnected() == false) {
        SerialBoth::println("Can't initialize EEPROM");
    }
    #endif

    // Load configuration from SD card ///////////////////////////////////////////////
    #if DEBUG == 1
    SerialBoth::println("Loading configuration...");
    #endif
    if(LoadConfigFromSd(filename, conf) == false) { // If loading from SD fails...
        uint8_t eepromSet = 0;
        eeprom.get(0, eepromSet); // Read from EEPROM if config was ever written to it
        if (eepromSet == 1) {
            ReadConfigFromEeprom(eeprom, conf); // If it was, load it
        }
        else {
            eepromSet = 1; // If it wasn't, set it as it was
            eeprom.put(0, eepromSet);
        }
    }
    UpdateConfigToEeprom(eeprom, conf); // Save config to EEPROM

    #if DEBUG == 1
    conf.print();
    #endif
    //////////////////////////////////////////////////////////////////////////////////

    // Initialize RTC ////////////////////////////////////////////////////////////////
    rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    if (!rtc.IsDateTimeValid()) {
        if (rtc.LastError() != 0) {
            // Communications error. See https://www.arduino.cc/en/Reference/WireEndTransmission for what the number means
            #if DEBUG == 1
            SerialBoth::println("RTC communications error = " + String(rtc.LastError()));
            #endif
        } else {
            // Common Causes:
            //    1) first time you ran and the device wasn't running yet
            //    2) the battery on the device is low or even missing
            #if DEBUG == 1
            SerialBoth::println("RTC lost confidence in the DateTime!");
            #endif
            // Following line sets the RTC to the date & time this sketch was compiled
            // It will also reset the valid flag internally unless the Rtc device is having an issue
            rtc.SetDateTime(compiled);
        }
    }

    if (!rtc.GetIsRunning()) {
        #if DEBUG == 1
        SerialBoth::println("RTC was not actively running, starting now");
        #endif
        rtc.SetIsRunning(true);
    }

    RtcDateTime now = rtc.GetDateTime();
    if (now < compiled) {
        SerialBoth::println("RTC is older than compile time! (Updating DateTime)");
        rtc.SetDateTime(compiled);
    }
    else if (now > compiled) {
        SerialBoth::println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) {
        SerialBoth::println("RTC is the same as compile time! (not expected but all is fine)");
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    rtc.SetSquareWavePin(DS1307SquareWaveOut_Low);
    /////////////////////////////////////////////////////////////////////////////////

    // Initialise menu system ///////////////////////////////////////////////////////
    #if DEBUG == 1
    SerialBoth::println("Use keys '+', '-', '*', '/'");
    SerialBoth::println("to control the menu navigation");
    #endif

    Timer1_Index = attachDueInterrupt(1000, timerIsr, "Timer1");
    nav.showTitle = false; // Disables menu title
    nav.timeOut = conf.timeToSleep; // Set the menu to go into idle state after specified time of inactivity 
    delay(2000);
    nav.idleOn(); // Force menu to go into an idle (sleep) state on next poll
    /////////////////////////////////////////////////////////////////////////////////
}

void loop() {
    nav.poll(); // Checks menu inputs and updates menu outputs
    if (nav.sleepTask) {
        // If in sleep state, make a measurement
        measure();
    }
}