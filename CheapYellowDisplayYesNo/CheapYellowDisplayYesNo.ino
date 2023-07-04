/*******************************************************************
    A version of the Yes No button from Bluey on the CYD

    https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/

    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// Make sure to copy the UserSetup.h file into the library as
// per the Github Instructions here: https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/SETUP.md#library-configuration

// ----------------------------
// Standard Libraries
// ----------------------------

#include <SPI.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <XPT2046_Touchscreen.h>
// A library for interfacing with the touch screen
//
// Can be installed from the library manager (Search for "XPT2046")
//https://github.com/PaulStoffregen/XPT2046_Touchscreen

#include <TFT_eSPI.h>
// A library for interfacing with LCD displays
//
// Can be installed from the library manager (Search for "TFT_eSPI")
//https://github.com/Bodmer/TFT_eSPI

#include "Audio.h"
// A library for playing music on the DAC of the ESP32
//
// Not available in library manager, so needs to be manually downloaded
// https://github.com/schreibfaul1/ESP32-audioI2S

// ----------------------------
// Touch Screen pins
// ----------------------------

// The CYD touch uses some non default
// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// ----------------------------

SPIClass mySpi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

TFT_eSPI tft = TFT_eSPI();

Audio audio(true, I2S_DAC_CHANNEL_LEFT_EN);

#define YELLOW_CASE 0xE6F1 //pale yellow
#define YES_BUTTON 0x86B3 //pale green
#define YES_BUTTON_TEXT 0x548B //darker green
#define NO_BUTTON 0xC14C //rouge?
#define NO_BUTTON_TEXT 0x70E8 //Maroon

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

void setup() {
  Serial.begin(115200);

  // Initialise SPIFFS, if this fails try .begin(true)
  // NOTE: I believe this formats it though it will erase everything on
  // spiffs already! In this example that is not a problem.
  // I have found once I used the true flag once, I could use it
  // without the true flag after that.
  bool spiffsInitSuccess = SPIFFS.begin(false) || SPIFFS.begin(true);
  if (!spiffsInitSuccess)
  {
    Serial.println("SPIFFS initialisation failed!");
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }

  // Start the SPI for the touch screen and init the TS library
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(0);

  // Start the tft display and set it to black
  tft.init();

  audio.forceMono(true);
  audio.setVolume(10);

  //tft.setRotation(1); //This is the display in landscape
  drawYesNoButton();
}

void drawYesNoButton() {
  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);

  int displayCenter = SCREEN_WIDTH / 2; // center of display

  int yDivisions = SCREEN_HEIGHT / 5; //should be 64
  int yPadding = yDivisions / 2; //should be 32

  int caseRadius = yDivisions + yDivisions / 4;

  int x = 0;
  int y = 0;

  // draw case
  // -- Top circle
  x = displayCenter;
  y = yPadding + yDivisions;
  tft.fillCircle(x, y, caseRadius, YELLOW_CASE);
  // -- Bottom circle
  x = displayCenter;
  y = yPadding + (yDivisions * 3);
  tft.fillCircle(x, y, caseRadius, YELLOW_CASE);
  // -- Rectangle in the middle
  x = displayCenter - caseRadius;
  y = yPadding + yDivisions;
  tft.fillRect(x, y, (caseRadius * 2) + 1, yDivisions * 2, YELLOW_CASE);
  // end of case

  // draw buttons
  int heightOfCase = yDivisions * 4;
  int buttonDivisions = heightOfCase / 3;
  int buttonRadius = buttonDivisions / 2;
  int buttonPadding = buttonDivisions / 3;
  // -- Yes Button
  x = displayCenter;
  y = yPadding + buttonPadding + buttonRadius;
  tft.fillCircle(x, y, buttonRadius, YES_BUTTON);
  tft.setTextColor(YES_BUTTON_TEXT, YES_BUTTON);
  tft.drawCentreString("Y", x, y - 12, 4);

  // -- No Button
  x = displayCenter;
  y = yPadding + (buttonPadding * 2) + (buttonRadius * 3);
  tft.fillCircle(x, y, buttonRadius, NO_BUTTON);
  tft.setTextColor(NO_BUTTON_TEXT, NO_BUTTON);
  tft.drawCentreString("N", x, y - 12, 4);

}

void printTouchToSerial(TS_Point p) {
  Serial.print("Pressure = ");
  Serial.print(p.z);
  Serial.print(", x = ");
  Serial.print(p.x);
  Serial.print(", y = ");
  Serial.print(p.y);
  Serial.println();
}

bool audioPlaying = false;

unsigned long touchCheckDue = 0;

void loop() {
  audio.loop();
  if (millis() > touchCheckDue) {
    if (ts.tirqTouched() && ts.touched()) {
      TS_Point p = ts.getPoint();
      printTouchToSerial(p);
      touchCheckDue = millis() + 100;
      if (p.x > 1000 && p.x < 3000) {
        if (p.y > 850 && p.y < 3150) {
          audioPlaying = true;
          if (p.y < 2000) {
            Serial.println("Yes Press");
            audio.connecttoFS(SPIFFS, "/yes.wav");
          } else {
            Serial.println("No Press");
            audio.connecttoFS(SPIFFS, "/no.wav");
          }
        }
      }

    }

  }
}

void audio_eof_mp3(const char *info) { //end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
  audioPlaying = false;
}
