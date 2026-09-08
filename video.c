#include "globals.h"

lv_obj_t *video_win = NULL;
lv_obj_t *video_play_btn = NULL, *video_prev_btn = NULL, *video_next_btn = NULL, *video_return_btn = NULL;
lv_obj_t *video_time_label = NULL;
pid_t g_video_pid = -1;
int g_video_playing = 0;
lv_timer_t *video_timer = NULL;
int g_video_seconds = 0;
int g_video_index = 0;
char *g_video_path = NULL;
lv_disp_t *g_saved_disp = NULL;

/* 视频控制界面 */
lv_obj_t *vctrl_win = NULL;

 void set_video_controls_visible(bool visible) {
    if (!video_play_btn || !video_prev_btn || !video_next_btn || !video_return_btn) return;
    lv_opa_t opa = visible ? LV_OPA_COVER : LV_OPA_TRANSP;
    lv_obj_set_style_opa(video_play_btn, opa, 0);
    lv_obj_set_style_opa(video_prev_btn, opa, 0);
    lv_obj_set_style_opa(video_next_btn, opa, 0);
    lv_obj_set_style_opa(video_return_btn, opa, 0);
}

 void start_video(void) {
    if (g_video_pid > 0 || !g_video_path) return;
    g_saved_disp = lv_disp_get_default();
    if (g_saved_disp) lv_disp_set_default(NULL);
    pid_t pid = fork();
    if (pid == 0) {
        execlp("mplayer64", "mplayer64", "-vo", "fbdev2", "-zoom", "-x", "1024", "-y", "600", "-quiet", g_video_path, NULL);
        exit(0);
    } else if (pid > 0) {
        g_video_pid = pid;
        g_video_playing = 1;
        set_video_controls_visible(false);
    }
}

 void stop_video(void) {
    if (g_video_pid > 0) { kill(g_video_pid, SIGTERM); waitpid(g_video_pid, NULL, 0); g_video_pid = -1; }
    g_video_playing = 0;
    set_video_controls_visible(true);
    if (video_timer) lv_timer_pause(video_timer);
    if (g_saved_disp) {
        lv_disp_set_default(g_saved_disp);
        g_saved_disp = NULL;
        lv_timer_handler();
    }
}

 void video_timer_cb(lv_timer_t *timer) {
    if (!g_video_playing) return;
    g_video_seconds++;
    int min = g_video_seconds / 60, sec = g_video_seconds % 60;
    lv_label_set_text_fmt(video_time_label, "%02d:%02d", min, sec);
}

 void play_current_video(void) {
    if (!g_video_path) {
        if (video_list && video_list->num > 0) {
            g_video_index = 0;
            g_video_path = get_video_path_by_index(0);
        } else { printf("No video files!\n"); return; }
    }
    start_video();
    lv_obj_t *label = lv_obj_get_child(video_play_btn, 0);
    if (label) lv_label_set_text(label, "Pause");
    g_video_seconds = 0;
    lv_label_set_text_fmt(video_time_label, "00:00");
    if (!video_timer) video_timer = lv_timer_create(video_timer_cb, 1000, NULL);
    else lv_timer_resume(video_timer);
}

void video_play_click(lv_event_t *e) {
    if (!video_play_btn || !video_win) { printf("Video not ready\n"); return; }
    if (lv_obj_has_flag(video_win, LV_OBJ_FLAG_HIDDEN)) { printf("Video hidden\n"); return; }
    lv_obj_t *label = lv_obj_get_child(video_play_btn, 0);
    if (g_video_playing) {
        stop_video();
        if (label) lv_label_set_text(label, "Play");
    } else {
        if (!g_video_path) {
            if (video_list && video_list->num > 0) {
                g_video_index = 0;
                g_video_path = get_video_path_by_index(0);
            } else { printf("No video files.\n"); return; }
        }
        play_current_video();
    }
}

void video_prev_click(lv_event_t *e) {
    if (!video_list || video_list->num <= 1) { printf("No or only one video.\n"); return; }
    stop_video();
    if (video_timer) { lv_timer_del(video_timer); video_timer = NULL; }
    g_video_index = (g_video_index - 1 + video_list->num) % video_list->num;
    g_video_path = get_video_path_by_index(g_video_index);
    play_current_video();
}

