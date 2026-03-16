
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <Wire.h>
#include <Adafruit_SGP30.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// LVGL
static lv_color_t draw_buf[SCREEN_WIDTH * 20];
static lv_display_t *display;
TFT_eSPI tft = TFT_eSPI();

// SGP30
Adafruit_SGP30 sgp;

// Panels
lv_obj_t *temp_panel;      // eCO2 gauge panel
lv_obj_t *pressure_panel;  // TVOC gauge panel

// Arcs
lv_obj_t *temp_arc;        // eCO2 arc
lv_obj_t *pressure_arc;    // TVOC arc

// Value labels
lv_obj_t *temp_value_label;     // eCO2 value
lv_obj_t *pressure_value_label; // TVOC value

// Charts
lv_obj_t *chart;
lv_obj_t *pressure_chart;
lv_chart_series_t *temp_series;
lv_chart_series_t *pressure_series;

//  Display flush ─
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

static uint32_t my_tick(void) { return millis(); }

//  Dashboard ─
void create_dashboard() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "SGP30");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x333333), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    //  eCO2 graph panel (left) ─
    lv_obj_t *eco2_chart_panel = lv_obj_create(scr);
    lv_obj_set_size(eco2_chart_panel, 159, 120);
    lv_obj_align(eco2_chart_panel, LV_ALIGN_TOP_LEFT, 0, 35);
    lv_obj_set_style_bg_color(eco2_chart_panel, lv_color_white(), 0);
    lv_obj_set_style_border_width(eco2_chart_panel, 1, 0);
    lv_obj_set_style_border_color(eco2_chart_panel, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(eco2_chart_panel, 6, 0);
    lv_obj_set_style_pad_all(eco2_chart_panel, 4, 0);
    lv_obj_set_scroll_dir(eco2_chart_panel, LV_DIR_NONE);

    // eCO2 label
    lv_obj_t *eco2_label = lv_label_create(eco2_chart_panel);
    lv_label_set_text(eco2_label, "eCO2");
    lv_obj_set_style_text_font(eco2_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(eco2_label, lv_color_hex(0x4CAF50), 0);
    lv_obj_align(eco2_label, LV_ALIGN_TOP_LEFT, 2, 0);

    // eCO2 chart
    chart = lv_chart_create(eco2_chart_panel);
    lv_obj_set_size(chart, 140, 90);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 20);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 395, 420);
    lv_chart_set_div_line_count(chart, 4, 5);
    lv_obj_set_style_bg_color(chart, lv_color_white(), 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0xEEEEEE), LV_PART_MAIN);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);

    temp_series = lv_chart_add_series(chart, lv_color_hex(0x4CAF50),
                                      LV_CHART_AXIS_PRIMARY_Y);
    for(int i = 0; i < 20; i++) {
        lv_chart_set_next_value(chart, temp_series, 400);
    }

    // eCO2 scale labels
    lv_obj_t *eco2_max = lv_label_create(eco2_chart_panel);
    lv_label_set_text(eco2_max, "420");
    lv_obj_set_style_text_font(eco2_max, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(eco2_max, lv_color_hex(0x999999), 0);
    lv_obj_align(eco2_max, LV_ALIGN_TOP_RIGHT, -2, 2);

    lv_obj_t *eco2_mid = lv_label_create(eco2_chart_panel);
    lv_label_set_text(eco2_mid, "407");
    lv_obj_set_style_text_font(eco2_mid, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(eco2_mid, lv_color_hex(0x999999), 0);
    lv_obj_align(eco2_mid, LV_ALIGN_RIGHT_MID, 2, 0);

    lv_obj_t *eco2_min = lv_label_create(eco2_chart_panel);
    lv_label_set_text(eco2_min, "395");
    lv_obj_set_style_text_font(eco2_min, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(eco2_min, lv_color_hex(0x999999), 0);
    lv_obj_align(eco2_min, LV_ALIGN_BOTTOM_RIGHT, -2, -8);

    //  TVOC graph panel (right) 
    lv_obj_t *tvoc_chart_panel = lv_obj_create(scr);
    lv_obj_set_size(tvoc_chart_panel, 159, 120);
    lv_obj_align(tvoc_chart_panel, LV_ALIGN_TOP_RIGHT, 0, 35);
    lv_obj_set_style_bg_color(tvoc_chart_panel, lv_color_white(), 0);
    lv_obj_set_style_border_width(tvoc_chart_panel, 1, 0);
    lv_obj_set_style_border_color(tvoc_chart_panel, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(tvoc_chart_panel, 6, 0);
    lv_obj_set_style_pad_all(tvoc_chart_panel, 4, 0);
    lv_obj_set_scroll_dir(tvoc_chart_panel, LV_DIR_NONE);

    // TVOC label
    lv_obj_t *tvoc_label = lv_label_create(tvoc_chart_panel);
    lv_label_set_text(tvoc_label, "TVOC");
    lv_obj_set_style_text_font(tvoc_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(tvoc_label, lv_color_hex(0x2196F3), 0);
    lv_obj_align(tvoc_label, LV_ALIGN_TOP_LEFT, 2, 0);

    // TVOC chart
    pressure_chart = lv_chart_create(tvoc_chart_panel);
    lv_obj_set_size(pressure_chart, 140, 90);
    lv_obj_align(pressure_chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_chart_set_type(pressure_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(pressure_chart, 20);
    lv_chart_set_range(pressure_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 30);
    lv_chart_set_div_line_count(pressure_chart, 4, 5);
    lv_obj_set_style_bg_color(pressure_chart, lv_color_white(), 0);
    lv_obj_set_style_border_width(pressure_chart, 0, 0);
    lv_obj_set_style_line_color(pressure_chart, lv_color_hex(0xEEEEEE), LV_PART_MAIN);
    lv_obj_set_style_size(pressure_chart, 0, 0, LV_PART_INDICATOR);

    pressure_series = lv_chart_add_series(pressure_chart, lv_color_hex(0x2196F3),
                                          LV_CHART_AXIS_PRIMARY_Y);
    for(int i = 0; i < 20; i++) {
        lv_chart_set_next_value(pressure_chart, pressure_series, 15);
    }

    // TVOC scale labels
    lv_obj_t *tvoc_max = lv_label_create(tvoc_chart_panel);
    lv_label_set_text(tvoc_max, "30");
    lv_obj_set_style_text_font(tvoc_max, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(tvoc_max, lv_color_hex(0x999999), 0);
    lv_obj_align(tvoc_max, LV_ALIGN_TOP_RIGHT, -2, 2);

    lv_obj_t *tvoc_mid = lv_label_create(tvoc_chart_panel);
    lv_label_set_text(tvoc_mid, "15");
    lv_obj_set_style_text_font(tvoc_mid, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(tvoc_mid, lv_color_hex(0x999999), 0);
    lv_obj_align(tvoc_mid, LV_ALIGN_RIGHT_MID, 2, 0);

    lv_obj_t *tvoc_min = lv_label_create(tvoc_chart_panel);
    lv_label_set_text(tvoc_min, "0");
    lv_obj_set_style_text_font(tvoc_min, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(tvoc_min, lv_color_hex(0x999999), 0);
    lv_obj_align(tvoc_min, LV_ALIGN_BOTTOM_RIGHT, -2, -8);

    //  eCO2 gauge panel (bottom left) 
    temp_panel = lv_obj_create(scr);
    lv_obj_set_size(temp_panel, 158, 100);
    lv_obj_align(temp_panel, LV_ALIGN_BOTTOM_LEFT, 1, -1);
    lv_obj_set_style_bg_color(temp_panel, lv_color_white(), 0);
    lv_obj_set_style_border_width(temp_panel, 1, 0);
    lv_obj_set_style_border_color(temp_panel, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(temp_panel, 6, 0);
    lv_obj_set_style_pad_all(temp_panel, 4, 0);

    lv_obj_t *eco2_title = lv_label_create(temp_panel);
    lv_label_set_text(eco2_title, "eCO2");
    lv_obj_set_style_text_font(eco2_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(eco2_title, lv_color_hex(0x333333), 0);
    lv_obj_align(eco2_title, LV_ALIGN_TOP_LEFT, 4, 2);

    // eCO2 arc
    temp_arc = lv_arc_create(temp_panel);
    lv_obj_set_size(temp_arc, 70, 70);
    lv_arc_set_rotation(temp_arc, 135);
    lv_arc_set_bg_angles(temp_arc, 0, 270);
    lv_arc_set_range(temp_arc, 395, 420);
    lv_arc_set_value(temp_arc, 400);
    lv_obj_remove_style(temp_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_color(temp_arc, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(temp_arc, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_set_style_arc_width(temp_arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(temp_arc, 8, LV_PART_MAIN);
    lv_obj_align(temp_arc, LV_ALIGN_CENTER, 10, 8);

    temp_value_label = lv_label_create(temp_panel);
    lv_label_set_text(temp_value_label, "400ppm");
    lv_obj_set_style_text_font(temp_value_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(temp_value_label, lv_color_hex(0x333333), 0);
    lv_obj_align_to(temp_value_label, temp_arc, LV_ALIGN_CENTER, -12, 0);

    // eCO2 scale labels
    lv_obj_t *eco2_g_min = lv_label_create(temp_panel);
    lv_label_set_text(eco2_g_min, "395");
    lv_obj_set_style_text_font(eco2_g_min, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(eco2_g_min, lv_color_hex(0x999999), 0);
    lv_obj_align_to(eco2_g_min, temp_arc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -8);

    lv_obj_t *eco2_g_max = lv_label_create(temp_panel);
    lv_label_set_text(eco2_g_max, "420");
    lv_obj_set_style_text_font(eco2_g_max, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(eco2_g_max, lv_color_hex(0x999999), 0);
    lv_obj_align_to(eco2_g_max, temp_arc, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, -8);

    //  TVOC gauge panel (bottom right) 
    pressure_panel = lv_obj_create(scr);
    lv_obj_set_size(pressure_panel, 158, 100);
    lv_obj_align(pressure_panel, LV_ALIGN_BOTTOM_RIGHT, -1, -1);
    lv_obj_set_style_bg_color(pressure_panel, lv_color_white(), 0);
    lv_obj_set_style_border_width(pressure_panel, 1, 0);
    lv_obj_set_style_border_color(pressure_panel, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(pressure_panel, 6, 0);
    lv_obj_set_style_pad_all(pressure_panel, 4, 0);

    lv_obj_t *tvoc_title = lv_label_create(pressure_panel);
    lv_label_set_text(tvoc_title, "TVOC");
    lv_obj_set_style_text_font(tvoc_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(tvoc_title, lv_color_hex(0x333333), 0);
    lv_obj_align(tvoc_title, LV_ALIGN_TOP_LEFT, 4, 2);

    // TVOC arc
    pressure_arc = lv_arc_create(pressure_panel);
    lv_obj_set_size(pressure_arc, 70, 70);
    lv_arc_set_rotation(pressure_arc, 135);
    lv_arc_set_bg_angles(pressure_arc, 0, 270);
    lv_arc_set_range(pressure_arc, 0, 30);
    lv_arc_set_value(pressure_arc, 15);
    lv_obj_remove_style(pressure_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_color(pressure_arc, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(pressure_arc, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_set_style_arc_width(pressure_arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(pressure_arc, 8, LV_PART_MAIN);
    lv_obj_align(pressure_arc, LV_ALIGN_CENTER, 10, 8);

    pressure_value_label = lv_label_create(pressure_panel);
    lv_label_set_text(pressure_value_label, "0ppb");
    lv_obj_set_style_text_font(pressure_value_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(pressure_value_label, lv_color_hex(0x333333), 0);
    lv_obj_align_to(pressure_value_label, pressure_arc, LV_ALIGN_CENTER, -12, 0);

    // TVOC scale labels
    lv_obj_t *tvoc_g_min = lv_label_create(pressure_panel);
    lv_label_set_text(tvoc_g_min, "0");
    lv_obj_set_style_text_font(tvoc_g_min, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(tvoc_g_min, lv_color_hex(0x999999), 0);
    lv_obj_align_to(tvoc_g_min, pressure_arc, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -8);

    lv_obj_t *tvoc_g_max = lv_label_create(pressure_panel);
    lv_label_set_text(tvoc_g_max, "30");
    lv_obj_set_style_text_font(tvoc_g_max, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(tvoc_g_max, lv_color_hex(0x999999), 0);
    lv_obj_align_to(tvoc_g_max, pressure_arc, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, -8);
}

//  Setup ─
void setup() {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    Wire.begin(27, 22);
    if (!sgp.begin()) {
        Serial.println("SGP30 not found!");
        while(1);
    }

    lv_init();
    lv_tick_set_cb(my_tick);

    display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(display, my_disp_flush);
    lv_display_set_buffers(display, draw_buf, NULL,
                           sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    create_dashboard();
}

//  Loop 
void loop() {
    lv_timer_handler();

    static uint32_t last_update = 0;
    if (millis() - last_update > 2000) {
        last_update = millis();

        if (sgp.IAQmeasure()) {
            float eco2 = sgp.eCO2;
            float tvoc = sgp.TVOC;

            // Update charts
            lv_chart_set_next_value(chart, temp_series, (int)eco2);
            lv_chart_set_next_value(pressure_chart, pressure_series, (int)tvoc);
            lv_chart_refresh(chart);
            lv_chart_refresh(pressure_chart);

            // Update arcs
            lv_arc_set_value(temp_arc, (int)eco2);
            lv_arc_set_value(pressure_arc, (int)tvoc);

            // Update value labels
            char buf[16];
            snprintf(buf, sizeof(buf), "%dppm", (int)eco2);
            lv_label_set_text(temp_value_label, buf);
            snprintf(buf, sizeof(buf), "%dppb", (int)tvoc);
            lv_label_set_text(pressure_value_label, buf);

            // eCO2 alert above 415 ppm
            if (eco2 > 415) {
                lv_obj_set_style_arc_color(temp_arc, lv_color_hex(0xE53935), LV_PART_INDICATOR);
            } else {
                lv_obj_set_style_arc_color(temp_arc, lv_color_hex(0x4CAF50), LV_PART_INDICATOR);
            }

            // TVOC alert above 25 ppb
            if (tvoc > 25) {
                lv_obj_set_style_arc_color(pressure_arc, lv_color_hex(0xE53935), LV_PART_INDICATOR);
            } else {
                lv_obj_set_style_arc_color(pressure_arc, lv_color_hex(0x2196F3), LV_PART_INDICATOR);
            }
        }
    }

    delay(5);
}