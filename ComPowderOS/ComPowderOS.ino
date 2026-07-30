/*******************************************************************************
  ComPowder OS — shell LVGL per ESP32S3-128SPIT (GC9A01 1.28" + CST816S)

  Basato sul tuo sketch funzionante: stessi pin, stesso bus SPI, stesso
  mapping registri touch. La differenza è che ora il rendering delle "app"
  passa da gfx->drawXXX diretto a widget LVGL, gestiti da uno screen-manager
  che cambia schermata con swipe (gesture I2C, come nel tuo codice) o
  pulsanti fisici.

  Librerie richieste (stesse versioni del pacchetto del produttore):
    Arduino_GFX_Library   1.4.7
    lvgl                  8.3.6   <-- IMPORTANTE: usa la 8.3.x, non la 9.x
    Adafruit_NeoPixel     1.12.5

  Impostazioni scheda Arduino IDE (dal manuale del produttore):
    Board: ESP32S3 Dev Module
    USB CDC On Boot: Enabled
    Flash Mode: QIO 80MHz
    Flash Size: 16MB (128Mb)
    Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)
    PSRAM: OPI PSRAM

  Copia lv_conf.h nella cartella del tuo sketchbook /libraries (accanto a
  dove estrai la libreria lvgl) oppure segui le istruzioni nel README.
*******************************************************************************/

#include <Arduino_GFX_Library.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <lvgl.h>
#include "apps.h"

// ================== PIN DISPLAY ==================
#define TFT_SCK    3
#define TFT_MOSI   10
#define TFT_CS     2
#define TFT_DC     18
#define TFT_RST    21
#define TFT_BLK    42

// ================== PIN TOUCH ==================
#define TOUCH_SDA  8
#define TOUCH_SCL  9
#define TOUCH_RST  0
#define TOUCH_INT  11
#define TOUCH_ADDR 0x15

// ================== PIN RTC ==================
#define RTC_ADDR   0x51   // PCF85063, stesso bus I2C del touch

// ================== PIN PULSANTI FISICI ==================
#define SW_UP      14
#define SW_PW      15
#define SW_DOWN    16

// ================== PIN LED ==================
#define LED_PIN    46
#define NUM_LEDS   2

// ================== COLORI (RGB565, come nel tuo sketch originale) ==================
#define BLACK   0x0000
#define WHITE   0xFFFF

// ================== HARDWARE ==================
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, -1, HSPI);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0, true);
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ================== LVGL BUFFER ==================
static const uint32_t SCREEN_W = 240;
static const uint32_t SCREEN_H = 240;
static lv_disp_draw_buf_t draw_buf;
// 20 righe di buffer: buon compromesso fluidità/RAM su questa board
static lv_color_t *buf1;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// ================== STATO TOUCH ==================
volatile bool touchIntFlag = false;
static lv_coord_t lastTouchX = 0, lastTouchY = 0;
static bool touchPressed = false;

// ================== APP MANAGER ==================
// Ordine di navigazione (swipe sinistra = avanti, destra = indietro)
enum AppId { APP_RADAR = 0, APP_MUSIC, APP_NETSCAN, APP_CLOCK, APP_COUNT };
static lv_obj_t *screens[APP_COUNT];
static int currentApp = APP_RADAR;

