#include "globals.h"

lv_obj_t *dw, *aw, *bw, *cw;
lv_obj_t *album_btn, *music_btn, *alenda_btn;
lv_obj_t *pre_btn, *next_btn, *video_btn, *return_btn;
lv_obj_t *game_btn;
lv_obj_t *comm_btn = NULL;
lv_obj_t *back_to_aw_btn = NULL;

/* 日历 */
lv_obj_t *g_calendar = NULL;
lv_obj_t *cal_input_ta = NULL;
lv_calendar_date_t g_cal_mark_dates[50];
int g_cal_mark_count = 0;
#define CAL_MARK_FILE  "/lwx/cal_marks.txt"

/* ========== 日历辅助函数 ========== */
 void load_cal_marks(void) {
    g_cal_mark_count = 0;
    FILE *f = fopen(CAL_MARK_FILE, "r");
    if (!f) return;
    while (fscanf(f, "%d %d %d", &g_cal_mark_dates[g_cal_mark_count].year,
                  &g_cal_mark_dates[g_cal_mark_count].month,
                  &g_cal_mark_dates[g_cal_mark_count].day) == 3) {
        g_cal_mark_count++;
        if (g_cal_mark_count >= 50) break;
    }
    fclose(f);
}

 void save_cal_marks(void) {
    FILE *f = fopen(CAL_MARK_FILE, "w");
    if (!f) return;
    for (int i = 0; i < g_cal_mark_count; i++)
        fprintf(f, "%d %d %d\n", g_cal_mark_dates[i].year, g_cal_mark_dates[i].month, g_cal_mark_dates[i].day);
    fclose(f);
}

 void refresh_cal_marks(lv_obj_t *calendar) {
    if (g_cal_mark_count > 0) lv_calendar_set_highlighted_dates(calendar, g_cal_mark_dates, g_cal_mark_count);
}

 int is_date_marked(int y, int m, int d) {
    for (int i = 0; i < g_cal_mark_count; i++)
        if (g_cal_mark_dates[i].year == y && g_cal_mark_dates[i].month == m && g_cal_mark_dates[i].day == d) return 1;
    return 0;
}

 void mark_date_cb(lv_event_t *e) {
    (void)e;
    const char *text = lv_textarea_get_text(cal_input_ta);
    if (strlen(text) == 0) return;
    int y, m, d;
    if (sscanf(text, "%d-%d-%d", &y, &m, &d) != 3) { printf("格式错误\n"); return; }
    if (m < 1 || m > 12 || d < 1 || d > 31) { printf("日期无效\n"); return; }
    if (is_date_marked(y, m, d)) { printf("已标记过\n"); return; }
    g_cal_mark_dates[g_cal_mark_count].year = y;
    g_cal_mark_dates[g_cal_mark_count].month = m;
    g_cal_mark_dates[g_cal_mark_count].day = d;
    g_cal_mark_count++; save_cal_marks();
    if (g_calendar) refresh_cal_marks(g_calendar);
    printf("标记成功: %d-%d-%d\n", y, m, d);
}

 void del_date_cb(lv_event_t *e) 
 {
    (void)e;
    const char *text = lv_textarea_get_text(cal_input_ta);
    if (strlen(text) == 0) return;
    int y, m, d;
    if (sscanf(text, "%d-%d-%d", &y, &m, &d) != 3) 
    { 
        printf("格式错误\n"); return; 
    }
    for (int i = 0; i < g_cal_mark_count; i++) 
    {
        if (g_cal_mark_dates[i].year == y && g_cal_mark_dates[i].month == m && g_cal_mark_dates[i].day == d) 
        {
            for (int j = i; j < g_cal_mark_count - 1; j++) 
            {
                g_cal_mark_dates[j] = g_cal_mark_dates[j + 1];
            }
            g_cal_mark_count--; save_cal_marks();
            if (g_calendar) refresh_cal_marks(g_calendar);
            printf("已删除: %d-%d-%d\n", y, m, d); return;
        }
    }
    printf("未找到: %d-%d-%d\n", y, m, d);
}

 void cal_input_event_cb(lv_event_t *e) 
 {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *kb = lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED) 
    {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_DEFOCUSED) 
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_READY) mark_date_cb(NULL);
}

