#include "MyMeasurement.h"

MyMeasurement::MyMeasurement(
        const EvaluatableQuantity& voltage,
        const EvaluatableQuantity& current,
        const Quantity& power,
        const Quantity& consumption,
        const RtcDateTime& now) :
        voltage{ voltage },
        current{ current },
        power{ power },
        consumption{ consumption },
        now{ now } {}

bool MyMeasurement::LogToSd() {
    // SdUtility::InitSd(1);
    
    String filename = String(now.Day()) + "-" + String(now.Month()) + ".csv";
    String log = "";
    if (SD.exists(filename) == false) {
        log = "Time [HH:MM:SS],Voltage [V],State [None=0|Low|Fine|High],Current [A],State [None=0|Low|Fine|High],Power [W],Consumption [Wh]\n";
    }
    
    SdFile::dateTimeCallback(SdUtility::dateTime);
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        #if DEBUG == 1
        SerialBoth::println();
        SerialBoth::println("Failed to log the measurement. Write error: " + String(file.getWriteError()));
        #endif
        return false;
    }

    log += (DateTimeFormatter::GetTime(now) + "," +
            String(voltage.get_value()) + "," +
            String(voltage.get_eval()) + "," +
            String(current.get_value()) + "," +
            String(current.get_eval()) + "," +
            String(power.get_value()) + "," + 
            String(consumption.get_value()));
    
    file.println(log);  // Write the log to the file
    if (file.getWriteError() != 0) {
        #if DEBUG == 1
        SerialBoth::println("Failed to log the measurement. Write error: " + String(file.getWriteError()));
        #endif
        file.close();
        return false;
    }

    file.close();
    // SD.end();

    return true;
}