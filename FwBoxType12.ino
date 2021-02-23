//
// Copyright (c) 2020 Fw-Box (https://fw-box.com)
// Author: Hartman Hsieh
//
// Description :
//   None
//
// Connections :
//
// Required Library :
//

#include <Wire.h>
#include "FwBox.h"
#include "FwBox_PMSX003.h"
#include "SoftwareSerial.h"
#include <U8g2lib.h>
#include "FwBox_UnifiedLcd.h"
#include "FwBox_NtpTime.h"

#define DEVICE_TYPE 12
#define FIRMWARE_VERSION "1.1.1"

#define PIN_LED 2

#define ANALOG_VALUE_DEBOUNCING 8

//
// Debug definitions
//
#define FW_BOX_DEBUG 0

#if FW_BOX_DEBUG == 1
  #define DBG_PRINT(VAL) Serial.print(VAL)
  #define DBG_PRINTLN(VAL) Serial.println(VAL)
  #define DBG_PRINTF(FORMAT, ARG) Serial.printf(FORMAT, ARG)
  #define DBG_PRINTF2(FORMAT, ARG1, ARG2) Serial.printf(FORMAT, ARG1, ARG2)
#else
  #define DBG_PRINT(VAL)
  #define DBG_PRINTLN(VAL)
  #define DBG_PRINTF(FORMAT, ARG)
  #define DBG_PRINTF2(FORMAT, ARG1, ARG2)
#endif

//
// Global variable
//
const char* WEEK_DAY_NAME[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}; // Days Of The Week 

//
// LCD1602
//
FwBox_UnifiedLcd* Lcd = 0;

//
// OLED 128x128
//
U8G2_SSD1327_MIDAS_128X128_1_HW_I2C* u8g2 = 0;

SoftwareSerial SerialSensor(13, 15); // RX:D7, TX:D8

FwBox_PMSX003 Pms(&SerialSensor);

unsigned long ReadingTime = 0;

int DisplayMode = 0;

void setup()
{
  Serial.begin(9600);

  //
  // Initialize the fw-box core (early stage)
  //
  fbEarlyBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  //
  // Set pin mode
  //
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH); // Turn off the LED

  //
  // Initialize the LCD1602
  //
  Lcd = new FwBox_UnifiedLcd(16, 2);
  if(Lcd->begin() != 0) {
    //
    // LCD1602 doesn't exist, delete it.
    //
    delete Lcd;
    Lcd = 0;
    DBG_PRINTLN("LCD1602 initialization failed.");
  }

  //
  // Detect the I2C address of OLED.
  //
  Wire.beginTransmission(0x78>>1);
  if (Wire.endTransmission() == 0) {
    u8g2 = new U8G2_SSD1327_MIDAS_128X128_1_HW_I2C(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);  /* Uno: A4=SDA, A5=SCL, add "u8g2.setBusClock(400000);" into setup() for speedup if possible */
    u8g2->begin();
    u8g2->enableUTF8Print();
    u8g2->setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
    DBG_PRINTLN("OLED 128x128 is detected.");
  }
  else {
    DBG_PRINTLN("U8G2_SSD1327_MIDAS_128X128_1_HW_I2C is not found.");
    u8g2 = 0;
  }

  //
  // Display the screen
  //
  display(analogRead(A0)); // Read 'A0' to change the display mode.

  //
  // Initialize the fw-box core
  //
  fbBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  //
  // Initialize the PMSX003 Sensor
  //
  Pms.begin();

  //
  // Sync NTP time
  //
  FwBox_NtpTimeBegin();

} // void setup()


void loop()
{
  if ((millis() - ReadingTime) > 2000) {
    //
    // Read the sensors
    //
    if (read() == 0) { // Success
      if ((Pms.pm1_0() == 0) && (Pms.pm2_5() == 0) && (Pms.pm10_0() == 0)) {
        DBG_PRINTLN("Invalid values");
      }
      else {
        FwBoxIns.setValue(0, Pms.pm1_0());
        FwBoxIns.setValue(1, Pms.pm2_5());
        FwBoxIns.setValue(2, Pms.pm10_0());
      }
    }

    ReadingTime = millis();
  }

  //
  // Display the screen
  //
  display(analogRead(A0)); // Read 'A0' to change the display mode.

  //
  // Run the handle
  //
  fbHandle();

} // END OF "void loop()"

uint8_t read()
{
  //
  // Running readPms before running pm2_5, temp, humi and readDeviceType.
  //
  if(Pms.readPms()) {
    if(Pms.readDeviceType() == FwBox_PMSX003::PMS3003) {
      DBG_PRINTLN("PMS3003 is detected.");
      DBG_PRINT("PM1.0=");
      DBG_PRINTLN(Pms.pm1_0());
      DBG_PRINT("PM2.5=");
      DBG_PRINTLN(Pms.pm2_5());
      DBG_PRINT("PM10=");
      DBG_PRINTLN(Pms.pm10_0());
      return 0; // Success
    }
  }

  DBG_PRINTLN("PMS data format is wrong.");
  return 1; // Error
}