static void loadApp(int idx, lv_scr_load_anim_t anim) {
  currentApp = (idx + APP_COUNT) % APP_COUNT;
  lv_scr_load_anim(screens[currentApp], anim, 180, 0, false);
}
static void nextApp() { loadApp(currentApp + 1, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void prevApp() { loadApp(currentApp - 1, LV_SCR_LOAD_ANIM_MOVE_RIGHT); }

// ================== DISPLAY FLUSH CALLBACK ==================
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  lv_disp_flush_ready(disp);
}

// ================== TOUCH: lettura combinata gesture + coordinate ==================
// Stesso registro/burst-read del tuo sketch originale (parte da 0x01, 6 byte):
// [0]=gesture [1]=points [2]=x_high [3]=x_low [4]=y_high [5]=y_low
static unsigned long lastGestureNav = 0;
const long gestureNavCooldown = 450; // ms — il CST816S manda più INT per un solo swipe, senza questo la app "salta" più schermate a ogni gesto

static void readTouchController() {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x01);
  Wire.endTransmission(false);
  Wire.requestFrom(TOUCH_ADDR, 6, true);
  if (Wire.available() < 6) { touchPressed = false; return; }

  byte gesture = Wire.read();
  byte points  = Wire.read();
  byte x_high  = Wire.read();
  byte x_low   = Wire.read();
  byte y_high  = Wire.read();
  byte y_low   = Wire.read();

  int x = ((x_high & 0x0F) << 8) | x_low;
  int y = ((y_high & 0x0F) << 8) | y_low;

  unsigned long now = millis();
  if ((gesture == 0x03 || gesture == 0x04) && (now - lastGestureNav < gestureNavCooldown)) {
    return; // stesso swipe, evento ripetuto: ignora
  }

  if (gesture == 0x03) {          // swipe sinistra -> prossima app
    nextApp();
    lastGestureNav = now;
  } else if (gesture == 0x04) {   // swipe destra -> app precedente
    prevApp();
    lastGestureNav = now;
  } else if (points > 0) {        // tap/pressione -> inoltra a LVGL
    lastTouchX = constrain(x, 0, (int)SCREEN_W - 1);
    lastTouchY = constrain(y, 0, (int)SCREEN_H - 1);
    touchPressed = true;
    return;
  }
  touchPressed = false;
}

void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  if (touchIntFlag) {
    touchIntFlag = false;
    readTouchController();
  }
  data->point.x = lastTouchX;
  data->point.y = lastTouchY;
  data->state = touchPressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
  // Il tap è un evento singolo: dopo averlo consumato, rilascia
  if (touchPressed) touchPressed = false;
}

void IRAM_ATTR touchISR() { touchIntFlag = true; }

// ================== RTC PCF85063 ==================
// Mappatura registri identica a quella usata dal produttore nel firmware
// di test: 0x04 sec, 0x05 min, 0x06 ore, 0x07 giorno, 0x08 giorno settimana,
// 0x09 mese, 0x0A anno — tutti in BCD tranne il giorno della settimana.

// ---- Imposta a 1 SOLO per il caricamento in cui vuoi settare l'ora,
//      poi rimettilo a 0 e ricarica lo sketch (l'RTC è a batteria tampone
//      e mantiene l'ora anche senza alimentazione principale). ----
#define SET_RTC_ON_BOOT 0
#if SET_RTC_ON_BOOT
  // Modifica questi valori con data/ora attuali prima di caricare
  static const int SET_YEAR = 26, SET_MONTH = 7, SET_DAY = 11;
  static const int SET_WEEKDAY = 5; // 0=Lun ... 6=Dom
  static const int SET_HOUR = 18, SET_MINUTE = 30, SET_SECOND = 0;
#endif

static uint8_t toBCD(int v) { return ((v / 10) << 4) | (v % 10); }
static int fromBCD(uint8_t v, uint8_t mask) { v &= mask; return ((v >> 4) * 10) + (v & 0x0F); }

static void rtcWriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission(true);
}

static uint8_t rtcReadReg(uint8_t reg) {
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(RTC_ADDR, 1, true);
  return Wire.available() ? Wire.read() : 0;
}

void rtcInit() {
#if SET_RTC_ON_BOOT
  rtcWriteReg(0x04, toBCD(SET_SECOND));
  rtcWriteReg(0x05, toBCD(SET_MINUTE));
  rtcWriteReg(0x06, toBCD(SET_HOUR) & 0x3F);
  rtcWriteReg(0x07, toBCD(SET_DAY) & 0x3F);
  rtcWriteReg(0x08, SET_WEEKDAY);
  rtcWriteReg(0x09, toBCD(SET_MONTH) & 0x1F);
  rtcWriteReg(0x0A, toBCD(SET_YEAR));
  Serial.println("RTC: ora impostata da SET_RTC_ON_BOOT — ricordati di rimetterlo a 0!");
#endif

  // Difensivo, eseguito SEMPRE (anche a SET_RTC_ON_BOOT=0):
  // 1) Control_1 (0x00) = 0x00 -> garantisce STOP=0 (oscillatore in marcia) e modalità 24h.
  //    Se lo STOP bit fosse rimasto a 1 da un boot precedente, l'orologio resta congelato
  //    su un orario fisso esattamente come descritto: questo lo sblocca.
  rtcWriteReg(0x00, 0x00);

  // 2) Pulisce il flag OS (bit7 del registro secondi) senza toccare il valore dei secondi,
  //    così un eventuale "integrity not guaranteed" dal power-on non blocca nulla.
  uint8_t sec = rtcReadReg(0x04);
  rtcWriteReg(0x04, sec & 0x7F);
}

