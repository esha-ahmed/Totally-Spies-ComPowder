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

// Colori
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREEN 0x07E0
#define PINK  0xFC18

// Inizializza Display
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0, true);

void setup() {
  Serial.begin(115200);
  
  // 1. Accendi retroilluminazione
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  // 2. Avvia Display
  if (!gfx->begin()) Serial.println("Errore display!");
  gfx->fillScreen(BLACK);
  
  // Testo al centro
  gfx->setTextColor(PINK);
  gfx->setTextSize(2);
  gfx->setCursor(30, 110);
  gfx->println("Tocca lo schermo!");

  // 3. Inizializza Touch (Senza librerie esterne)
  pinMode(TOUCH_INT, INPUT_PULLUP);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_RST, HIGH);
  delay(50);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  
  Serial.println("Pronto! Disegna sullo schermo.");
}

void loop() {
  // Se il chip touch sente il dito, abbassa il pin INT
  if (digitalRead(TOUCH_INT) == LOW) {
    
    // Leggiamo i registri grezzi del tocco (dal tuo PDF)
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(0x03); 
    Wire.endTransmission(false);
    Wire.requestFrom(TOUCH_ADDR, 4, true);
    
    if (Wire.available() == 4) {
      byte x_high = Wire.read();
      byte x_low  = Wire.read();
      byte y_high = Wire.read();
      byte y_low  = Wire.read();
      
      // Calcoliamo la X e la Y
      int x = ((x_high & 0x0F) << 8) | x_low;
      int y = ((y_high & 0x0F) << 8) | y_low;
      
      // Stampa sul Mac
      Serial.print("X: "); Serial.print(x);
      Serial.print(" | Y: "); Serial.println(y);

      // Disegna un pallino verde dove hai toccato!
      gfx->fillCircle(x, y, 4, GREEN);
    }
    delay(10);
  }
}