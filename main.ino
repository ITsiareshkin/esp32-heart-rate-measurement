/*
  Author: Ivan Tsiareshkin, xtsiar00
*/

#include <SPI.h>
#include <Wire.h>

// OLED libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Sensor libraries
#include "MAX30105.h"
#include "heartRate.h"

// OLED specs and pins defines
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_CLK    18
#define OLED_MOSI   23
#define OLED_RESET  17
#define OLED_DC     16
#define OLED_CS     5


MAX30105 Sensor;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Small heart logo
static const unsigned char PROGMEM small_bmp[] = {
0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,
0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00 };

// Big heart logo
static const unsigned char PROGMEM big_bmp[] = { 
0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00 };

// Global variables for heart rate calculation
const byte RATE_SIZE = 4; // Heart rates array sise
byte rates[RATE_SIZE]; // Heart rates array
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred
float beatsPerMinute;
int beatAvg; // Average heart rate calculated from the rate of the last 4 heart beats

void setup() { 
    Serial.begin(115200); // Upload speed

    // Initialize display
    if(!display.begin(SSD1306_SWITCHCAPVCC)) {
        Serial.println(F("SSD1306 allocation failed."));
    }
    
    display.clearDisplay(); 
    display.display();
    delay(250);
    
    // Initialize sensor
    // Use default I2C port, 400kHz speed
    if (!Sensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println(F("MAX30105 was not found."));
    }
    Sensor.setup(); //Configure sensor with default settings
    Sensor.setPulseAmplitudeRed(0x0A); // Turn Red LED to low to indicate sensor is running
    Sensor.setPulseAmplitudeGreen(0); // Turn off Green LED
}

void loop() {
    long irValue = Sensor.getIR(); // Reading the IR value to check if there is a finger on the sensor or not
    if(irValue >= 50000) { // Check if a finger is detected
        display.clearDisplay();
        display.drawBitmap(5, 5, small_bmp, 24, 21, SSD1306_WHITE); // Draw small heart

        display.setTextSize(1);                                
        display.setTextColor(SSD1306_WHITE); 
        display.setCursor(50, 0);                
        display.println("Heart rate"); 

        display.setTextSize(2);             
        display.setCursor(75, 20);                
        display.println(beatAvg); // Display average heart rate

        display.display();
        
        if (checkForBeat(irValue) == true) { // If a heart beat is detected                                     
            long delta = millis() - lastBeat; // Measure duration between two beats
            lastBeat = millis();
    
            beatsPerMinute = 60 / (delta / 1000.0); // Calculating the BPM
      
            if (beatsPerMinute < 255 && beatsPerMinute > 20) { // To calculate the average we strore 4 values then apply formula to calculate the average     
              rates[rateSpot++] = (byte)beatsPerMinute; // Store reading in the array
              rateSpot %= RATE_SIZE; // Wrap variable
        
              // Take average of readings
              beatAvg = 0;
              for (byte x = 0 ; x < RATE_SIZE ; x++) {
                  beatAvg += rates[x];
              }
              beatAvg /= RATE_SIZE;
            }

            display.clearDisplay(); 
            display.drawBitmap(0, 0, big_bmp, 32, 32, SSD1306_WHITE); // Draw big heart

            display.setTextSize(1);                             
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(50, 0);                
            display.println("Heart rate");

            display.setTextSize(2);             
            display.setCursor(75, 20);                
            display.println(beatAvg); // Display average heart rate

            display.display();
            delay(150); 
        }
    
    } else { // Finger is not detected 
        display.clearDisplay();
        
        display.setTextSize(1);                    
        display.setTextColor(SSD1306_WHITE);  
                   
        display.setCursor(30,5);                
        display.println("Please put "); 
        
        display.setCursor(30,15);
        display.println("your finger ");  
        
        display.display();
    }
}
