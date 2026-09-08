#include "globals.h"

#define GRID_SIZE 4
#define TOTAL_HOLES (GRID_SIZE * GRID_SIZE)

/* 难度参数 */
#define MIN_MOLES          1
#define MAX_MOLES_BASE     3
#define MAX_MOLES_INCR     1
#define MAX_MOLES_LIMIT    6
#define SPAWN_INTERVAL_BASE  700
#define SPAWN_INTERVAL_MIN   300
#define HIGHSCORE_FILE "/lwx/highscore.txt"

lv_obj_t *game_win = NULL;
lv_obj_t *game_return_btn = NULL;
lv_obj_t *game_score_label = NULL;
lv_obj_t *game_timer_label = NULL;
lv_obj_t *game_holes[TOTAL_HOLES];
int game_score = 0;
int game_time_left = 30;
lv_timer_t *game_timer = NULL;
lv_timer_t *game_spawn_timer = NULL;
int game_running = 0;
int high_score = 0;
lv_obj_t *game_dialog = NULL;
lv_obj_t *game_start_btn = NULL;
static lv_timer_t *g_hit_timer = NULL;

static lv_obj_t *gctrl_diff_lb = NULL;
/* 游戏控制界面 */
static lv_obj_t *gctrl_win = NULL;
static int g_diff = 0;  /* 0=简单, 1=普通, 2=困难 */
static const char *g_diff_names[] = {"简单", "普通", "困难"};

/* 暂停对话框 */
static lv_obj_t *pause_dialog = NULL;

/* 难度系数 */
static int get_spawn_interval(void) {
    int base[] = {900, 700, 500};
    return base[g_diff];
}
static int get_max_moles(void) {
    int max[] = {2, 3, 5};
    return max[g_diff] + (game_score / 5) * MAX_MOLES_INCR;
}

void load_high_score(void) {
    FILE *fp = fopen(HIGHSCORE_FILE, "r");
    if (fp) { if (fscanf(fp, "%d", &high_score) != 1) high_score = 0; fclose(fp); }
    else high_score = 0;
}

static void save_high_score(void) {
    FILE *fp = fopen(HIGHSCORE_FILE, "w");
    if (fp) { fprintf(fp, "%d\n", high_score); fclose(fp); }
}

static void game_hole_reset(lv_obj_t *hole) {
    if (!hole) return;
    lv_obj_set_style_bg_color(hole, lv_color_hex(0x8B4513), 0);
    lv_obj_t *label = lv_obj_get_child(hole, 0);
    if (label) lv_label_set_text(label, "");
    lv_obj_set_user_data(hole, (void*)0);
    uint32_t n = lv_obj_get_child_count(hole);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *c = lv_obj_get_child(hole, i);
        int tag = (int)(intptr_t)lv_obj_get_user_data(c);
        if (tag == 1 || tag == 2) lv_obj_add_flag(c, LV_OBJ_FLAG_HIDDEN);
    }
}

static void game_star_cb(lv_timer_t *timer) {
    lv_obj_t *hole = lv_timer_get_user_data(timer);
    if (hole) game_hole_reset(hole);
}

static void game_hit_cb(lv_timer_t *timer) {
    lv_obj_t *hole = lv_timer_get_user_data(timer);
    if (!hole) return;
    int has_mole = (int)(intptr_t)lv_obj_get_user_data(hole);
    uint32_t n = lv_obj_get_child_count(hole);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *c = lv_obj_get_child(hole, i);
        int tag = (int)(intptr_t)lv_obj_get_user_data(c);
        if (tag == 1) lv_obj_add_flag(c, LV_OBJ_FLAG_HIDDEN);
        else if (tag == 2 && has_mole) lv_obj_remove_flag(c, LV_OBJ_FLAG_HIDDEN);
    }
    if (has_mole) {
        lv_obj_set_user_data(hole, (void*)0);
        lv_timer_t *t2 = lv_timer_create(game_star_cb, 400, hole);
        lv_timer_set_repeat_count(t2, 1);
    } else { game_hole_reset(hole); }
}