void display(int analogValue)
{
  //
  // Draw the LCD 1602
  //
  if (Lcd != 0) {
    //
    // Change the display mode according to the value of PIN - 'A0'.
    //
    DisplayMode = getDisplayMode(5, analogValue);
    //DBG_PRINTF("analogValue=%d\n", analogValue);
    //DBG_PRINTF("DisplayMode=%d\n", DisplayMode);

    switch (DisplayMode) {
      case 1:
        LcdDisplayType1();
        break;
      case 2:
        LcdDisplayType2();
        break;
      case 3:
        LcdDisplayType3();
        break;
      case 4:
        LcdDisplayType4();
        break;
      case 5:
        LcdDisplayType5();
        break;
      default:
        LcdDisplayType1();
        break;
    }
  }

  //
  // Draw the OLED
  //
  if(u8g2 != 0) {
    u8g2->setFont(u8g2_font_unifont_t_chinese1);
    u8g2->firstPage();
    do {
      String line0 = String(Pms.pm1_0()) + " " + FwBoxIns.getValUnit(0);
      u8g2->setCursor(5, 20);
      u8g2->print(line0);

      String line1 = String(Pms.pm2_5()) + " " + FwBoxIns.getValUnit(1);
      u8g2->setCursor(5, 45);
      u8g2->print(line1);

      String line2 = String(Pms.pm10_0()) + " " + FwBoxIns.getValUnit(2);
      u8g2->setCursor(5, 70);
      u8g2->print(line2);
    } while ( u8g2->nextPage() );
  } // END OF "if(u8g2 != 0)"
}


//
// Display Date, Time, Humidity and Temperature
// 顯示日期，時間，溼度與溫度。
//
void LcdDisplayType1()
{
  //
  // Print YEAR-MONTH-DAY
  //
  Lcd->setCursor(0, 0);
  //Lcd->print(year());
  //Lcd->print("-");   
  PrintLcdDigits(month());
  Lcd->print("-");   
  PrintLcdDigits(day());
  Lcd->print(" ");
  Lcd->print(WEEK_DAY_NAME[weekday() - 1]);
  Lcd->print("  ");
  Lcd->setCursor(16 - 5, 0); // Align right
  Lcd->print("PM2.5");

  //
  // Print HOUR:MIN:SEC WEEK
  //
  Lcd->setCursor(0, 1);
  PrintLcdDigits(hour());
  Lcd->print(":");
  PrintLcdDigits(minute());
  Lcd->print(":");    
  PrintLcdDigits(second());
  Lcd->print("     ");
  Lcd->setCursor(16 - 4, 1); // Align right
  if (Pms.pm2_5() <= 12) {
    Lcd->print("GOOD");
  }
  else if (Pms.pm2_5() > 12 && Pms.pm2_5() <= 55.4) {
    Lcd->print("FAIR");
  }
  else if (Pms.pm2_5() > 55.4) {
    Lcd->print("POOR");
  }
}

//
// Display Date and Time
// 顯示日期與時間。
//
void LcdDisplayType2()
{
  //
  // Print YEAR-MONTH-DAY
  //
  Lcd->setCursor(0, 0);
  Lcd->print("   ");
  Lcd->print(year());
  Lcd->print("-");   
  PrintLcdDigits(month());
  Lcd->print("-");   
  PrintLcdDigits(day());
  Lcd->print("   ");

  //
  // Print HOUR:MIN:SEC WEEK
  //
  Lcd->setCursor(0, 1);
  Lcd->print("  ");
  PrintLcdDigits(hour());
  Lcd->print(":");
  PrintLcdDigits(minute());
  Lcd->print(":");    
  PrintLcdDigits(second());
  Lcd->print(" ");
  Lcd->print(WEEK_DAY_NAME[weekday() - 1]);
  Lcd->print("  ");
}

//
// Display Humidity and Temperature
// 顯示溼度與溫度。
//
void LcdDisplayType3()
{
  Lcd->setCursor(0, 0);
  Lcd->print("PM1.0:");
  Lcd->print(Pms.pm1_0());
  Lcd->print(" ug/m3   ");
  Lcd->setCursor(0, 1);
  Lcd->print("PM2.5:");
  Lcd->print(Pms.pm2_5());
  Lcd->print(" ug/m3   ");
}

//
// Display Humidity and Temperature
// 顯示溼度與溫度。
//
void LcdDisplayType4()
{
  Lcd->setCursor(0, 0);
  Lcd->print("PM10:");
  Lcd->print(Pms.pm10_0());
  Lcd->print(" ug/m3    ");
  Lcd->setCursor(0, 1);
  Lcd->print("                ");
}

//
// Display the information
//
void LcdDisplayType5()
{
  //
  // Print the device ID and the firmware version.
  //
  Lcd->setCursor(0, 0);
  Lcd->print("ID:");
  Lcd->print(FwBoxIns.getSimpleChipId());
  Lcd->print("     v");
  Lcd->print(FIRMWARE_VERSION);
  Lcd->print("    ");

  //
  // Display the local IP address.
  //
  Lcd->setCursor(0, 1);
  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    int space_len = 16 - ip.length();
    Lcd->print(ip);

    //
    // Fill the char space at the end.
    //
    for (int i = 0; i < space_len; i++)
      Lcd->print(" ");
  }
  else {
    Lcd->print("                ");
  }
}

int getDisplayMode(int pageCount,int analogValue)
{
  int page_width = 1024 / pageCount;

  for (int i = 0; i < pageCount; i++) {
    if (i == 0) {
      if (analogValue < (page_width*(i+1))-ANALOG_VALUE_DEBOUNCING) { // The value - '5' is for debouncing.
        return i + 1;
      }
    }
    else if (i == (pageCount - 1)) {
      if (analogValue >= (page_width*i)+ANALOG_VALUE_DEBOUNCING) { // The value - '5' is for debouncing.
        return i + 1;
      }
    }
    else {
      if ((analogValue >= (page_width*i)+ANALOG_VALUE_DEBOUNCING) && (analogValue < (page_width*(i+1))-ANALOG_VALUE_DEBOUNCING)) { // The value - '5' is for debouncing.
        return i + 1;
      }
    }
  }

  return 1; // default page
}

void PrintLcdDigits(int digits)
{
  if (digits < 10)
    Lcd->print('0');
  Lcd->print(digits);
}