void video_next_click(lv_event_t *e) {
    if (!video_list || video_list->num <= 1) { printf("No or only one video.\n"); return; }
    stop_video();
    if (video_timer) { lv_timer_del(video_timer); video_timer = NULL; }
    g_video_index = (g_video_index + 1) % video_list->num;
    g_video_path = get_video_path_by_index(g_video_index);
    play_current_video();
}

/* ========== 通用：输入路径扫描添加 ========== */
static lv_obj_t *add_scan_ta = NULL;
static lv_obj_t *add_scan_kb = NULL;
static int add_scan_type = 0;

static void add_scan_ready_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_READY) return;
    if (!add_scan_ta) return;
    const char *path = lv_textarea_get_text(add_scan_ta);
    if (strlen(path) == 0) return;
    if (add_scan_type == 1) {
        getlist((char*)path, b1);
        printf("扫描音乐: %s, 共 %d 个\n", path, b1->num);
    } else {
        get_video_list((char*)path, video_list);
        printf("扫描视频: %s, 共 %d 个\n", path, video_list->num);
    }
    if (add_scan_kb) lv_obj_add_flag(add_scan_kb, LV_OBJ_FLAG_HIDDEN);
    if (add_scan_ta) lv_obj_add_flag(add_scan_ta, LV_OBJ_FLAG_HIDDEN);
}

static void add_scan_bg_click(lv_event_t *e) {
    lv_obj_t *bg = lv_event_get_target(e);
    /* 直接删 bg，子对象自动被删，不用手动隐藏 */
    add_scan_ta = NULL;
    add_scan_kb = NULL;
    lv_obj_del(bg);
}