static void game_hole_click(lv_event_t *e) {
    if (!game_running) return;
    lv_obj_t *hole = lv_event_get_target(e);
    int has_mole = (int)(intptr_t)lv_obj_get_user_data(hole);
    uint32_t n = lv_obj_get_child_count(hole);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *c = lv_obj_get_child(hole, i);
        int tag = (int)(intptr_t)lv_obj_get_user_data(c);
        if (tag == 1) lv_obj_remove_flag(c, LV_OBJ_FLAG_HIDDEN);
        else if (tag == 2) lv_obj_add_flag(c, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_t *label = lv_obj_get_child(hole, 0);
    if (label) lv_label_set_text(label, "");
    if (has_mole) { game_score++; lv_label_set_text_fmt(game_score_label, "Score: %d", game_score); }
    lv_timer_t *t = lv_timer_create(game_hit_cb, 150, hole);
    lv_timer_set_repeat_count(t, 1);
}

static void game_spawn_timer_cb(lv_timer_t *timer) {
    if (!game_running) return;
    for (int i = 0; i < TOTAL_HOLES; i++) game_hole_reset(game_holes[i]);
    int max_allowed = get_max_moles();
    if (max_allowed > MAX_MOLES_LIMIT) max_allowed = MAX_MOLES_LIMIT;
    if (max_allowed > TOTAL_HOLES) max_allowed = TOTAL_HOLES;
    int count = MIN_MOLES + rand() % (max_allowed - MIN_MOLES + 1);
    int indices[TOTAL_HOLES];
    for (int i = 0; i < TOTAL_HOLES; i++) indices[i] = i;
    for (int i = TOTAL_HOLES - 1; i > 0; i--) {
        int j = rand() % (i + 1); int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
    }
    for (int k = 0; k < count; k++) {
        int idx = indices[k];
        lv_obj_t *hole = game_holes[idx];
        lv_obj_set_style_bg_color(hole, lv_color_hex(0xFFD700), 0);
        lv_obj_t *label = lv_obj_get_child(hole, 0);
        if (label) lv_label_set_text(label, "🐹");
        lv_obj_set_user_data(hole, (void*)1);
    }
    int interval = get_spawn_interval() - (game_score / 10) * 50;
    if (interval < SPAWN_INTERVAL_MIN) interval = SPAWN_INTERVAL_MIN;
    if (game_spawn_timer) lv_timer_set_period(game_spawn_timer, interval);
}

static void game_timer_cb(lv_timer_t *timer);

/* ========== 暂停对话框 ========== */
static void pause_continue_click(lv_event_t *e) {
    if (pause_dialog) { lv_obj_del(pause_dialog); pause_dialog = NULL; }
    /* 继续游戏 */
    game_running = 1;
    if (game_timer) lv_timer_resume(game_timer);
    if (game_spawn_timer) lv_timer_resume(game_spawn_timer);
    if (game_return_btn) lv_obj_remove_flag(game_return_btn, LV_OBJ_FLAG_HIDDEN);
}

static void pause_exit_click(lv_event_t *e) {
    if (pause_dialog) { lv_obj_del(pause_dialog); pause_dialog = NULL; }
    /* 退出到控制界面 */
    game_running = 0;
    if (game_timer) { lv_timer_del(game_timer); game_timer = NULL; }
    if (game_spawn_timer) { lv_timer_del(game_spawn_timer); game_spawn_timer = NULL; }
    if (game_win) lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
    if (gctrl_win) lv_obj_remove_flag(gctrl_win, LV_OBJ_FLAG_HIDDEN);
    /* 复位洞 */
    for (int i = 0; i < TOTAL_HOLES; i++) game_hole_reset(game_holes[i]);
    game_score = 0; game_time_left = 30;
}

/* ========== 修改 game_return_click ========== */
static void game_return_click(lv_event_t *e) {
    if (game_dialog != NULL) { printf("Dialog active.\n"); return; }
    if (!game_running) return;
    /* 暂停游戏 */
    game_running = 0;
    if (game_timer) lv_timer_pause(game_timer);
    if (game_spawn_timer) lv_timer_pause(game_spawn_timer);
    if (game_return_btn) lv_obj_add_flag(game_return_btn, LV_OBJ_FLAG_HIDDEN);

    /* 弹出暂停对话框 */
    pause_dialog = lv_obj_create(lv_screen_active());
    lv_obj_set_size(pause_dialog, 350, 250);
    lv_obj_center(pause_dialog);
    lv_obj_set_style_bg_color(pause_dialog, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(pause_dialog, 2, 0);
    lv_obj_set_style_radius(pause_dialog, 10, 0);

    lv_obj_t *title = lv_label_create(pause_dialog);
    lv_label_set_text(title, "游戏暂停");
    lv_obj_set_style_text_font(title, &lv_myfont_30, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *score_lb = lv_label_create(pause_dialog);
    lv_label_set_text_fmt(score_lb, "当前得分: %d", game_score);
    lv_obj_set_style_text_font(score_lb, &lv_myfont_30, 0);
    lv_obj_align(score_lb, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *cont_btn = lv_button_create(pause_dialog);
    lv_obj_set_size(cont_btn, 120, 50);
    lv_obj_align(cont_btn, LV_ALIGN_BOTTOM_LEFT, 30, -30);
    lv_obj_t *cl = lv_label_create(cont_btn); lv_label_set_text(cl, "继续"); lv_obj_center(cl);
    lv_obj_set_style_text_font(cl, &lv_myfont_30, 0);
    lv_obj_add_event_cb(cont_btn, pause_continue_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t *exit_btn = lv_button_create(pause_dialog);
    lv_obj_set_size(exit_btn, 120, 50);
    lv_obj_align(exit_btn, LV_ALIGN_BOTTOM_RIGHT, -30, -30);
    lv_obj_t *el = lv_label_create(exit_btn); lv_label_set_text(el, "退出"); lv_obj_center(el);
    lv_obj_set_style_text_font(el, &lv_myfont_30, 0);
    lv_obj_add_event_cb(exit_btn, pause_exit_click, LV_EVENT_CLICKED, NULL);
}

/* ========== 游戏结束、重新开始等（保持原有） ========== */
static void game_restart_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e); lv_obj_t *mbox = lv_obj_get_parent(btn); lv_obj_del(mbox);
    game_dialog = NULL;
    for (int i = 0; i < TOTAL_HOLES; i++) game_hole_reset(game_holes[i]);
    game_score = 0; game_time_left = 30; game_running = 1;
    lv_label_set_text_fmt(game_score_label, "Score: %d", game_score);
    lv_label_set_text_fmt(game_timer_label, "Time: %ds", game_time_left);
    if (game_timer) { lv_timer_del(game_timer); game_timer = NULL; }
    game_timer = lv_timer_create(game_timer_cb, 1000, NULL);
    if (game_spawn_timer) { lv_timer_del(game_spawn_timer); game_spawn_timer = NULL; }
    game_spawn_timer = lv_timer_create(game_spawn_timer_cb, get_spawn_interval(), NULL);
    if (game_start_btn) lv_obj_add_flag(game_start_btn, LV_OBJ_FLAG_HIDDEN);
    printf("Game restarted.\n");
}

static void game_exit_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e); lv_obj_t *mbox = lv_obj_get_parent(btn); lv_obj_del(mbox);
    game_dialog = NULL; game_running = 0;
    if (game_timer) { lv_timer_del(game_timer); game_timer = NULL; }
    if (game_spawn_timer) { lv_timer_del(game_spawn_timer); game_spawn_timer = NULL; }
    if (game_win) {
        lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
        if (gctrl_win) lv_obj_remove_flag(gctrl_win, LV_OBJ_FLAG_HIDDEN);
    }
    printf("Returned to game control.\n");
}

static void game_timer_cb(lv_timer_t *timer) {
    game_time_left--;
    lv_label_set_text_fmt(game_timer_label, "Time: %ds", game_time_left);
    if (game_time_left <= 0) {
        game_running = 0;
        if (game_spawn_timer) { lv_timer_del(game_spawn_timer); game_spawn_timer = NULL; }
        for (int i = 0; i < TOTAL_HOLES; i++) game_hole_reset(game_holes[i]);
        int is_new_record = 0;
        if (game_score > high_score) { high_score = game_score; is_new_record = 1; save_high_score(); }
        lv_obj_t *parent = lv_screen_active();
        lv_obj_t *mbox = lv_obj_create(parent); game_dialog = mbox;
        lv_obj_set_size(mbox, 420, 280); lv_obj_center(mbox);
        lv_obj_set_style_bg_color(mbox, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(mbox, 2, 0);
        lv_obj_set_style_border_color(mbox, lv_color_hex(0x000000), 0);
        lv_obj_set_style_radius(mbox, 10, 0);
        lv_obj_t *title = lv_label_create(mbox);
        lv_label_set_text(title, "Game Over");
        lv_obj_set_style_text_color(title, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
        char msg[128];
        if (is_new_record) sprintf(msg, "🎉 New Record!\nScore: %d", game_score);
        else sprintf(msg, "Score: %d\n%d more to beat the record!", game_score, high_score - game_score);
        lv_obj_t *content = lv_label_create(mbox);
        lv_label_set_text(content, msg);
        lv_obj_set_style_text_color(content, lv_color_hex(0x000000), 0);
        lv_obj_set_style_text_font(content, &lv_font_montserrat_18, 0);
        lv_obj_align(content, LV_ALIGN_CENTER, 0, -10);
        lv_obj_t *btn_restart = lv_btn_create(mbox);
        lv_obj_set_size(btn_restart, 100, 40); lv_obj_align(btn_restart, LV_ALIGN_BOTTOM_LEFT, 30, -20);
        lv_obj_t *restart_label = lv_label_create(btn_restart);
        lv_label_set_text(restart_label, "Restart"); lv_obj_center(restart_label);
        lv_obj_add_event_cb(btn_restart, game_restart_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t *btn_exit = lv_btn_create(mbox);
        lv_obj_set_size(btn_exit, 100, 40); lv_obj_align(btn_exit, LV_ALIGN_BOTTOM_RIGHT, -30, -20);
        lv_obj_t *exit_label = lv_label_create(btn_exit);
        lv_label_set_text(exit_label, "Exit"); lv_obj_center(exit_label);
        lv_obj_add_event_cb(btn_exit, game_exit_cb, LV_EVENT_CLICKED, NULL);
        if (game_timer) { lv_timer_del(game_timer); game_timer = NULL; }
    }
}

void game_start_cb(lv_event_t *e) {
    if (game_start_btn) lv_obj_add_flag(game_start_btn, LV_OBJ_FLAG_HIDDEN);
    game_score = 0; game_time_left = 30; game_running = 1;
    lv_label_set_text_fmt(game_score_label, "Score: %d", game_score);
    lv_label_set_text_fmt(game_timer_label, "Time: %ds", game_time_left);
    for (int i = 0; i < TOTAL_HOLES; i++) game_hole_reset(game_holes[i]);
    if (game_timer) lv_timer_del(game_timer);
    game_timer = lv_timer_create(game_timer_cb, 1000, NULL);
    if (game_spawn_timer) lv_timer_del(game_spawn_timer);
    game_spawn_timer = lv_timer_create(game_spawn_timer_cb, get_spawn_interval(), NULL);
    printf("Game started. Difficulty: %s\n", g_diff_names[g_diff]);
}

/* ========== 控制界面按钮 ========== */
static void gctrl_start_click(lv_event_t *e) {
    if (gctrl_win) lv_obj_add_flag(gctrl_win, LV_OBJ_FLAG_HIDDEN);
    if (!game_win) {
        /* 第一次创建游戏窗口 */
        game_btn_handler(NULL);
        return;
    }
    lv_obj_remove_flag(game_win, LV_OBJ_FLAG_HIDDEN);
    if (game_running == 0 && game_dialog == NULL) {
        if (game_start_btn) lv_obj_remove_flag(game_start_btn, LV_OBJ_FLAG_HIDDEN);
        game_score = 0; game_time_left = 30;
        lv_label_set_text_fmt(game_score_label, "Score: 0");
        lv_label_set_text_fmt(game_timer_label, "Time: 30s");
        for (int i = 0; i < TOTAL_HOLES; i++) game_hole_reset(game_holes[i]);
    }
    printf("Entering game.\n");
}

static void gctrl_diff_item_cb(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx <= 2) g_diff = idx;
    if (gctrl_diff_lb) lv_label_set_text_fmt(gctrl_diff_lb, "难度: %s", g_diff_names[g_diff]);
    /* 关闭选择窗口 */
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *parent = lv_obj_get_parent(target);
    while (parent && parent != lv_screen_active()) {
        lv_obj_t *p = lv_obj_get_parent(parent);
        if (p == lv_screen_active()) { lv_obj_del(parent); break; }
        parent = p;
    }
    printf("Difficulty set to: %s\n", g_diff_names[g_diff]);
}

static void gctrl_diff_bg_click(lv_event_t *e) {
    lv_obj_del(lv_event_get_target(e));
}

static void gctrl_diff_click(lv_event_t *e) {
    /* 半透明背景遮罩 */
    lv_obj_t *bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg, 1024, 600);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_50, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_add_event_cb(bg, gctrl_diff_bg_click, LV_EVENT_CLICKED, NULL);

    /* 选择框在遮罩上 */
    lv_obj_t *win = lv_obj_create(bg);
    lv_obj_set_size(win, 350, 300);
    lv_obj_center(win);
    lv_obj_set_style_bg_color(win, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(win, 2, 0);
    lv_obj_set_style_radius(win, 10, 0);

    lv_obj_t *ti = lv_label_create(win);
    lv_label_set_text(ti, "选择难度");
    lv_obj_set_style_text_font(ti, &lv_myfont_30, 0);
    lv_obj_align(ti, LV_ALIGN_TOP_MID, 0, 10);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(win);
        lv_obj_set_size(btn, 200, 50);
        lv_obj_align(btn, LV_ALIGN_CENTER, 0, -60 + i * 70);
        lv_obj_t *lb = lv_label_create(btn);
        lv_label_set_text(lb, g_diff_names[i]);
        lv_obj_center(lb);
        lv_obj_set_style_text_font(lb, &lv_myfont_30, 0);
        lv_obj_add_event_cb(btn, gctrl_diff_item_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    }
}

static void gctrl_back_click(lv_event_t *e) {
    if (gctrl_win) lv_obj_add_flag(gctrl_win, LV_OBJ_FLAG_HIDDEN);
    if (bw) lv_obj_remove_flag(bw, LV_OBJ_FLAG_HIDDEN);
    if (album_btn) lv_obj_remove_flag(album_btn, LV_OBJ_FLAG_HIDDEN);
    if (music_btn) lv_obj_remove_flag(music_btn, LV_OBJ_FLAG_HIDDEN);
    if (alenda_btn) lv_obj_remove_flag(alenda_btn, LV_OBJ_FLAG_HIDDEN);
    if (video_btn) lv_obj_remove_flag(video_btn, LV_OBJ_FLAG_HIDDEN);
    if (game_btn) lv_obj_remove_flag(game_btn, LV_OBJ_FLAG_HIDDEN);
    if (comm_btn) lv_obj_remove_flag(comm_btn, LV_OBJ_FLAG_HIDDEN);
    if (chat_btn) lv_obj_remove_flag(chat_btn, LV_OBJ_FLAG_HIDDEN);
}

/* ========== game_btn_handler：显示控制界面 ========== */
void game_btn_handler(lv_event_t *e) {
    /* 第一次创建游戏窗口（仍保留，但不显示） */
    if (!game_win) {
        game_win = lv_obj_create(lv_screen_active());
        if (!game_win) { printf("Error: game window creation failed\n"); return; }
        lv_obj_set_size(game_win, 1024, 600); lv_obj_set_pos(game_win, 0, 0);
        lv_obj_set_style_border_width(game_win, 0, 0); lv_obj_set_style_pad_all(game_win, 0, 0);
        lv_obj_set_style_margin_all(game_win, 0, 0); lv_obj_set_style_radius(game_win, 0, 0);
        lv_obj_set_style_bg_color(game_win, lv_color_hex(0x228B22), 0);
        lv_obj_add_flag(game_win, LV_OBJ_FLAG_HIDDEN);
        game_return_btn = lv_button_create(game_win);
        lv_obj_set_size(game_return_btn, 80, 80); lv_obj_align(game_return_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
        lv_obj_set_style_bg_opa(game_return_btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(game_return_btn, 0, 0);
        lv_obj_set_style_outline_width(game_return_btn, 0, 0);
        lv_obj_set_style_shadow_width(game_return_btn, 0, 0);
        lv_obj_add_event_cb(game_return_btn, game_return_click, LV_EVENT_PRESSED, NULL);
        game_score_label = lv_label_create(game_win);
        lv_obj_align(game_score_label, LV_ALIGN_TOP_LEFT, 20, 20);
        lv_label_set_text(game_score_label, "Score: 0");
        lv_obj_set_style_text_color(game_score_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(game_score_label, &lv_font_montserrat_30, 0);
        game_timer_label = lv_label_create(game_win);
        lv_obj_align(game_timer_label, LV_ALIGN_TOP_RIGHT, -120, 20);
        lv_label_set_text(game_timer_label, "Time: 30s");
        lv_obj_set_style_text_color(game_timer_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(game_timer_label, &lv_font_montserrat_30, 0);
        int hole_size = 120, spacing = 15;
        int total_width = GRID_SIZE * hole_size + (GRID_SIZE - 1) * spacing;
        int total_height = GRID_SIZE * hole_size + (GRID_SIZE - 1) * spacing;
        int start_x = (1024 - total_width) / 2, start_y = (600 - total_height) / 2 + 30;
        for (int row = 0; row < GRID_SIZE; row++) {
            for (int col = 0; col < GRID_SIZE; col++) {
                int idx = row * GRID_SIZE + col;
                lv_obj_t *hole = lv_btn_create(game_win);
                lv_obj_set_size(hole, hole_size, hole_size);
                lv_obj_set_pos(hole, start_x + col * (hole_size + spacing), start_y + row * (hole_size + spacing));
                lv_obj_set_style_bg_color(hole, lv_color_hex(0x8B4513), 0);
                lv_obj_set_style_radius(hole, LV_RADIUS_CIRCLE, 0);
                lv_obj_set_user_data(hole, (void*)0);
                lv_obj_t *label = lv_label_create(hole);
                lv_label_set_text(label, ""); lv_obj_center(label);
                lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
                lv_obj_set_style_text_font(label, &lv_font_montserrat_40, 0);
                lv_obj_add_event_cb(hole, game_hole_click, LV_EVENT_PRESSED, NULL);
                lv_obj_t *hm = lv_image_create(hole);
                lv_image_set_src(hm, &hammer_img); lv_obj_set_size(hm, 120, 120);
                lv_obj_center(hm); lv_obj_add_flag(hm, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_user_data(hm, (void*)1);
                lv_obj_t *st = lv_image_create(hole);
                lv_image_set_src(st, &star_img); lv_obj_set_size(st, 120, 120);
                lv_obj_center(st); lv_obj_add_flag(st, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_user_data(st, (void*)2);
                game_holes[idx] = hole;
            }
        }
        game_start_btn = lv_btn_create(game_win);
        lv_obj_set_size(game_start_btn, 200, 80); lv_obj_align(game_start_btn, LV_ALIGN_CENTER, 0, 50);
        lv_obj_t *start_label = lv_label_create(game_start_btn);
        lv_label_set_text(start_label, "Start"); lv_obj_center(start_label);
        lv_obj_add_event_cb(game_start_btn, game_start_cb, LV_EVENT_CLICKED, NULL);
        game_score = 0; game_time_left = 30; game_running = 0;
        printf("Game window created.\n");
    }

    /* 隐藏主菜单 */
    if (album_btn) lv_obj_add_flag(album_btn, LV_OBJ_FLAG_HIDDEN);
    if (music_btn) lv_obj_add_flag(music_btn, LV_OBJ_FLAG_HIDDEN);
    if (alenda_btn) lv_obj_add_flag(alenda_btn, LV_OBJ_FLAG_HIDDEN);
    if (video_btn) lv_obj_add_flag(video_btn, LV_OBJ_FLAG_HIDDEN);
    if (game_btn) lv_obj_add_flag(game_btn, LV_OBJ_FLAG_HIDDEN);
    if (bw) lv_obj_add_flag(bw, LV_OBJ_FLAG_HIDDEN);
    if (cw) lv_obj_add_flag(cw, LV_OBJ_FLAG_HIDDEN);
    if (dw) lv_obj_add_flag(dw, LV_OBJ_FLAG_HIDDEN);
    if (g_gif) lv_obj_add_flag(g_gif, LV_OBJ_FLAG_HIDDEN);
    if (pre_btn) lv_obj_add_flag(pre_btn, LV_OBJ_FLAG_HIDDEN);
    if (next_btn) lv_obj_add_flag(next_btn, LV_OBJ_FLAG_HIDDEN);
    if (return_btn) lv_obj_add_flag(return_btn, LV_OBJ_FLAG_HIDDEN);
    if (comm_btn) lv_obj_add_flag(comm_btn, LV_OBJ_FLAG_HIDDEN);
    if (chat_btn) lv_obj_add_flag(chat_btn, LV_OBJ_FLAG_HIDDEN);

    /* 显示控制界面 */
    if (!gctrl_win) {
        gctrl_win = lv_obj_create(lv_screen_active());
        if (!gctrl_win) return;
        lv_obj_set_size(gctrl_win, 1024, 600);
        lv_obj_set_pos(gctrl_win, 0, 0);
        lv_obj_set_style_bg_color(gctrl_win, lv_color_hex(0x2B2B2B), 0);
        lv_obj_set_style_radius(gctrl_win, 0, 0);
        lv_obj_set_style_border_width(gctrl_win, 0, 0);
        lv_obj_add_flag(gctrl_win, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *ti = lv_label_create(gctrl_win); lv_label_set_text(ti, "GAME");
        lv_obj_set_style_text_font(ti, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(ti, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(ti, LV_ALIGN_TOP_MID, 0, 20);

        /* 难度显示 */
        gctrl_diff_lb = lv_label_create(gctrl_win);
        lv_label_set_text_fmt(gctrl_diff_lb, "难度: %s", g_diff_names[g_diff]);
        lv_obj_set_style_text_font(gctrl_diff_lb, &lv_myfont_30, 0);
        lv_obj_set_style_text_color(gctrl_diff_lb, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(gctrl_diff_lb, LV_ALIGN_TOP_MID, 0, 70);

        /* 开始按钮 */
        lv_obj_t *start = lv_button_create(gctrl_win);
        lv_obj_set_size(start, 200, 80);
        lv_obj_align(start, LV_ALIGN_CENTER, 0, -60);
        lv_obj_t *sl = lv_label_create(start); lv_label_set_text(sl, "Start"); lv_obj_center(sl);
        lv_obj_add_event_cb(start, gctrl_start_click, LV_EVENT_PRESSED, NULL);

        /* 难度按钮 */
        lv_obj_t *diff = lv_button_create(gctrl_win);
        lv_obj_set_size(diff, 200, 80);
        lv_obj_align(diff, LV_ALIGN_CENTER, 0, 50);
        lv_obj_t *dl = lv_label_create(diff); lv_label_set_text(dl, "Difficulty"); lv_obj_center(dl);
        lv_obj_add_event_cb(diff, gctrl_diff_click, LV_EVENT_PRESSED, NULL);

        /* 返回按钮 */
        lv_obj_t *back = lv_button_create(gctrl_win);
        lv_obj_set_size(back, 120, 60);
        lv_obj_align(back, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
        lv_obj_t *bl = lv_label_create(back); lv_label_set_text(bl, "Back"); lv_obj_center(bl);
        lv_obj_add_event_cb(back, gctrl_back_click, LV_EVENT_PRESSED, NULL);
    }
    lv_obj_remove_flag(gctrl_win, LV_OBJ_FLAG_HIDDEN);
}