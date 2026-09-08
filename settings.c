#include "globals.h"

lv_obj_t *setting_win = NULL;
lv_obj_t *bright_val_label = NULL;
lv_obj_t *buzzer_btn = NULL;
lv_obj_t *led_btns[4] = {NULL, NULL, NULL, NULL};
int buzzer_on = 0;
int led_on[4] = {0, 0, 0, 0};

#define HW_BUZZER_GPIO  111
#define HW_LED_GPIOS    {120, 121, 123, 124}
static const int hw_led_gpios[4] = HW_LED_GPIOS;
#define FB_DEVICE       "/dev/fb0"

 int hw_sysfs_write(const char *path, const char *s) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) { perror(path); return -1; }
    int ok = (write(fd, s, strlen(s)) > 0) ? 0 : -1;
    close(fd); return ok;
}

 int hw_read_int(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1; int v = -1; fscanf(f, "%d", &v); fclose(f); return v;
}

 void hw_backlight_set(int val) {
    char buf[16], p[128];
    int max = hw_read_int("/sys/class/backlight/backlight/max_brightness");
    if (max <= 0) max = 255;
    if (val < 0) val = 0; if (val > max) val = max;
    snprintf(buf, sizeof(buf), "%d", val);
    snprintf(p, sizeof(p), "/sys/class/backlight/backlight/brightness"); hw_sysfs_write(p, buf);
    snprintf(p, sizeof(p), "/sys/class/backlight/backlight1/brightness"); hw_sysfs_write(p, buf);
}

 int hw_backlight_get(void) {
    int v = hw_read_int("/sys/class/backlight/backlight/brightness");
    return v < 0 ? 0 : v;
}

 void hw_gpio_set(int gpio, int val) {
    char p[128], b[16];
    snprintf(p, sizeof(p), "/sys/class/gpio/gpio%d", gpio);
    if (access(p, F_OK) != 0) {
        snprintf(b, sizeof(b), "%d", gpio); hw_sysfs_write("/sys/class/gpio/export", b); usleep(200000);
    }
    snprintf(p, sizeof(p), "/sys/class/gpio/gpio%d/direction", gpio); hw_sysfs_write(p, "out");
    snprintf(p, sizeof(p), "/sys/class/gpio/gpio%d/value", gpio); hw_sysfs_write(p, val ? "1" : "0");
}

 void hw_screen_set(int on) {
    int fd = open(FB_DEVICE, O_RDWR);
    if (fd < 0) { perror(FB_DEVICE); return; }
    ioctl(fd, on ? FB_BLANK_UNBLANK : FB_BLANK_POWERDOWN);
    close(fd);
}

 void setting_back_click(lv_event_t *e) {
    if (setting_win) lv_obj_add_flag(setting_win, LV_OBJ_FLAG_HIDDEN);
    if (aw) lv_obj_remove_flag(aw, LV_OBJ_FLAG_HIDDEN);
}

 void bright_up_cb(lv_event_t *e) {
    hw_backlight_set(hw_backlight_get() + 20);
    if (bright_val_label) lv_label_set_text_fmt(bright_val_label, "%d/255", hw_backlight_get());
}

 void bright_down_cb(lv_event_t *e) {
    hw_backlight_set(hw_backlight_get() - 20);
    if (bright_val_label) lv_label_set_text_fmt(bright_val_label, "%d/255", hw_backlight_get());
}

 void buzzer_click_cb(lv_event_t *e) {
    buzzer_on = !buzzer_on;
    hw_gpio_set(HW_BUZZER_GPIO, buzzer_on);
    if (buzzer_btn) { lv_obj_t *lb = lv_obj_get_child(buzzer_btn, 0); if (lb) lv_label_set_text(lb, buzzer_on ? "ON" : "OFF"); }
    printf("Buzzer %s (GPIO%d)\n", buzzer_on ? "ON" : "OFF", HW_BUZZER_GPIO);
}

 void led_click_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(btn);
    if (idx < 0 || idx > 3) return;
    led_on[idx] = !led_on[idx];
    hw_gpio_set(hw_led_gpios[idx], led_on[idx]);
    if (led_btns[idx]) { lv_obj_t *lb = lv_obj_get_child(led_btns[idx], 0); if (lb) lv_label_set_text(lb, led_on[idx] ? "ON" : "OFF"); }
    printf("LED%d %s (GPIO%d)\n", idx + 1, led_on[idx] ? "ON" : "OFF", hw_led_gpios[idx]);
}