void rtcReadTime(int &hour, int &minute, int &second, int &day, int &month, int &year) {
  second = fromBCD(rtcReadReg(0x04), 0x70);
  minute = fromBCD(rtcReadReg(0x05), 0x70);
  hour   = fromBCD(rtcReadReg(0x06), 0x30);
  day    = fromBCD(rtcReadReg(0x07), 0x30);
  month  = fromBCD(rtcReadReg(0x09), 0x10);
  year   = fromBCD(rtcReadReg(0x0A), 0xF0);
}

// ================== PULSANTI FISICI ==================
unsigned long lastButtonPress = 0;
const long debounceDelay = 220;

void handlePhysicalButtons(unsigned long now) {
  if (now - lastButtonPress <= debounceDelay) return;
  if (digitalRead(SW_UP) == LOW) {
    prevApp();
    lastButtonPress = now;
  } else if (digitalRead(SW_DOWN) == LOW) {
    nextApp();
    lastButtonPress = now;
  } else if (digitalRead(SW_PW) == LOW) {
    Serial.println("Pulsante Power/Select premuto!");
    lastButtonPress = now;
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  // Pulsanti
  pinMode(SW_UP, INPUT_PULLUP);
  pinMode(SW_PW, INPUT_PULLUP);
  pinMode(SW_DOWN, INPUT_PULLUP);

  // LED stato
  pixels.begin();
  pixels.setBrightness(80);
  pixels.setPixelColor(0, pixels.Color(255, 20, 147));
  if (NUM_LEDS > 1) pixels.setPixelColor(1, pixels.Color(255, 20, 147));
  pixels.show();

  // Touch I2C
  pinMode(TOUCH_INT, INPUT_PULLUP);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_RST, HIGH);
  delay(50);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Wire.setClock(400000);
  attachInterrupt(digitalPinToInterrupt(TOUCH_INT), touchISR, FALLING);
  rtcInit();

  // Display
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
  if (!gfx->begin()) Serial.println("Errore display!");
  gfx->fillScreen(BLACK);

  // LVGL init
  lv_init();

  buf1 = (lv_color_t *)heap_caps_malloc(SCREEN_W * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
  if (!buf1) {
    Serial.println("ERRORE: allocazione buffer LVGL fallita! Controlla che PSRAM sia OPI PSRAM in Strumenti.");
    while (1) delay(1000);
  }
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_W * 20);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_W;
  disp_drv.ver_res = SCREEN_H;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);

  // Costruzione delle app come schermate LVGL indipendenti
  screens[APP_RADAR]   = radarApp_create();
  screens[APP_MUSIC]    = musicApp_create();
  screens[APP_NETSCAN]  = netscanApp_create();
  screens[APP_CLOCK]    = clockApp_create();

  lv_scr_load(screens[APP_RADAR]);

  Serial.println("ComPowder OS Boot completato!");
}

// ================== LOOP ==================
unsigned long lastClockRead = 0;

void loop() {
  unsigned long now = millis();
  handlePhysicalButtons(now);

  // Aggiorna le animazioni specifiche di ogni app (radar sweep, spectrum, ecc.)
  radarApp_update(now);
  musicApp_update(now);
  netscanApp_update(now);

  if (now - lastClockRead >= 1000) {
    lastClockRead = now;
    int h, m, s, d, mo, y;
    rtcReadTime(h, m, s, d, mo, y);
    clockApp_setTime(h, m, s, d, mo, y);
    Serial.printf("\r\nRTC: %02d:%02d:%02d %02d/%02d/%02d (raw sec=0x%02X)",
                  h, m, s, d, mo, y, rtcReadReg(0x04));
  }

  lv_timer_handler();
  delay(5);
}