void add_scan_show(int type) {
    add_scan_type = type;
    const char *hint = (type == 1) ? "输入音乐目录路径..." : "输入视频目录路径...";

    /* 背景遮罩（点击关闭） */
    lv_obj_t *bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bg, 1024, 600);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_50, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_add_event_cb(bg, add_scan_bg_click, LV_EVENT_CLICKED, NULL);

    if (!add_scan_ta) {
        add_scan_ta = lv_textarea_create(bg);   /* 创建在 bg 上 */
        lv_obj_set_size(add_scan_ta, 550, 60);
        lv_obj_align(add_scan_ta, LV_ALIGN_CENTER, 0, -80);
        lv_obj_set_style_bg_color(add_scan_ta, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(add_scan_ta, &lv_myfont_30, 0);
    }
    if (!add_scan_kb) {
        add_scan_kb = lv_keyboard_create(bg);    /* 创建在 bg 上 */
        lv_obj_set_size(add_scan_kb, 800, 200);
        lv_obj_align(add_scan_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_event_cb(add_scan_ta, add_scan_ready_cb, LV_EVENT_ALL, NULL);
    }
    lv_textarea_set_text(add_scan_ta, "");
    lv_textarea_set_placeholder_text(add_scan_ta, hint);
    lv_obj_remove_flag(add_scan_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(add_scan_kb, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(add_scan_kb, add_scan_ta);
}

 void video_add_click(lv_event_t *e) { add_scan_show(2); }

 void mplayer_cmd(const char *cmd) {
    int fd = open("/tmp/mp_fifo", O_WRONLY | O_NONBLOCK);
    if (fd < 0) return;
    write(fd, cmd, strlen(cmd));
    close(fd);
}

 void vol_up_cb(lv_event_t *e)    { mplayer_cmd("volume +10 1\n"); }
 void vol_down_cb(lv_event_t *e)  { mplayer_cmd("volume -10 1\n"); }

void video_return_click(lv_event_t *e) {
    stop_video();
    if (video_timer) { lv_timer_del(video_timer); video_timer = NULL; }
    if (video_win) lv_obj_add_flag(video_win, LV_OBJ_FLAG_HIDDEN);
    /* 返回控制界面 */
    if (vctrl_win) lv_obj_remove_flag(vctrl_win, LV_OBJ_FLAG_HIDDEN);
}

/* ========== 播放界面（无 Add/音量） ========== */
 void create_video_window(void) {
    if (video_win) return;
    video_win = lv_obj_create(lv_screen_active());
    if (!video_win) { printf("Error: video window creation failed\n"); return; }
    lv_obj_set_size(video_win, 1024, 600);
    lv_obj_set_pos(video_win, 0, 0);
    lv_obj_set_style_border_width(video_win, 0, 0);
    lv_obj_set_style_outline_width(video_win, 0, 0);
    lv_obj_set_style_pad_all(video_win, 0, 0);
    lv_obj_set_style_margin_all(video_win, 0, 0);
    lv_obj_set_style_radius(video_win, 0, 0);
    lv_obj_set_style_shadow_width(video_win, 0, 0);
    lv_obj_set_style_bg_color(video_win, lv_color_hex(0x000000), 0);
    lv_obj_add_flag(video_win, LV_OBJ_FLAG_HIDDEN);

    video_play_btn = lv_button_create(video_win);
    if (video_play_btn) { lv_obj_set_size(video_play_btn, 120, 80); lv_obj_align(video_play_btn, LV_ALIGN_CENTER, 0, 0);
        lv_obj_t *play_label = lv_label_create(video_play_btn);
        if (play_label) { lv_label_set_text(play_label, "Play"); lv_obj_center(play_label); }
        lv_obj_add_event_cb(video_play_btn, video_play_click, LV_EVENT_PRESSED, NULL); }

    video_prev_btn = lv_button_create(video_win);
    if (video_prev_btn) { lv_obj_set_size(video_prev_btn, 80, 60); lv_obj_align_to(video_prev_btn, video_play_btn, LV_ALIGN_OUT_LEFT_MID, -20, 0);
        lv_obj_t *prev_label = lv_label_create(video_prev_btn);
        if (prev_label) { lv_label_set_text(prev_label, "Prev"); lv_obj_center(prev_label); }
        lv_obj_add_event_cb(video_prev_btn, video_prev_click, LV_EVENT_PRESSED, NULL); }

    video_next_btn = lv_button_create(video_win);
    if (video_next_btn) { lv_obj_set_size(video_next_btn, 80, 60); lv_obj_align_to(video_next_btn, video_play_btn, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
        lv_obj_t *next_label = lv_label_create(video_next_btn);
        if (next_label) { lv_label_set_text(next_label, "Next"); lv_obj_center(next_label); }
        lv_obj_add_event_cb(video_next_btn, video_next_click, LV_EVENT_PRESSED, NULL); }

    video_return_btn = lv_button_create(video_win);
    if (video_return_btn) { lv_obj_set_size(video_return_btn, 120, 60); lv_obj_align(video_return_btn, LV_ALIGN_TOP_RIGHT, -20, 20);
        lv_obj_t *ret_label = lv_label_create(video_return_btn);
        if (ret_label) { lv_label_set_text(ret_label, "return"); lv_obj_center(ret_label); }
        lv_obj_add_event_cb(video_return_btn, video_return_click, LV_EVENT_PRESSED, NULL); }

    video_time_label = lv_label_create(video_win);
    if (video_time_label) { lv_obj_align(video_time_label, LV_ALIGN_CENTER, 0, -80);
        lv_label_set_text(video_time_label, "00:00");
        lv_obj_set_style_text_color(video_time_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(video_time_label, &lv_font_montserrat_30, 0); }
}

/* ========== 视频删除功能 ========== */
static lv_obj_t *vdel_win = NULL;
static lv_obj_t *vdel_list = NULL;
static lv_obj_t *vdel_sel = NULL;

static void vdel_exec(void) {
    if (!vdel_sel || !video_list) return;
    Node *p = (Node *)lv_obj_get_user_data(vdel_sel);
    if (!p) return;
    if (p->next == p) video_list->first = video_list->last = NULL;
    else {
        p->pre->next = p->next; p->next->pre = p->pre;
        if (video_list->first == p) video_list->first = p->next;
        if (video_list->last == p) video_list->last = p->pre;
    }
    video_list->num--;
    free(p->data); free(p);
    if (vdel_win) { lv_obj_del(vdel_win); vdel_win = NULL; vdel_list = NULL; vdel_sel = NULL; }
}

static void vdel_btn_cb(lv_event_t *e) {
    if (!vdel_sel) {
        lv_obj_t *mbox = lv_obj_create(lv_screen_active());
        lv_obj_set_size(mbox, 300, 150); lv_obj_center(mbox);
        lv_obj_set_style_bg_color(mbox, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(mbox, 2, 0); lv_obj_set_style_radius(mbox, 10, 0);
        lv_obj_t *lb = lv_label_create(mbox); lv_label_set_text(lb, "请先选择文件"); lv_obj_center(lb);
        lv_obj_set_style_text_font(lb, &lv_myfont_30, 0);
        return;
    }
    vdel_exec();
}

static void vdel_item_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    if (vdel_sel) lv_obj_set_style_bg_color(vdel_sel, lv_color_hex(0xF0F0F0), 0);
    vdel_sel = btn; lv_obj_set_style_bg_color(btn, lv_color_hex(0x87CEEB), 0);
}

static void vdel_close_cb(lv_event_t *e) {
    if (vdel_win) { lv_obj_del(vdel_win); vdel_win = NULL; vdel_list = NULL; vdel_sel = NULL; }
}

static void vctrl_del_click(lv_event_t *e) {
    if (vdel_win) return;
    if (!video_list || video_list->num == 0) { printf("No video files.\n"); return; }
    vdel_win = lv_obj_create(lv_screen_active());
    lv_obj_set_size(vdel_win, 500, 500); lv_obj_center(vdel_win);
    lv_obj_set_style_bg_color(vdel_win, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(vdel_win, 2, 0); lv_obj_set_style_radius(vdel_win, 10, 0);
    lv_obj_t *title = lv_label_create(vdel_win); lv_label_set_text(title, "删除视频文件");
    lv_obj_set_style_text_font(title, &lv_myfont_30, 0); lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    vdel_list = lv_list_create(vdel_win);
    lv_obj_set_size(vdel_list, 440, 340); lv_obj_align(vdel_list, LV_ALIGN_TOP_MID, 0, 50);
    Node *p = video_list->first;
    if (p) {
        for (int i = 0; i < video_list->num; i++) {
            char *name = strrchr(p->data, '/'); if (!name) name = p->data; else name++;
            lv_obj_t *btn = lv_list_add_btn(vdel_list, NULL, name);
            lv_obj_set_style_text_font(lv_obj_get_child(btn, 0), &lv_myfont_30, 0);
            lv_obj_set_user_data(btn, p); lv_obj_add_event_cb(btn, vdel_item_cb, LV_EVENT_CLICKED, NULL);
            p = p->next;
        }
    }
    lv_obj_t *del_btn = lv_button_create(vdel_win); lv_obj_set_size(del_btn, 120, 50);
    lv_obj_align(del_btn, LV_ALIGN_BOTTOM_LEFT, 40, -20);
    lv_obj_t *dl = lv_label_create(del_btn); lv_label_set_text(dl, "删除"); lv_obj_center(dl);
    lv_obj_set_style_text_font(dl, &lv_myfont_30, 0);
    lv_obj_add_event_cb(del_btn, vdel_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_btn = lv_button_create(vdel_win); lv_obj_set_size(close_btn, 120, 50);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_RIGHT, -40, -20);
    lv_obj_t *cl = lv_label_create(close_btn); lv_label_set_text(cl, "关闭"); lv_obj_center(cl);
    lv_obj_set_style_text_font(cl, &lv_myfont_30, 0);
    lv_obj_add_event_cb(close_btn, vdel_close_cb, LV_EVENT_CLICKED, NULL);
}

/* ========== 选择播放 ========== */
/* 选择播放：点击列表项 */
static void vctrl_select_item_cb(lv_event_t *e) {
    char *path = (char *)lv_event_get_user_data(e);
    if (!path) return;
    g_video_path = path;
    g_video_index = 0;
    /* 关选择窗口 */
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *parent = lv_obj_get_parent(target);
    while (parent && parent != lv_screen_active()) {
        lv_obj_t *p = lv_obj_get_parent(parent);
        if (p == lv_screen_active()) { lv_obj_del(parent); break; }
        parent = p;
    }
    /* 隐藏控制界面，显示播放界面 */
    if (vctrl_win) lv_obj_add_flag(vctrl_win, LV_OBJ_FLAG_HIDDEN);
    if (!video_win) create_video_window();
    if (video_win) {
        lv_obj_remove_flag(video_win, LV_OBJ_FLAG_HIDDEN);
        stop_video(); g_video_seconds = 0;
        if (video_time_label) lv_label_set_text_fmt(video_time_label, "00:00");
        if (video_play_btn) { lv_obj_t *lb = lv_obj_get_child(video_play_btn, 0); if (lb) lv_label_set_text(lb, "Play"); }
    }
}

/* 选择播放窗口关闭按钮 */
static void vctrl_select_close_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *parent = lv_obj_get_parent(target);
    while (parent && parent != lv_screen_active()) {
        lv_obj_t *p = lv_obj_get_parent(parent);
        if (p == lv_screen_active()) { lv_obj_del(parent); break; }
        parent = p;
    }
}

/* 打开选择播放窗口 */
static void vctrl_select_play(lv_event_t *e) {
    if (!video_list || video_list->num == 0) { printf("No video files.\n"); return; }
    lv_obj_t *win = lv_obj_create(lv_screen_active());
    lv_obj_set_size(win, 500, 500); lv_obj_center(win);
    lv_obj_set_style_bg_color(win, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(win, 2, 0); lv_obj_set_style_radius(win, 10, 0);
    lv_obj_t *ti = lv_label_create(win); lv_label_set_text(ti, "选择视频");
    lv_obj_set_style_text_font(ti, &lv_myfont_30, 0);
    lv_obj_align(ti, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_t *list = lv_list_create(win); lv_obj_set_size(list, 440, 380);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
    Node *p = video_list->first;
    if (p) {
        for (int i = 0; i < video_list->num; i++) {
            char *name = strrchr(p->data, '/'); if (!name) name = p->data; else name++;
            lv_obj_t *btn = lv_list_add_btn(list, NULL, name);
            lv_obj_set_style_text_font(lv_obj_get_child(btn, 0), &lv_myfont_30, 0);
            lv_obj_add_event_cb(btn, vctrl_select_item_cb, LV_EVENT_CLICKED, p->data);
            p = p->next;
        }
    }
    lv_obj_t *cb = lv_button_create(win); lv_obj_set_size(cb, 120, 50);
    lv_obj_align(cb, LV_ALIGN_BOTTOM_RIGHT, -40, -20);
    lv_obj_t *cl = lv_label_create(cb); lv_label_set_text(cl, "关闭"); lv_obj_center(cl);
    lv_obj_set_style_text_font(cl, &lv_myfont_30, 0);
    lv_obj_add_event_cb(cb, vctrl_select_close_cb, LV_EVENT_CLICKED, NULL);
}

/* ========== 控制界面 ========== */
void vctrl_play_click(lv_event_t *e) {
    if (!video_list || video_list->num == 0) { printf("No video files.\n"); return; }
    if (!g_video_path) { g_video_index = 0; g_video_path = get_video_path_by_index(0); }
    if (vctrl_win) lv_obj_add_flag(vctrl_win, LV_OBJ_FLAG_HIDDEN);
    if (!video_win) create_video_window();
    if (video_win) {
        lv_obj_remove_flag(video_win, LV_OBJ_FLAG_HIDDEN);
        stop_video(); g_video_seconds = 0;
        if (video_time_label) lv_label_set_text_fmt(video_time_label, "00:00");
        if (video_play_btn) { lv_obj_t *lb = lv_obj_get_child(video_play_btn, 0); if (lb) lv_label_set_text(lb, "Play"); }
    }
}

static void vctrl_back_click(lv_event_t *e) {
    if (vctrl_win) lv_obj_add_flag(vctrl_win, LV_OBJ_FLAG_HIDDEN);
    if (bw) lv_obj_remove_flag(bw, LV_OBJ_FLAG_HIDDEN);
    /* 恢复所有主菜单按钮 */
    if (album_btn) lv_obj_remove_flag(album_btn, LV_OBJ_FLAG_HIDDEN);
    if (music_btn) lv_obj_remove_flag(music_btn, LV_OBJ_FLAG_HIDDEN);
    if (alenda_btn) lv_obj_remove_flag(alenda_btn, LV_OBJ_FLAG_HIDDEN);
    if (video_btn) lv_obj_remove_flag(video_btn, LV_OBJ_FLAG_HIDDEN);
    if (game_btn) lv_obj_remove_flag(game_btn, LV_OBJ_FLAG_HIDDEN);
    if (comm_btn) lv_obj_remove_flag(comm_btn, LV_OBJ_FLAG_HIDDEN);
    if (chat_btn) lv_obj_remove_flag(chat_btn, LV_OBJ_FLAG_HIDDEN);
}

/* ========== video_btn_handler：打开控制界面 ========== */
void video_btn_handler(lv_event_t *e) {
    if (video_list->num == 0) { printf("No video files.\n"); return; }
    if (!g_video_path) { g_video_index = 0; g_video_path = get_video_path_by_index(0); }
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
    if (game_btn) lv_obj_add_flag(game_btn, LV_OBJ_FLAG_HIDDEN);
    if (comm_btn) lv_obj_add_flag(comm_btn, LV_OBJ_FLAG_HIDDEN);
    if (chat_btn) lv_obj_add_flag(chat_btn, LV_OBJ_FLAG_HIDDEN);
    if (bw) lv_obj_add_flag(bw, LV_OBJ_FLAG_HIDDEN);

    if (!vctrl_win) {
        vctrl_win = lv_obj_create(lv_screen_active());
        if (!vctrl_win) return;
        lv_obj_set_size(vctrl_win, 1024, 600);
        lv_obj_set_pos(vctrl_win, 0, 0);
        lv_obj_set_style_bg_color(vctrl_win, lv_color_hex(0x2B2B2B), 0);
        lv_obj_set_style_radius(vctrl_win, 0, 0);
        lv_obj_set_style_border_width(vctrl_win, 0, 0);
        lv_obj_add_flag(vctrl_win, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *ti = lv_label_create(vctrl_win); lv_label_set_text(ti, "VIDEO");
        lv_obj_set_style_text_font(ti, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(ti, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(ti, LV_ALIGN_TOP_MID, 0, 20);

        /* Add 按钮（左上） */
        lv_obj_t *add = lv_button_create(vctrl_win); lv_obj_set_size(add, 80, 50);
        lv_obj_set_pos(add, 10, 10);
        lv_obj_t *al = lv_label_create(add); lv_label_set_text(al, "Add"); lv_obj_center(al);
        lv_obj_add_event_cb(add, video_add_click, LV_EVENT_PRESSED, NULL);

        /* Del 按钮（Add 右边） */
        lv_obj_t *del = lv_button_create(vctrl_win); lv_obj_set_size(del, 80, 50);
        lv_obj_set_pos(del, 100, 10);
        lv_obj_t *dl = lv_label_create(del); lv_label_set_text(dl, "Del"); lv_obj_center(dl);
        lv_obj_add_event_cb(del, vctrl_del_click, LV_EVENT_PRESSED, NULL);

        /* Play 按钮（居中偏左） */
        lv_obj_t *play = lv_button_create(vctrl_win); lv_obj_set_size(play, 160, 80);
        lv_obj_align(play, LV_ALIGN_CENTER, -120, 0);
        lv_obj_t *pl = lv_label_create(play); lv_label_set_text(pl, "Play"); lv_obj_center(pl);
        lv_obj_add_event_cb(play, vctrl_play_click, LV_EVENT_PRESSED, NULL);

        /* Select 按钮（居中偏右） */
        lv_obj_t *sel = lv_button_create(vctrl_win); lv_obj_set_size(sel, 160, 80);
        lv_obj_align(sel, LV_ALIGN_CENTER, 120, 0);
        lv_obj_t *sl = lv_label_create(sel); lv_label_set_text(sl, "Select"); lv_obj_center(sl);
        lv_obj_add_event_cb(sel, vctrl_select_play, LV_EVENT_PRESSED, NULL);

        /* V+ / V-（右上） */
        lv_obj_t *vu = lv_button_create(vctrl_win); lv_obj_set_size(vu, 80, 60);
        lv_obj_align(vu, LV_ALIGN_TOP_RIGHT, -10, 20);
        lv_obj_t *vul = lv_label_create(vu); lv_label_set_text(vul, "V+"); lv_obj_center(vul);
        lv_obj_add_event_cb(vu, vol_up_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_t *vd = lv_button_create(vctrl_win); lv_obj_set_size(vd, 80, 60);
        lv_obj_align_to(vd, vu, LV_ALIGN_OUT_LEFT_MID, -10, 0);
        lv_obj_t *vdl = lv_label_create(vd); lv_label_set_text(vdl, "V-"); lv_obj_center(vdl);
        lv_obj_add_event_cb(vd, vol_down_cb, LV_EVENT_PRESSED, NULL);

        /* Back（右下） */
        lv_obj_t *bk = lv_button_create(vctrl_win); lv_obj_set_size(bk, 120, 60);
        lv_obj_align(bk, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
        lv_obj_t *bl = lv_label_create(bk); lv_label_set_text(bl, "Back"); lv_obj_center(bl);
        lv_obj_add_event_cb(bk, vctrl_back_click, LV_EVENT_PRESSED, NULL);
    }
    lv_obj_remove_flag(vctrl_win, LV_OBJ_FLAG_HIDDEN);
}

void hide_video_window(void) {
    if (video_win) { lv_obj_add_flag(video_win, LV_OBJ_FLAG_HIDDEN); stop_video();
        if (video_timer) { lv_timer_del(video_timer); video_timer = NULL; } }
}