void create_setting_win(void) {
    if (setting_win) return;
    setting_win = lv_obj_create(lv_screen_active()); if (!setting_win) return;
    lv_obj_set_size(setting_win, 1024, 600); lv_obj_set_pos(setting_win, 0, 0);
    lv_obj_set_style_radius(setting_win, 0, 0); lv_obj_set_style_border_width(setting_win, 0, 0);
    lv_obj_set_style_bg_color(setting_win, lv_color_hex(0xEEEEEE), 0);
    lv_obj_add_flag(setting_win, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *title = lv_label_create(setting_win);
    lv_label_set_text(title, "SETTINGS"); lv_obj_set_style_text_font(title, &lv_font_montserrat_30, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    /* 亮度 */
    lv_obj_t *b_lab = lv_label_create(setting_win); lv_label_set_text(b_lab, "Brightness");
    lv_obj_set_style_text_font(b_lab, &lv_font_montserrat_20, 0); lv_obj_align(b_lab, LV_ALIGN_TOP_LEFT, 60, 90);
    lv_obj_t *b_minus = lv_button_create(setting_win); lv_obj_set_size(b_minus, 100, 70);
    lv_obj_align(b_minus, LV_ALIGN_TOP_LEFT, 60, 140);
    lv_obj_t *mi_lab = lv_label_create(b_minus); lv_label_set_text(mi_lab, "-"); lv_obj_center(mi_lab);
    lv_obj_add_event_cb(b_minus, bright_down_cb, LV_EVENT_PRESSED, NULL);
    bright_val_label = lv_label_create(setting_win);
    lv_label_set_text_fmt(bright_val_label, "%d/255", hw_backlight_get());
    lv_obj_set_style_text_font(bright_val_label, &lv_font_montserrat_30, 0); lv_obj_align(bright_val_label, LV_ALIGN_TOP_LEFT, 220, 155);
    lv_obj_t *b_plus = lv_button_create(setting_win); lv_obj_set_size(b_plus, 100, 70);
    lv_obj_align(b_plus, LV_ALIGN_TOP_LEFT, 400, 140);
    lv_obj_t *pl_lab = lv_label_create(b_plus); lv_label_set_text(pl_lab, "+"); lv_obj_center(pl_lab);
    lv_obj_add_event_cb(b_plus, bright_up_cb, LV_EVENT_PRESSED, NULL);
    /* 蜂鸣器 */
    lv_obj_t *z_lab = lv_label_create(setting_win); lv_label_set_text(z_lab, "Buzzer");
    lv_obj_set_style_text_font(z_lab, &lv_font_montserrat_20, 0); lv_obj_align(z_lab, LV_ALIGN_TOP_LEFT, 60, 250);
    buzzer_btn = lv_button_create(setting_win); lv_obj_set_size(buzzer_btn, 200, 80);
    lv_obj_align(buzzer_btn, LV_ALIGN_TOP_LEFT, 60, 300);
    lv_obj_t *bz_lab = lv_label_create(buzzer_btn); lv_label_set_text(bz_lab, "OFF"); lv_obj_center(bz_lab);
    lv_obj_add_event_cb(buzzer_btn, buzzer_click_cb, LV_EVENT_PRESSED, NULL);
    /* LED */
    lv_obj_t *l_lab = lv_label_create(setting_win); lv_label_set_text(l_lab, "LEDs (GPIO3_D0/D1/D3/D4)");
    lv_obj_set_style_text_font(l_lab, &lv_font_montserrat_20, 0); lv_obj_align(l_lab, LV_ALIGN_TOP_LEFT, 600, 90);
    const char *led_names[4] = {"LED1", "LED2", "LED3", "LED4"};
    for (int i = 0; i < 4; i++) {
        led_btns[i] = lv_button_create(setting_win); lv_obj_set_size(led_btns[i], 170, 70);
        lv_obj_set_pos(led_btns[i], 600 + (i % 2) * 200, 140 + (i / 2) * 100);
        lv_obj_t *lb = lv_label_create(led_btns[i]); lv_label_set_text_fmt(lb, "%s: OFF", led_names[i]); lv_obj_center(lb);
        lv_obj_add_event_cb(led_btns[i], led_click_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_set_user_data(led_btns[i], (void*)(intptr_t)i);
    }
    lv_obj_t *back_btn = lv_button_create(setting_win); lv_obj_set_size(back_btn, 160, 70);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_RIGHT, -30, -30);
    lv_obj_t *back_lab = lv_label_create(back_btn); lv_label_set_text(back_lab, "Back"); lv_obj_center(back_lab);
    lv_obj_add_event_cb(back_btn, setting_back_click, LV_EVENT_PRESSED, NULL);
    printf("Setting UI created.\n");
}

void setting_btn_click(lv_event_t *e) {
    if (!setting_win) create_setting_win();
    if (aw) lv_obj_add_flag(aw, LV_OBJ_FLAG_HIDDEN);
    if (setting_win) { lv_obj_remove_flag(setting_win, LV_OBJ_FLAG_HIDDEN);
        if (bright_val_label) lv_label_set_text_fmt(bright_val_label, "%d/255", hw_backlight_get()); }
}