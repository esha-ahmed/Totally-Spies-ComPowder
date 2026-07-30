#include <Arduino.h>
#include "apps.h"
#include <math.h>

/* =========================================================================
   APP 1 — RADAR
   Griglia + cerchi statici (disegnati una volta) + linea "sweep" rotante
   ridisegnata ogni frame con lv_line_set_points, + bersaglio lampeggiante.
   ========================================================================= */
static lv_obj_t *screenRadar;
static lv_obj_t *sweepLine;
static lv_obj_t *targetDot;
static lv_point_t sweepPoints[2];
static int radarAngle = 0;
static unsigned long lastRadarStep = 0;
static const int RADAR_CX = 120, RADAR_CY = 120, RADAR_R = 95;

lv_obj_t *radarApp_create() {
  screenRadar = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screenRadar, lv_color_black(), 0);
  lv_obj_set_style_border_width(screenRadar, 0, 0);

  lv_obj_t *title = lv_label_create(screenRadar);
  lv_label_set_text(title, "COMPOWDER");
  lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_PINK), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 22);

  // Cerchi concentrici e crociera, disegnati come oggetti statici (non
  // vanno ridisegnati ogni frame, a differenza dello sweep)
  lv_obj_t *ring1 = lv_obj_create(screenRadar);
  lv_obj_remove_style_all(ring1);
  lv_obj_set_size(ring1, RADAR_R * 2 * 50 / 95, RADAR_R * 2 * 50 / 95);
  lv_obj_set_style_radius(ring1, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_color(ring1, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_border_width(ring1, 1, 0);
  lv_obj_set_style_bg_opa(ring1, LV_OPA_TRANSP, 0);
  lv_obj_align(ring1, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t *ring2 = lv_obj_create(screenRadar);
  lv_obj_remove_style_all(ring2);
  lv_obj_set_size(ring2, RADAR_R * 2, RADAR_R * 2);
  lv_obj_set_style_radius(ring2, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_color(ring2, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_border_width(ring2, 1, 0);
  lv_obj_set_style_bg_opa(ring2, LV_OPA_TRANSP, 0);
  lv_obj_align(ring2, LV_ALIGN_CENTER, 0, 0);

  static lv_point_t hLine[] = {{RADAR_CX - RADAR_R, RADAR_CY}, {RADAR_CX + RADAR_R, RADAR_CY}};
  static lv_point_t vLine[] = {{RADAR_CX, RADAR_CY - RADAR_R}, {RADAR_CX, RADAR_CY + RADAR_R}};
  static lv_style_t gridStyle;
  lv_style_init(&gridStyle);
  lv_style_set_line_color(&gridStyle, lv_palette_main(LV_PALETTE_GREEN));
  lv_style_set_line_width(&gridStyle, 1);

  lv_obj_t *lh = lv_line_create(screenRadar);
  lv_line_set_points(lh, hLine, 2);
  lv_obj_add_style(lh, &gridStyle, 0);

  lv_obj_t *lv_v = lv_line_create(screenRadar);
  lv_line_set_points(lv_v, vLine, 2);
  lv_obj_add_style(lv_v, &gridStyle, 0);

  // Linea rotante (lo sweep radar vero e proprio)
  static lv_style_t sweepStyle;
  lv_style_init(&sweepStyle);
  lv_style_set_line_color(&sweepStyle, lv_palette_main(LV_PALETTE_GREEN));
  lv_style_set_line_width(&sweepStyle, 2);
  sweepLine = lv_line_create(screenRadar);
  lv_obj_add_style(sweepLine, &sweepStyle, 0);
  sweepPoints[0].x = RADAR_CX; sweepPoints[0].y = RADAR_CY;
  sweepPoints[1].x = RADAR_CX + RADAR_R; sweepPoints[1].y = RADAR_CY;
  lv_line_set_points(sweepLine, sweepPoints, 2);

  // Bersaglio lampeggiante
  targetDot = lv_obj_create(screenRadar);
  lv_obj_remove_style_all(targetDot);
  lv_obj_set_size(targetDot, 10, 10);
  lv_obj_set_style_radius(targetDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(targetDot, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_set_style_bg_opa(targetDot, LV_OPA_COVER, 0);
  lv_obj_set_pos(targetDot, RADAR_CX + 20 - 5, RADAR_CY - 20 - 5);

  return screenRadar;
}

void radarApp_update(unsigned long now) {
  if (lv_scr_act() != screenRadar) return;
  if (now - lastRadarStep < 20) return;
  lastRadarStep = now;

  radarAngle = (radarAngle + 5) % 360;
  float rad = radarAngle * PI / 180.0f;
  sweepPoints[1].x = RADAR_CX + (int)(RADAR_R * cosf(rad));
  sweepPoints[1].y = RADAR_CY + (int)(RADAR_R * sinf(rad));
  lv_line_set_points(sweepLine, sweepPoints, 2);

  lv_obj_set_style_bg_opa(targetDot, (now % 1000 < 500) ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
}

/* =========================================================================
   APP 2 — MUSIC PLAYER (stile Spotify)
   Album art circolare + controlli + spectrum analyzer animato con lv_bar.
   ========================================================================= */
static lv_obj_t *screenMusic;
static lv_obj_t *playBtnLabel;
static lv_obj_t *bars[10];
static bool isPlaying = false;
static unsigned long lastSpectrumStep = 0;

static void playPauseCb(lv_event_t *e) {
  isPlaying = !isPlaying;
  lv_label_set_text(playBtnLabel, isPlaying ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}
static void prevTrackCb(lv_event_t *e) { Serial.println("SPOTIFY: Prev Track"); }
static void nextTrackCb(lv_event_t *e) { Serial.println("SPOTIFY: Next Track"); }

lv_obj_t *musicApp_create() {
  screenMusic = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screenMusic, lv_color_black(), 0);
  lv_obj_set_style_border_width(screenMusic, 0, 0);

  lv_obj_t *title = lv_label_create(screenMusic);
  lv_label_set_text(title, "SPOTIFY");
  lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  // Copertina album (placeholder circolare)
  lv_obj_t *cover = lv_obj_create(screenMusic);
  lv_obj_remove_style_all(cover);
  lv_obj_set_size(cover, 70, 70);
  lv_obj_set_style_radius(cover, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(cover, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
  lv_obj_set_style_bg_opa(cover, LV_OPA_COVER, 0);
  lv_obj_align(cover, LV_ALIGN_CENTER, 0, -35);

  // Spectrum analyzer: 10 barre verticali attorno alla cover
  for (int i = 0; i < 10; i++) {
    bars[i] = lv_bar_create(screenMusic);
    lv_obj_remove_style_all(bars[i]);
    lv_bar_set_range(bars[i], 0, 100);
    lv_bar_set_value(bars[i], 10, LV_ANIM_OFF);
    lv_obj_set_size(bars[i], 6, 40);
    lv_obj_set_style_bg_color(bars[i], lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bars[i], LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bars[i], lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bars[i], LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bars[i], 2, LV_PART_MAIN);
    lv_obj_align(bars[i], LV_ALIGN_CENTER, -45 + i * 10, -35);
  }

  // Controlli: prev / play-pause / next
  lv_obj_t *prevBtn = lv_btn_create(screenMusic);
  lv_obj_set_size(prevBtn, 44, 44);
  lv_obj_set_style_radius(prevBtn, LV_RADIUS_CIRCLE, 0);
  lv_obj_align(prevBtn, LV_ALIGN_CENTER, -55, 45);
  lv_obj_add_event_cb(prevBtn, prevTrackCb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *prevLbl = lv_label_create(prevBtn);
  lv_label_set_text(prevLbl, LV_SYMBOL_PREV);
  lv_obj_center(prevLbl);

  lv_obj_t *playBtn = lv_btn_create(screenMusic);
  lv_obj_set_size(playBtn, 54, 54);
  lv_obj_set_style_radius(playBtn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(playBtn, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_align(playBtn, LV_ALIGN_CENTER, 0, 45);
  lv_obj_add_event_cb(playBtn, playPauseCb, LV_EVENT_CLICKED, NULL);
  playBtnLabel = lv_label_create(playBtn);
  lv_label_set_text(playBtnLabel, LV_SYMBOL_PLAY);
  lv_obj_center(playBtnLabel);

  lv_obj_t *nextBtn = lv_btn_create(screenMusic);
  lv_obj_set_size(nextBtn, 44, 44);
  lv_obj_set_style_radius(nextBtn, LV_RADIUS_CIRCLE, 0);
  lv_obj_align(nextBtn, LV_ALIGN_CENTER, 55, 45);
  lv_obj_add_event_cb(nextBtn, nextTrackCb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *nextLbl = lv_label_create(nextBtn);
  lv_label_set_text(nextLbl, LV_SYMBOL_NEXT);
  lv_obj_center(nextLbl);

  return screenMusic;
}

void musicApp_update(unsigned long now) {
  if (lv_scr_act() != screenMusic) return;
  if (!isPlaying) return;
  if (now - lastSpectrumStep < 80) return;
  lastSpectrumStep = now;

  for (int i = 0; i < 10; i++) {
    int v = 10 + random(0, 90);
    lv_bar_set_value(bars[i], v, LV_ANIM_ON);
  }
}

/* =========================================================================
   APP 3 — NET SCANNER
   Due "card" di stato (WiFi / BLE) con LED che pulsano durante la scansione.
   ========================================================================= */
static lv_obj_t *screenNetscan;
static lv_obj_t *ledWifi;
static lv_obj_t *ledBle;
static unsigned long lastScanStep = 0;
static bool scanBlinkOn = false;

lv_obj_t *netscanApp_create() {
  screenNetscan = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screenNetscan, lv_color_black(), 0);
  lv_obj_set_style_border_width(screenNetscan, 0, 0);

  lv_obj_t *title = lv_label_create(screenNetscan);
  lv_label_set_text(title, "NET SCANNER");
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 26);

  lv_obj_t *panel = lv_obj_create(screenNetscan);
  lv_obj_set_size(panel, 170, 70);
  lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(panel, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 8, 0);
  lv_obj_align(panel, LV_ALIGN_CENTER, 0, 5);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  ledWifi = lv_led_create(panel);
  lv_obj_set_size(ledWifi, 12, 12);
  lv_led_set_color(ledWifi, lv_palette_main(LV_PALETTE_GREEN));
  lv_obj_align(ledWifi, LV_ALIGN_TOP_LEFT, 12, 14);
  lv_led_on(ledWifi);

  lv_obj_t *wifiLbl = lv_label_create(panel);
  lv_label_set_text(wifiLbl, "WiFi: Ready");
  lv_obj_set_style_text_color(wifiLbl, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_align(wifiLbl, LV_ALIGN_TOP_LEFT, 32, 12);

  ledBle = lv_led_create(panel);
  lv_obj_set_size(ledBle, 12, 12);
  lv_led_set_color(ledBle, lv_palette_main(LV_PALETTE_GREEN));
  lv_obj_align(ledBle, LV_ALIGN_TOP_LEFT, 12, 42);
  lv_led_on(ledBle);

  lv_obj_t *bleLbl = lv_label_create(panel);
  lv_label_set_text(bleLbl, "BLE:  Ready");
  lv_obj_set_style_text_color(bleLbl, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_align(bleLbl, LV_ALIGN_TOP_LEFT, 32, 40);

  return screenNetscan;
}

void netscanApp_update(unsigned long now) {
  if (lv_scr_act() != screenNetscan) return;
  if (now - lastScanStep < 600) return;
  lastScanStep = now;
  scanBlinkOn = !scanBlinkOn;
  if (scanBlinkOn) lv_led_set_brightness(ledWifi, 120);
  else lv_led_set_brightness(ledWifi, 255);
}

/* =========================================================================
   APP 4 — OROLOGIO
   Quadrante analogico con lv_meter: scala unica 0-60 (i 12 numeri
   dell'orologio coincidono con i tick ogni 5 unità), tre lancette.
   La lancetta ore usa lo stesso range mappando (ore%12)*5 + minuti/12,
   il classico trucco per farla muovere in modo continuo sulla stessa scala.
   ========================================================================= */
static lv_obj_t *screenClock;
static lv_obj_t *meter;
static lv_meter_indicator_t *needleHour;
static lv_meter_indicator_t *needleMin;
static lv_meter_indicator_t *needleSec;
static lv_obj_t *dateLabel;

static const char *WEEKDAY_NAMES[7] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};

lv_obj_t *clockApp_create() {
  screenClock = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screenClock, lv_color_black(), 0);
  lv_obj_set_style_border_width(screenClock, 0, 0);

  meter = lv_meter_create(screenClock);
  lv_obj_set_size(meter, 210, 210);
  lv_obj_center(meter);
  lv_obj_set_style_bg_opa(meter, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(meter, 0, LV_PART_MAIN);

  lv_meter_scale_t *scale = lv_meter_add_scale(meter);
  // 60 tick minori (minuti/secondi), major ogni 5 (= le 12 posizioni orarie)
  lv_meter_set_scale_ticks(meter, scale, 60, 1, 8, lv_palette_lighten(LV_PALETTE_GREY, 1));
  lv_meter_set_scale_major_ticks(meter, scale, 5, 3, 14, lv_color_white(), 12);
  lv_meter_set_scale_range(meter, scale, 0, 60, 360, 270); // 270° = i "12" in alto

  // Lancetta ore: corta e spessa
  needleHour = lv_meter_add_needle_line(meter, scale, 7, lv_color_white(), -35);
  // Lancetta minuti: lunga, media
  needleMin  = lv_meter_add_needle_line(meter, scale, 5, lv_color_white(), -20);
  // Lancetta secondi: sottile, accento colorato
  needleSec  = lv_meter_add_needle_line(meter, scale, 2, lv_palette_main(LV_PALETTE_PINK), -10);

  dateLabel = lv_label_create(screenClock);
  lv_label_set_text(dateLabel, "--/--");
  lv_obj_set_style_text_color(dateLabel, lv_color_white(), 0);
  lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, 55);

  return screenClock;
}

void clockApp_setTime(int hour, int minute, int second, int day, int month, int year) {
  if (lv_scr_act() != screenClock) return;

  int hourPos = (hour % 12) * 5 + minute / 12;
  lv_meter_set_indicator_value(meter, needleHour, hourPos);
  lv_meter_set_indicator_value(meter, needleMin, minute);
  lv_meter_set_indicator_value(meter, needleSec, second);

  char buf[24];
  snprintf(buf, sizeof(buf), "%02d/%02d/%02d", day, month, year);
  lv_label_set_text(dateLabel, buf);
}
