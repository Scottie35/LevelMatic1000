/*
  Arduino Uno Ultrasonic Fuel Gauge / Liquid Level Sensor System
  by Scott Ogrin
  MIT License
*/

// include the library code:
#include <LiquidCrystal.h>
#include <math.h>

// Init vars
// **************
//   CHANGE ME!
// **************
const int tankEmptyDepth = 153; // This MUST be no greater than 450 cm (500cm for the HC-SR04)!
const int tankFullDepth = 15; // This should be at least 25 cm, if possible (2cm for the HC-SR04)

// Change the above tankEmptyDepth and tankFullDepth constants to be the distance (in centimeters):
//    - tankEmptyDepth  = between the ultrasonic sensor and the empty level (i.e. bottom of tank)
//    - tankFullDepth   = between the sensor and the liquid when the tank is full (MINIMUM 25cm)
// Note that the ultrasonic sensor works only between 25cm and 450cm, so the min tankFullDepth = 25.
// For my tank, the tankFullDepth = 15, which is okay... BUT it means that when the tank is full, 
// I will probably get incorrect readings until the level drops 10cm. This isn't a problem in my case 
// since I I don't care about accurate level readings when the fuel tank is full! But it means that 
// after getting the tank filled, the level will read near-empty or "Error: Timeout".
//
// You could also use the HC-SR04, which is larger but has a min depth of 2cm and max of 500cm.
// Note however that it's not waterproof! I chose the JSN-SR04T-2.0 for that reason.
//
// Note also that you might want to set tankEmptyDepth to be less than the bottom of your tank,
// esp if you have a vertical feed sucking liquid from, say, 5cm above the bottom of the tank.
// For example, my tank is 163cm deep from the sensor, so I set tankEmptyDepth to 153. This ensures
// that when my LevelMatic reads 0%, I should have 10cm of fuel left in the tank.
//
// If measuring in inches: 1 inch = 2.54 cm

// These vars hold the current and last-measured level from the sensor
int currentLevel = 0;
int lastLevel = 0;

// These vars are for showPartialBarChar() and showCurrentLevel()
int done = 0;
char levelTxt[] = "Current level:";

// Var for showError
// This error means the ultrasound unit couldn't do a measurement, and timed out
// Usually that means the sensor is at a weird angle, too close to the liquid, or
// the ultrasound waves are bouncing off the walls of the tank.
char timeoutErrorTxt[] = "ERROR: Timeout";

// Var for echo response from ultrasonic sensor board
unsigned long timeHigh;

// Custom chars for LCD
byte barEmpty[8] = {
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
};
byte barOne[8] = {
  B11111,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B11111,
};
byte barTwo[8] = {
  B11111,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11111,
};
byte barThree[8] = {
  B11111,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11111,
};
byte barFour[8] = {
  B11111,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11111,
};

// constants for ultrasonic sensor board IO pins
const int trigPin = 8, echoPin = 9;

// constants for LCD IO pins
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {

  // Set up IO pins for ultrasonic sensor board
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Set trigger pin for sensor to 0 (aka "do nothing yet")
  digitalWrite(trigPin, LOW);
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Set custom chars 0-7
  lcd.createChar(0, barEmpty);
  lcd.createChar(1, barOne);
  lcd.createChar(2, barTwo);
  lcd.createChar(3, barThree);
  lcd.createChar(4, barFour);
  lcd.clear();
}

void loop() {
  // Do level scan with ultrasonic board

  // Start a scan - trigger pin must be high for at least 10us
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Get time echo pin is high using pulseIn, which returns value in microseconds
  // Default timeout of 1s is more than enough for 8 pulses, and we're not in a hurry
  timeHigh = pulseIn(echoPin, HIGH);

  if (timeHigh == 0) {
    // Oops! Timeout...
    showError();
  } else {
    // Calculate level
    // Assume 343 m/s for the speed of the 40kHz sound waves in air at standard temperature and pressure
    // It's 58 us/cm, which we get from:
    //    (343 m/s * (1s / 1000000 us) * (100cm / 1m)) / 2 = 0.01715 cm / us
    // Must divide by 2 because sound wave travels to liquid, and back
    // Invert that to get:
    //    1 / 0.01715 cm/us = 58.309038 us/cm
    // Note resolution of ultrasonic sensor is +/- 0.5cm
    currentLevel = round(timeHigh / 58);
    if (currentLevel > tankEmptyDepth) {
      // If level is lower than empty, show 0%
      // This is useful if you want to have "empty" be "still 10cm of liquid left in tank"
      currentLevel = tankEmptyDepth;
    } else if (currentLevel < tankFullDepth) {
      // If level is higher than full, show 100%
      // This is useful since "full" level may vary when tank is refilled
      currentLevel = tankFullDepth;
    }
    // Don't redraw screen if level is the same as last time
    if (currentLevel != lastLevel) {
      lastLevel = currentLevel;
      showCurrentLevel();
    }
    
  }

  // Delay 2s between scans
  delay(2000);
}

void showError() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(timeoutErrorTxt);
}

void showPartialBarChar(int val) {
  switch (val) {
    case 0:
      lcd.write(byte(0)); // barEmpty
      ++done;
      break;
    case 1:
      lcd.write(byte(1)); // one bar
      ++done;
      break;
    case 2:
      lcd.write(byte(2)); // two bars
      ++done;
      break;
    case 3:
      lcd.write(byte(3)); // three bars
      ++done;
      break;
    case 4:
      lcd.write(byte(4)); // four bars
      ++done;
      break;
  }
}

void showCurrentLevel() {
  // Get integer between 0 and 50 for bar graph
  // We have 10 progress bar characters, and each character can have 0-5 vertical columns of pixels
  // Subtracting tankFullDepth gives us a precise ratio between full/empty, as if the ultrasonic sensor
  // would be 0 cm away from the liquid level when the tank is full.
  // Also, currentLevel contains height of "emptiness" above the liquid, so to get liquid level we do:
  //   abs(1 - currentLevel/tankEmptyDepth).
  float ratio = 1 - ((float)currentLevel - (float)tankFullDepth) / ((float)tankEmptyDepth - (float)tankFullDepth);
  ratio = abs(ratio);
  int textLevelInt = round(ratio * 100.0);
  int levelInt = round(ratio * 50.0);
  int fulls = 0;

  // Reset done
  done = 0;

  // Display text above progress bar
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(levelTxt);
  // Display progress bar based on levelInt
  lcd.setCursor(0, 1);
  // Draw progress bar for XX%
  fulls = levelInt / 5;
  if (fulls == 0) {
    // First char on bar is a partial char with 0-4 vertical columns of pixels
    showPartialBarChar(levelInt);
  } else {
    for (int i = 0; i < fulls; ++i) {
      lcd.write(255); // full
      ++done;
    }
  }
  if (done < 10) {
    if (fulls > 0) {
      // Here we may have a partial char with 0-4 vertical columns of pixels
      showPartialBarChar(levelInt - (fulls * 5));
    }
    // Here we may have blank boxes left
    if (done < 10) {
      // We have empty boxes to draw
      for (int i = 0; i < 10 - done; ++i) {
        lcd.write(byte(0)); // barEmpty
      }
    }
  }

  // Lastly, print percentage:
  if (textLevelInt == 100) {
    lcd.setCursor(12, 1);
    lcd.print("100%");
  } else if (textLevelInt < 10) {
    lcd.setCursor(14, 1);
    lcd.print(textLevelInt);
    lcd.print("%");
  } else {
    lcd.setCursor(13, 1);
    lcd.print(textLevelInt);
    lcd.print("%");
  }
}
