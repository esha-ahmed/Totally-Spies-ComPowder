#include <Arduino_GFX_Library.h>
#include <Wire.h>

// --- PIN DISPLAY ---
#define TFT_SCK    3
#define TFT_MOSI   10
#define TFT_CS     2
#define TFT_DC     18
#define TFT_RST    21
#define TFT_BLK    42

// --- PIN TOUCH ---
#define TOUCH_SDA  8
#define TOUCH_SCL  9
#define TOUCH_RST  0
#define TOUCH_INT  11
#define TOUCH_ADDR 0x15

// Colors
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define PINK  0xFC18

// Initialize Display
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0, true);

void setup() {
  Serial.begin(115200);
  
  // 1. Turn on the backlight
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  // 2. Start Display
  if (!gfx->begin()) Serial.println("Display error!");
  gfx->fillScreen(BLACK);
  
  // Text in the center
  gfx->setTextColor(PINK);
  gfx->setTextSize(2);
  gfx->setCursor(30, 110);
  gfx->println("Touch the screen!");

  // 3. Initialize Touch (Without external libraries)
  pinMode(TOUCH_INT, INPUT_PULLUP);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_RST, HIGH);
  delay(50);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  
  Serial.println("Ready! Draw on the screen.");
}

void loop() {
  // If the touch chip feels the finger, lower the INT pin
  if (digitalRead(TOUCH_INT) == LOW) {
    
    // We read the raw touch logs (from the seller's PDF)
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0x03); 
    Wire.endTransmission(false);
    Wire.requestFrom(TOUCH_ADDR, 4, true);
    
    if (Wire.available() == 4) {
      byte x_high = Wire.read();
      byte x_low  = Wire.read();
      byte y_high = Wire.read();
      byte y_low  = Wire.read();
      
      // Let's calculate the X and the Y
      int x = ((x_high & 0x0F) << 8) | x_low;
      int y = ((y_high & 0x0F) << 8) | y_low;
      
      // Print on Mac
      Serial.print("X: "); Serial.print(x);
      Serial.print(" | Y: "); Serial.println(y);

      // Draw a green dot where you touched!
      gfx->fillCircle(x, y, 4, GREEN);
    }
    delay(10);
  }
}
