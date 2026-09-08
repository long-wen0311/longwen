#include "globals.h"

/* ========== 密码相关 ========== */
#define PWD_FILE "/lwx/password.txt"
char g_pwd1[64] = "lwx";
char g_pwd2[64] = "123";
int login_stage = 0;
lv_obj_t *login_ta = NULL;
lv_obj_t *login_kb = NULL;
lv_obj_t *login_tip = NULL;

void load_passwords(void) {
    FILE *f = fopen(PWD_FILE, "r");
    if (f) {
        if (fgets(g_pwd1, sizeof(g_pwd1), f)) {
            char *nl = strchr(g_pwd1, '\n'); if (nl) *nl = '\0';
        }
        if (fgets(g_pwd2, sizeof(g_pwd2), f)) {
            char *nl = strchr(g_pwd2, '\n'); if (nl) *nl = '\0';
        }
        fclose(f);
    }
}

void save_passwords(void) {
    FILE *f = fopen(PWD_FILE, "w");
    if (f) {
        fprintf(f, "%s\n%s\n", g_pwd1, g_pwd2);
        fclose(f);
    }
}

 void close_tip_cb(lv_timer_t *timer) {
    if (login_tip) { lv_obj_del(login_tip); login_tip = NULL; }
}

 void ta_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
    static char new_pwd[64] = "";

    if (code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_READY) {
        const char *text = lv_textarea_get_text(ta);
        if (login_stage == 0) {
            if (strcmp(text, g_pwd1) == 0) {
                printf("登录成功\n");
                lv_obj_del(login_kb); login_kb = NULL;
                lv_obj_del(login_ta); login_ta = NULL;
                my_test8();
                return;
            }
            printf("密码错误\n");
            lv_textarea_set_text(ta, "");
        } else if (login_stage == 1) {
            strncpy(new_pwd, text, sizeof(new_pwd) - 1);
            new_pwd[sizeof(new_pwd) - 1] = '\0';
            lv_textarea_set_text(ta, "");
            lv_textarea_set_placeholder_text(ta, "二级密码");
            login_stage = 2;
            printf("请输入二级密码确认\n");
        } else if (login_stage == 2) {
            if (strcmp(text, g_pwd2) == 0) {
                strncpy(g_pwd1, new_pwd, sizeof(g_pwd1) - 1);
                save_passwords();
                printf("修改成功\n");
                if (login_tip) lv_obj_del(login_tip);
                login_tip = lv_obj_create(lv_screen_active());
                lv_obj_set_size(login_tip, 300, 150);
                lv_obj_center(login_tip);
                lv_obj_set_style_bg_color(login_tip, lv_color_hex(0x00AA00), 0);
                lv_obj_t *tl = lv_label_create(login_tip);
                lv_label_set_text(tl, "修改成功");
                lv_obj_center(tl);
                lv_obj_set_style_text_font(tl, &lv_myfont_30, 0);
                lv_timer_t *t = lv_timer_create(close_tip_cb, 2000, NULL);
                lv_timer_set_repeat_count(t, 1);
                lv_textarea_set_text(ta, "");
                lv_textarea_set_placeholder_text(ta, "一级密码");
                login_stage = 0;
                new_pwd[0] = '\0';
            } else {
                printf("二级密码错误\n");
                if (login_tip) lv_obj_del(login_tip);
                login_tip = lv_obj_create(lv_screen_active());
                lv_obj_set_size(login_tip, 300, 150);
                lv_obj_center(login_tip);
                lv_obj_set_style_bg_color(login_tip, lv_color_hex(0xAA0000), 0);
                lv_obj_t *tl = lv_label_create(login_tip);
                lv_label_set_text(tl, "密码错误");
                lv_obj_center(tl);
                lv_obj_set_style_text_font(tl, &lv_myfont_30, 0);
                lv_timer_t *t = lv_timer_create(close_tip_cb, 2000, NULL);
                lv_timer_set_repeat_count(t, 1);
                lv_textarea_set_text(ta, "");
                lv_textarea_set_placeholder_text(ta, "一级密码");
                login_stage = 0;
                new_pwd[0] = '\0';
            }
        }
    }
}

 void login_btn_click(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lb = lv_obj_get_child(btn, 0);
    const char *str = lv_label_get_text(lb);

    if (!login_ta) {
        login_ta = lv_textarea_create(lv_screen_active());
        lv_obj_align(login_ta, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(login_ta, 250, 60);
        lv_obj_set_style_bg_color(login_ta, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(login_ta, &lv_myfont_30, 0);
    }
    if (!login_kb) {
        login_kb = lv_keyboard_create(lv_screen_active());
        lv_obj_set_size(login_kb, 800, 200);
        lv_obj_align(login_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_flag(login_kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(login_ta, ta_event_cb, LV_EVENT_ALL, login_kb);
    }

    if (strcmp(str, "修改密码") == 0) {
        login_stage = 1;
        lv_textarea_set_text(login_ta, "");
        lv_textarea_set_placeholder_text(login_ta, "新密码");
    } else {
        login_stage = 0;
        lv_textarea_set_text(login_ta, "");
        lv_textarea_set_placeholder_text(login_ta, "一级密码");
    }
    lv_obj_remove_flag(login_ta, LV_OBJ_FLAG_HIDDEN);
}

void lv_example_keyboard_1(void) {
    load_passwords();

    lv_obj_t *bg = lv_image_create(lv_screen_active());
    lv_image_set_src(bg, "A:/lwx/bmp/9.bmp");
    lv_obj_set_size(bg, 1024, 600);
    lv_obj_set_pos(bg, 0, 0);

    lv_obj_t *btn1 = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn1, 200, 80);
    lv_obj_align(btn1, LV_ALIGN_CENTER, -120, 50);
    lv_obj_t *lb1 = lv_label_create(btn1);
    lv_label_set_text(lb1, "输入密码");
    lv_obj_center(lb1);
    lv_obj_set_style_text_font(lb1, &lv_myfont_30, 0);
    lv_obj_add_event_cb(btn1, login_btn_click, LV_EVENT_PRESSED, NULL);

    lv_obj_t *btn2 = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn2, 200, 80);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 120, 50);
    lv_obj_t *lb2 = lv_label_create(btn2);
    lv_label_set_text(lb2, "修改密码");
    lv_obj_center(lb2);
    lv_obj_set_style_text_font(lb2, &lv_myfont_30, 0);
    lv_obj_add_event_cb(btn2, login_btn_click, LV_EVENT_PRESSED, NULL);
}