/* ========== 界面切换 ========== */
void hide_video_window(void);
void switch_to_music(void) 
{
    if (b1->num == 0) 
    { 
        printf("No music files.\n"); return; 
    }
    hide_video_window();
    if (game_win && !lv_obj_has_flag(game_win, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
    if (cw) lv_obj_remove_flag(cw, LV_OBJ_FLAG_HIDDEN);
    if (bw) lv_obj_add_flag(bw, LV_OBJ_FLAG_HIDDEN);
    if (g_gif) lv_obj_add_flag(g_gif, LV_OBJ_FLAG_HIDDEN);
    if (dw) lv_obj_add_flag(dw, LV_OBJ_FLAG_HIDDEN);
    if (aw) lv_obj_add_flag(aw, LV_OBJ_FLAG_HIDDEN);
    i = 0; printf("Switched to music interface.\n");
}

void switch_to_video(void) {
    if (video_list->num == 0) { printf("No video files.\n"); return; }
    if (!g_video_path) { g_video_index = 0; g_video_path = get_video_path_by_index(0); }
    hide_video_window();
    /* 隐藏其他界面 */
    if (game_win && !lv_obj_has_flag(game_win, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
    if (album_btn) lv_obj_add_flag(album_btn, LV_OBJ_FLAG_HIDDEN);
    if (music_btn) lv_obj_add_flag(music_btn, LV_OBJ_FLAG_HIDDEN);
    if (alenda_btn) lv_obj_add_flag(alenda_btn, LV_OBJ_FLAG_HIDDEN);
    if (video_btn) lv_obj_add_flag(video_btn, LV_OBJ_FLAG_HIDDEN);
    if (g_gif) lv_obj_add_flag(g_gif, LV_OBJ_FLAG_HIDDEN);
    if (cw) lv_obj_add_flag(cw, LV_OBJ_FLAG_HIDDEN);
    if (dw) lv_obj_add_flag(dw, LV_OBJ_FLAG_HIDDEN);
    if (pre_btn) lv_obj_add_flag(pre_btn, LV_OBJ_FLAG_HIDDEN);
    if (next_btn) lv_obj_add_flag(next_btn, LV_OBJ_FLAG_HIDDEN);
    if (return_btn) lv_obj_add_flag(return_btn, LV_OBJ_FLAG_HIDDEN);
    if (bw) lv_obj_add_flag(bw, LV_OBJ_FLAG_HIDDEN);
    /* 改为调用 video_btn_handler，打开控制界面 */
    video_btn_handler(NULL);
}
 void calendar_btn_handler(lv_event_t *e) 
{
    hide_video_window();
    if (bw) lv_obj_add_flag(bw, LV_OBJ_FLAG_HIDDEN);
    if (g_gif) lv_obj_add_flag(g_gif, LV_OBJ_FLAG_HIDDEN);
    if (dw) lv_obj_remove_flag(dw, LV_OBJ_FLAG_HIDDEN);
    if (game_win && !lv_obj_has_flag(game_win, LV_OBJ_FLAG_HIDDEN)) 
    {
        lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
    }
}

 void change_win(lv_event_t *e) 
 {
    if (!aw || !bw || !cw || !dw) 
    { 
        printf("Error: window NULL\n"); return; 
    }
    lv_obj_t *bu = e->original_target; 
    if (!bu) 
    {
        return;
    }
    lv_obj_t *lb = lv_obj_get_child(bu, 0); 
    if (!lb) 
    {
        return;
    }
    char *str = lv_label_get_text(lb); 
    if (!str) 
    {
        return;
    }
    if (strcmp(str, "MENU") == 0) 
    {
        hide_video_window(); lv_obj_add_flag(aw, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(bw, LV_OBJ_FLAG_HIDDEN);
        if (g_gif) 
        {
            lv_obj_add_flag(g_gif, LV_OBJ_FLAG_HIDDEN);
        }   
        if (pre_btn)  
        {
            lv_obj_add_flag(pre_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (next_btn) 
        {
            lv_obj_add_flag(next_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (return_btn) 
        {
            lv_obj_add_flag(return_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (album_btn) 
        {
            lv_obj_remove_flag(album_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (music_btn) 
        {
            lv_obj_remove_flag(music_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (alenda_btn) 
        {
            lv_obj_remove_flag(alenda_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (video_btn) 
        {
            lv_obj_remove_flag(video_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (game_btn) 
        {
            lv_obj_remove_flag(game_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(game_btn, LV_OPA_COVER, 0);
        }
        if (comm_btn) 
        {
            lv_obj_remove_flag(comm_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(comm_btn, LV_OPA_COVER, 0);
        }
        if (chat_btn) 
        {
            lv_obj_remove_flag(chat_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(chat_btn, LV_OPA_COVER, 0);
        }
        if (back_to_aw_btn) 
        {
            lv_obj_remove_flag(back_to_aw_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (game_win && !lv_obj_has_flag(game_win, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
        i = 1;
    } 
    else if (strcmp(str, "album") == 0) 
    {
        hide_video_window();
        if (album_btn) 
        {
            lv_obj_add_flag(album_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (music_btn) 
        {
            lv_obj_add_flag(music_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (alenda_btn) 
        {
            lv_obj_add_flag(alenda_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (video_btn) 
        {
            lv_obj_add_flag(video_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (back_to_aw_btn) 
        {
            lv_obj_add_flag(back_to_aw_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (game_btn) 
        {
            lv_obj_add_flag(game_btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_opa(game_btn, LV_OPA_TRANSP, 0);
        }
        if (comm_btn) 
        { 
            lv_obj_add_flag(comm_btn, LV_OBJ_FLAG_HIDDEN); 
            lv_obj_set_style_opa(comm_btn, LV_OPA_TRANSP, 0); 
        }
        if (chat_btn) 
        { 
            lv_obj_add_flag(chat_btn, LV_OBJ_FLAG_HIDDEN); 
            lv_obj_set_style_opa(chat_btn, LV_OPA_TRANSP, 0); 
        }
        if (game_win && !lv_obj_has_flag(game_win, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
        lv_refr_now(NULL); my_test10();
    } else if (strcmp(str, "return") == 0) {
        if (g_gif) lv_obj_add_flag(g_gif, LV_OBJ_FLAG_HIDDEN);
        if (pre_btn) lv_obj_add_flag(pre_btn, LV_OBJ_FLAG_HIDDEN);
        if (next_btn) lv_obj_add_flag(next_btn, LV_OBJ_FLAG_HIDDEN);
        if (return_btn) lv_obj_add_flag(return_btn, LV_OBJ_FLAG_HIDDEN);
        if (album_btn) lv_obj_remove_flag(album_btn, LV_OBJ_FLAG_HIDDEN);
        if (music_btn) lv_obj_remove_flag(music_btn, LV_OBJ_FLAG_HIDDEN);
        if (alenda_btn) lv_obj_remove_flag(alenda_btn, LV_OBJ_FLAG_HIDDEN);
        if (video_btn) lv_obj_remove_flag(video_btn, LV_OBJ_FLAG_HIDDEN);
        if (back_to_aw_btn) lv_obj_remove_flag(back_to_aw_btn, LV_OBJ_FLAG_HIDDEN);
        if (game_btn) 
        { 
            lv_obj_remove_flag(game_btn, LV_OBJ_FLAG_HIDDEN); 
            lv_obj_set_style_opa(game_btn, LV_OPA_COVER, 0); 
            lv_obj_remove_state(game_btn, LV_STATE_DISABLED); 
        }
        if (comm_btn) 
        { 
            lv_obj_remove_flag(comm_btn, LV_OBJ_FLAG_HIDDEN); 
            lv_obj_set_style_opa(comm_btn, LV_OPA_COVER, 0); 
            lv_obj_remove_state(comm_btn, LV_STATE_DISABLED); 
        }
        if (chat_btn) 
        { 
            lv_obj_remove_flag(chat_btn, LV_OBJ_FLAG_HIDDEN); 
            lv_obj_set_style_opa(chat_btn, LV_OPA_COVER, 0); 
            lv_obj_remove_state(chat_btn, LV_STATE_DISABLED); 
        }
        i = 0;
        if (dw) lv_obj_add_flag(dw, LV_OBJ_FLAG_HIDDEN); if (cw) lv_obj_add_flag(cw, LV_OBJ_FLAG_HIDDEN);
        if (aw) lv_obj_add_flag(aw, LV_OBJ_FLAG_HIDDEN); if (bw) lv_obj_remove_flag(bw, LV_OBJ_FLAG_HIDDEN);
        if (game_win && !lv_obj_has_flag(game_win, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
    } else if (strcmp(str, "music") == 0) {
        if (b1->num == 0) printf("No music files. Use Add.\n");
        hide_video_window();
        if (cw) lv_obj_remove_flag(cw, LV_OBJ_FLAG_HIDDEN);
        if (bw) lv_obj_add_flag(bw, LV_OBJ_FLAG_HIDDEN);
        if (g_gif) lv_obj_add_flag(g_gif, LV_OBJ_FLAG_HIDDEN);
        if (game_win && !lv_obj_has_flag(game_win, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
        i = 0;
    }
}

 void back_to_aw_click(lv_event_t *e) {
    if (aw) lv_obj_remove_flag(aw, LV_OBJ_FLAG_HIDDEN);
    if (bw) lv_obj_add_flag(bw, LV_OBJ_FLAG_HIDDEN);
    if (cw) lv_obj_add_flag(cw, LV_OBJ_FLAG_HIDDEN); if (dw) lv_obj_add_flag(dw, LV_OBJ_FLAG_HIDDEN);
    if (g_gif) lv_obj_add_flag(g_gif, LV_OBJ_FLAG_HIDDEN);
    if (pre_btn) lv_obj_add_flag(pre_btn, LV_OBJ_FLAG_HIDDEN);
    if (next_btn) lv_obj_add_flag(next_btn, LV_OBJ_FLAG_HIDDEN);
    if (return_btn) lv_obj_add_flag(return_btn, LV_OBJ_FLAG_HIDDEN);
    if (game_win && !lv_obj_has_flag(game_win, LV_OBJ_FLAG_HIDDEN)) lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
    if (video_win) lv_obj_add_flag(video_win, LV_OBJ_FLAG_HIDDEN);
    if (setting_win) lv_obj_add_flag(setting_win, LV_OBJ_FLAG_HIDDEN);
    printf("Returned to aw\n");
}

/* ========== 创建界面 ========== */
 void create_aw(void) {
    aw = lv_obj_create(lv_screen_active()); 
    if (!aw) 
    {
        return;
    }
    lv_obj_set_size(aw, 1024, 600); 
    lv_obj_set_pos(aw, 0, 0);
    lv_obj_remove_flag(aw, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(aw, 0, 0); 
    lv_obj_set_style_shadow_width(aw, 0, 0);
    lv_obj_set_style_border_width(aw, 0, 0);
    lv_obj_set_style_outline_width(aw, 0, 0);
    lv_obj_set_style_pad_all(aw, 0, 0); 
    lv_obj_set_style_margin_all(aw, 0, 0);
    static lv_style_t as; lv_style_init(&as); 
    lv_style_set_bg_color(&as, lv_color_hex(0xFFFF00));
    lv_obj_add_style(aw, &as, LV_STATE_DEFAULT);
    lv_obj_t *img = lv_image_create(aw);
    if (img) 
    { 
        lv_image_set_src(img, "A:/lwx/bmp/8.bmp"); 
        lv_obj_set_size(img, 1024, 600); 
        lv_obj_set_pos(img, 0, 0); 
        lv_obj_move_background(img); 
    }
    lv_obj_t *abu = lv_button_create(aw);
    if (abu) 
    { 
        lv_obj_set_size(abu, 200, 100); 
        lv_obj_set_align(abu, LV_ALIGN_CENTER);
        lv_obj_t *alb = lv_label_create(abu); 
        if (alb) 
        { 
            lv_label_set_text(alb, "MENU"); 
            lv_obj_center(alb); 
        }
        lv_obj_add_event_cb(abu, change_win, LV_EVENT_PRESSED, NULL); 
    }
    lv_obj_t *set_btn = lv_button_create(aw);
    if (set_btn) 
    { 
        lv_obj_set_size(set_btn, 200, 100); 
        lv_obj_align_to(set_btn, abu, LV_ALIGN_OUT_RIGHT_MID, 40, 0);
        lv_obj_t *set_lb = lv_label_create(set_btn); 
        if (set_lb) 
        { 
            lv_label_set_text(set_lb, "SET"); 
            lv_obj_center(set_lb); 
        }
        lv_obj_add_event_cb(set_btn, setting_btn_click, LV_EVENT_PRESSED, NULL); 
    }
}

 void create_bw(void) {
    bw = lv_obj_create(lv_screen_active()); 
    if (!bw) 
    {
        return;
    }
    lv_obj_set_size(bw, 1024, 600); 
    lv_obj_set_pos(bw, 0, 0); 
    lv_obj_remove_flag(bw, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(bw, 0, 0); 
    lv_obj_set_style_shadow_width(bw, 0, 0);
    lv_obj_set_style_border_width(bw, 0, 0); 
    lv_obj_set_style_outline_width(bw, 0, 0);
    lv_obj_set_style_pad_all(bw, 0, 0); 
    lv_obj_set_style_margin_all(bw, 0, 0);
    lv_obj_set_style_bg_opa(bw, LV_OPA_TRANSP, 0);

    lv_obj_t *bw_bg = lv_image_create(bw);
    if (bw_bg) 
    { 
        lv_image_set_src(bw_bg, "A:/lwx/bmp/7.bmp"); 
        lv_obj_set_size(bw_bg, 1024, 600); 
        lv_obj_set_pos(bw_bg, 0, 0); 
        lv_obj_move_background(bw_bg); 
    }

    back_to_aw_btn = lv_button_create(bw);
    if (back_to_aw_btn) 
    { 
        lv_obj_set_size(back_to_aw_btn, 100, 50); 
        lv_obj_set_pos(back_to_aw_btn, 10, 10);
        lv_obj_t *label = lv_label_create(back_to_aw_btn); 
        if (label) 
        { 
            lv_label_set_text(label, "Back"); 
            lv_obj_center(label); 
        }
        lv_obj_add_event_cb(back_to_aw_btn, back_to_aw_click, LV_EVENT_PRESSED, NULL); 
    }

    album_btn = lv_button_create(bw); 
    if (album_btn) 
    { 
        lv_obj_set_size(album_btn, 50, 50); 
        lv_obj_set_pos(album_btn, 400, 200);
        lv_obj_t *blb = lv_label_create(album_btn); 
        if (blb) 
        { 
            lv_label_set_text(blb, "album"); 
            lv_obj_center(blb); 
        }
        lv_obj_add_event_cb(album_btn, change_win, LV_EVENT_PRESSED, NULL); 
    }

    alenda_btn = lv_button_create(bw); 
    if (alenda_btn) 
    { 
        lv_obj_set_size(alenda_btn, 50, 50); 
        lv_obj_set_pos(alenda_btn, 400, 300);
        lv_obj_t *cal_lb = lv_label_create(alenda_btn); 
        if (cal_lb) 
        { 
            lv_label_set_text(cal_lb, "calendar"); 
            lv_obj_center(cal_lb); 
        }
        lv_obj_add_event_cb(alenda_btn, calendar_btn_handler, LV_EVENT_PRESSED, NULL); 
    }

    game_btn = lv_button_create(bw); 
    if (game_btn) 
    { 
        lv_obj_set_size(game_btn, 50, 50); 
        lv_obj_set_pos(game_btn, 400, 400);
        lv_obj_t *blb_game = lv_label_create(game_btn); 
        if (blb_game) 
        { 
            lv_label_set_text(blb_game, "game"); 
            lv_obj_center(blb_game); 
        }
        lv_obj_add_event_cb(game_btn, game_btn_handler, LV_EVENT_PRESSED, NULL); 
    }

    music_btn = lv_button_create(bw); 
    if (music_btn) 
    { 
        lv_obj_set_size(music_btn, 50, 50); 
        lv_obj_set_pos(music_btn, 600, 200);
        lv_obj_t *blb4 = lv_label_create(music_btn); 
        if (blb4) 
        { 
            lv_label_set_text(blb4, "music"); 
            lv_obj_center(blb4); 
        }
        lv_obj_add_event_cb(music_btn, change_win, LV_EVENT_PRESSED, NULL); 
    }

    video_btn = lv_button_create(bw); 
    if (video_btn) 
    { 
        lv_obj_set_size(video_btn, 50, 50); 
        lv_obj_set_pos(video_btn, 600, 300);
        lv_obj_t *blb5 = lv_label_create(video_btn); 
        if (blb5) 
        { 
            lv_label_set_text(blb5, "video"); 
            lv_obj_center(blb5); 
        }
        lv_obj_add_event_cb(video_btn, video_btn_handler, LV_EVENT_PRESSED, NULL); 
    }

    comm_btn = lv_button_create(bw); 
    if (comm_btn) 
    { 
        lv_obj_set_size(comm_btn, 50, 50); 
        lv_obj_set_pos(comm_btn, 600, 400);
        lv_obj_t *comm_label = lv_label_create(comm_btn); 
        if (comm_label) 
        { 
            lv_label_set_text(comm_label, "Connect"); 
            lv_obj_center(comm_label); 
        }
        lv_obj_add_event_cb(comm_btn, comm_btn_click, LV_EVENT_PRESSED, NULL); 
    }

    chat_btn = lv_button_create(bw); 
    if (chat_btn) 
    { 
        lv_obj_set_size(chat_btn, 50, 50); 
        lv_obj_set_pos(chat_btn, 700, 400);
        lv_obj_t *chat_label = lv_label_create(chat_btn); 
        if (chat_label) 
        { 
            lv_label_set_text(chat_label, "Chat"); 
            lv_obj_center(chat_label); 
        }
        lv_obj_add_event_cb(chat_btn, chat_btn_click, LV_EVENT_PRESSED, NULL); 
    }

    pre_btn = lv_button_create(bw); 
    if (pre_btn) 
    { 
        lv_obj_set_size(pre_btn, 200, 100); 
        lv_obj_set_align(pre_btn, LV_ALIGN_LEFT_MID);
        lv_obj_t *blb1 = lv_label_create(pre_btn); 
        if (blb1) 
        { 
            lv_label_set_text(blb1, "pre"); 
            lv_obj_center(blb1); 
        }
        lv_obj_add_event_cb(pre_btn, pre_click, LV_EVENT_PRESSED, NULL); 
        lv_obj_add_flag(pre_btn, LV_OBJ_FLAG_HIDDEN); 
    }

    next_btn = lv_button_create(bw); 
    if (next_btn) 
    { 
        lv_obj_set_size(next_btn, 200, 100); 
        lv_obj_set_align(next_btn, LV_ALIGN_RIGHT_MID);
        lv_obj_t *blb2 = lv_label_create(next_btn); 
        if (blb2) 
        { 
            lv_label_set_text(blb2, "next"); 
            lv_obj_center(blb2); 
        }
        lv_obj_add_event_cb(next_btn, next_click, LV_EVENT_PRESSED, NULL); 
        lv_obj_add_flag(next_btn, LV_OBJ_FLAG_HIDDEN); 
    }

    return_btn = lv_button_create(bw); 
    if (return_btn) 
    { 
        lv_obj_set_size(return_btn, 200, 100); 
        lv_obj_set_align(return_btn, LV_ALIGN_TOP_RIGHT);
        lv_obj_t *blb3 = lv_label_create(return_btn); 
        if (blb3) 
        { 
            lv_label_set_text(blb3, "return"); 
            lv_obj_center(blb3); 
        }
        lv_obj_add_event_cb(return_btn, change_win, LV_EVENT_PRESSED, NULL); 
        lv_obj_add_flag(return_btn, LV_OBJ_FLAG_HIDDEN); 
    }
}

 void create_cw(void) 
 {
    cw = lv_obj_create(lv_screen_active()); 
    if (!cw) 
    {
        return;
    }
    lv_obj_set_size(cw, 1024, 600); 
    lv_obj_set_pos(cw, 0, 0); 
    lv_obj_remove_flag(cw, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(cw, 0, 0); 
    lv_obj_set_style_shadow_width(cw, 0, 0);
    lv_obj_set_style_border_width(cw, 0, 0); 
    lv_obj_set_style_outline_width(cw, 0, 0);
    lv_obj_set_style_pad_all(cw, 0, 0); 
    lv_obj_set_style_margin_all(cw, 0, 0);
    lv_obj_set_style_bg_opa(cw, LV_OPA_TRANSP, 0);
    lv_obj_t *cw_bg = lv_image_create(cw);
    if (cw_bg) 
    { 
        lv_image_set_src(cw_bg, "A:/lwx/bmp/9.bmp"); 
        lv_obj_set_size(cw_bg, 1024, 600); 
        lv_obj_set_pos(cw_bg, 0, 0); 
        lv_obj_move_background(cw_bg); 
    }

    lv_obj_t *add_music_btn = lv_button_create(cw);
    if (add_music_btn) 
    { 
        lv_obj_set_size(add_music_btn, 80, 50); 
        lv_obj_align(add_music_btn, LV_ALIGN_TOP_LEFT, 10, 10);
        lv_obj_t *add_music_label = lv_label_create(add_music_btn); 
        if (add_music_label)
        { 
            lv_label_set_text(add_music_label, "Add");
            lv_obj_center(add_music_label); 
        }
        lv_obj_add_event_cb(add_music_btn, music_add_click, LV_EVENT_PRESSED, NULL); 
    }

    lv_obj_t *del_music_btn = lv_button_create(cw);
    if (del_music_btn) 
    {
        lv_obj_set_size(del_music_btn, 80, 50);
        lv_obj_align(del_music_btn, LV_ALIGN_TOP_LEFT, 100, 10);
        lv_obj_t *del_music_label = lv_label_create(del_music_btn);
        if (del_music_label) 
        { 
            lv_label_set_text(del_music_label, "Del"); 
            lv_obj_center(del_music_label); 
        }
        lv_obj_add_event_cb(del_music_btn, music_delete_click, LV_EVENT_PRESSED, NULL);
    }

    lv_obj_t *cb1 = lv_button_create(cw); 
    if (cb1) 
    { 
        lv_obj_set_size(cb1, 200, 100); 
        lv_obj_set_align(cb1, LV_ALIGN_LEFT_MID);
        lv_obj_t *clb1 = lv_label_create(cb1); 
        if (clb1) 
        { 
            lv_label_set_text(clb1, "pre song"); lv_obj_center(clb1); 
        }
        lv_obj_add_event_cb(cb1, music_prev_click, LV_EVENT_PRESSED, NULL); 
    }

    lv_obj_t *cb2 = lv_button_create(cw); 
    if (cb2) 
    { 
        lv_obj_set_size(cb2, 200, 100); 
        lv_obj_set_align(cb2, LV_ALIGN_RIGHT_MID);
        lv_obj_t *clb2 = lv_label_create(cb2); 
        if (clb2) 
        { 
            lv_label_set_text(clb2, "next one"); lv_obj_center(clb2); 
        }
        lv_obj_add_event_cb(cb2, music_next_click, LV_EVENT_PRESSED, NULL);
     }

    lv_obj_t *cb3 = lv_button_create(cw); 
    if (cb3) 
    { 
        lv_obj_set_size(cb3, 200, 100); 
        lv_obj_set_align(cb3, LV_ALIGN_TOP_RIGHT);
        lv_obj_t *clb3 = lv_label_create(cb3); 
        if (clb3) 
        { 
            lv_label_set_text(clb3, "return"); lv_obj_center(clb3); 
        }
        lv_obj_add_event_cb(cb3, change_win, LV_EVENT_PRESSED, NULL); 
    }

    music_play_btn = lv_button_create(cw); 
    if (music_play_btn) 
    { 
        lv_obj_set_size(music_play_btn, 100, 100); 
        lv_obj_set_align(music_play_btn, LV_ALIGN_CENTER);
        lv_obj_t *cl4 = lv_label_create(music_play_btn); 
        if (cl4) 
        { 
            lv_label_set_text(cl4, "Play"); lv_obj_center(cl4); 
        }
        lv_obj_add_event_cb(music_play_btn, music_play_click, LV_EVENT_PRESSED, NULL); 
    }
}

 void create_dw(void) 
 {
    dw = lv_obj_create(lv_screen_active()); 
    if (!dw) 
    {
        return;
    }
    lv_obj_set_size(dw, 1024, 600); 
    lv_obj_set_pos(dw, 0, 0); 
    lv_obj_remove_flag(dw, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(dw, 0, 0); 
    lv_obj_set_style_shadow_width(dw, 0, 0);
    lv_obj_set_style_border_width(dw, 0, 0); 
    lv_obj_set_style_outline_width(dw, 0, 0);
    lv_obj_set_style_pad_all(dw, 0, 0); 
    lv_obj_set_style_margin_all(dw, 0, 0);
    lv_obj_set_style_bg_color(dw, lv_color_hex(0xFFFFFF), 0);
    load_cal_marks();
    lv_obj_t *calendar = lv_calendar_create(dw);

    g_calendar = calendar;
    if (calendar) 
    {
        lv_obj_set_size(calendar, 600, 500); 
        lv_obj_align(calendar, LV_ALIGN_CENTER, 0, -30);
        lv_calendar_header_arrow_create(calendar); 
        lv_obj_set_style_text_color(calendar, lv_color_hex(0x000000), 0);
        refresh_cal_marks(calendar);
        time_t now = time(NULL); 
        struct tm *tm_info = localtime(&now);
        lv_calendar_date_t today = {tm_info->tm_year+1900, tm_info->tm_mon+1, tm_info->tm_mday};
        lv_calendar_set_today_date(calendar, today.year, today.month, today.day);
        lv_calendar_set_showed_date(calendar, today.year, today.month);
    }

    cal_input_ta = lv_textarea_create(dw);
    if (cal_input_ta) 
    { 
        lv_obj_set_size(cal_input_ta, 200, 50); 
        lv_obj_align(cal_input_ta, LV_ALIGN_BOTTOM_LEFT, 30, -30);
        lv_textarea_set_placeholder_text(cal_input_ta, "YYYY-MM-DD"); 
        lv_obj_set_style_bg_color(cal_input_ta, lv_color_hex(0xFFFFFF), 0); 
    }

    lv_obj_t *cal_kb = lv_keyboard_create(lv_screen_active());
    if (cal_kb) 
    { 
        lv_obj_add_flag(cal_kb, LV_OBJ_FLAG_HIDDEN); 
        lv_obj_set_size(cal_kb, 800, 200); 
        lv_obj_align(cal_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_event_cb(cal_input_ta, cal_input_event_cb, LV_EVENT_ALL, cal_kb); 
    }

    lv_obj_t *mark_btn = lv_button_create(dw);
    if (mark_btn) 
    { 
        lv_obj_set_size(mark_btn, 120, 50); 
        lv_obj_align_to(mark_btn, cal_input_ta, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
        lv_obj_t *mk_label = lv_label_create(mark_btn); 
        lv_label_set_text(mk_label, "Mark"); lv_obj_center(mk_label);
        lv_obj_add_event_cb(mark_btn, mark_date_cb, LV_EVENT_PRESSED, NULL); 
    }

    lv_obj_t *del_btn = lv_button_create(dw);
    if (del_btn) 
    { 
        lv_obj_set_size(del_btn, 120, 50); 
        lv_obj_align_to(del_btn, mark_btn, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
        lv_obj_t *del_label = lv_label_create(del_btn); 
        lv_label_set_text(del_label, "Del"); 
        lv_obj_center(del_label);
        lv_obj_add_event_cb(del_btn, del_date_cb, LV_EVENT_PRESSED, NULL); 
    }

    lv_obj_t *back_btn = lv_button_create(dw);
    if (back_btn) 
    { 
        lv_obj_set_size(back_btn, 200, 80); 
        lv_obj_align(back_btn, LV_ALIGN_BOTTOM_RIGHT, -30, -30);
        lv_obj_t *back_label = lv_label_create(back_btn);
        if (back_label) 
        { 
            lv_label_set_text(back_label, "return"); 
            lv_obj_center(back_label);
         }
        lv_obj_add_event_cb(back_btn, change_win, LV_EVENT_PRESSED, NULL);
    }
}

void my_test8(void) {
    create_aw(); 
    create_bw(); 
    create_cw(); 
    create_dw();
    create_setting_win();
    if (bw) lv_obj_add_flag(bw, LV_OBJ_FLAG_HIDDEN);
    if (cw) lv_obj_add_flag(cw, LV_OBJ_FLAG_HIDDEN);
    if (dw) lv_obj_add_flag(dw, LV_OBJ_FLAG_HIDDEN);
}