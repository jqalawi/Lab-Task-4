#include "mbed.h"
#include <cctype>

// Pins
AnalogIn pot(A0);          // Potentiometer
AnalogIn tempSensor(A1);   // LM35 temperature sensor
DigitalIn gasSensor(PE_12); // MQ-2 gas sensor
DigitalOut siren(PE_10);   // Alarm output
UnbufferedSerial serial(USBTX, USBRX, 115200);

// Thresholds
const float TEMP_THRESHOLD = 50.0f;  // °C

// Function prototypes
void showHelp();
float readTemp();
void checkAlarms(float temp, bool gas);

int main() {
    showHelp();
    Timer tmr;
    tmr.start();
    
    while(1) {
        if(tmr.elapsed_time() >= 200ms) {
            tmr.reset();
            
            // Read sensors
            float tempC = readTemp();
            float potVal = pot.read();
            bool gas = !gasSensor;  // Active-low
            
            checkAlarms(tempC, gas);
            
            // Process serial commands
            if(serial.readable()) {
                char cmd;
                serial.read(&cmd, 1);
                
                char buf[64];
                switch(std::tolower(cmd)) {
                    case 'a': sprintf(buf, "Pot: %.2f\r\n", potVal); break;
                    case 'b': sprintf(buf, "LM35: %.2f\r\n", tempC/330); break;
                    case 'c': sprintf(buf, "Temp: %.1f°C\r\n", tempC); break;
                    case 'd': sprintf(buf, "Temp: %.1f°F\r\n", (tempC*9/5)+32); break;
                    case 'e': sprintf(buf, "Temp: %.1f°C | Pot: %.1f°C\r\n", 
                                     tempC, potVal*148+2); break;
                    case 'f': sprintf(buf, "Temp: %.1f°F | Pot: %.1f°F\r\n",
                                     (tempC*9/5)+32, ((potVal*148+2)*9/5)+32); break;
                    case 'g': sprintf(buf, "Gas: %s\r\n", gas ? "DETECTED" : "SAFE"); break;
                    default: showHelp(); continue;
                }
                serial.write(buf, strlen(buf));
            }
        }
    }
}

// Temperature reading function
float readTemp() {
    return tempSensor.read() * 330.0f;  // LM35 conversion (3.3V * 100°C/V)
}

// Alarm checking function
void checkAlarms(float temp, bool gas) {
    static bool tempAlarm = false, gasAlarm = false;
    
    // Temperature alarm logic
    if(temp >= TEMP_THRESHOLD && !tempAlarm) {
        serial.write("ALARM: Temperature Threshold Exceeded!\r\n", 38);
        tempAlarm = true;
    }
    else if(temp < TEMP_THRESHOLD) {
        tempAlarm = false;
    }
    
    // Gas alarm logic
    if(gas && !gasAlarm) {
        serial.write("ALARM: Gas Detected!\r\n", 20);
        gasAlarm = true;
        siren = 1;
    }
    else if(!gas) {
        gasAlarm = false;
        siren = 0;
    }
}

// Help menu function
void showHelp() {
    const char* help = 
        "\r\nAvailable Commands:\r\n"
        "a - Potentiometer raw value\r\n"
        "b - LM35 raw value\r\n"
        "c - Temperature in °C\r\n"
        "d - Temperature in °F\r\n"
        "e - Temp °C + Pot scaled\r\n"
        "f - Temp °F + Pot scaled\r\n"
        "g - Gas sensor status\r\n"
        "Press 'q' in any mode to return\r\n\r\n";
    serial.write(help, strlen(help));
}
