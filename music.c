#include "globals.h"

/* ========== 全局变量 ========== */
int i = 1;
Hnode *b1 = NULL;
Hnode *video_list = NULL;
lv_obj_t *music_play_btn = NULL;

int g_song_index = 0;
pid_t g_play_pid = -1;
int g_is_playing = 0;
int g_gif_index = 0;
lv_obj_t *g_gif = NULL;
/* ========== 音乐删除 ========== */
static lv_obj_t *del_win = NULL;
static lv_obj_t *del_list = NULL;
static lv_obj_t *del_selected = NULL;

static const char *g_pic_list[] = {
    "A:/lwx/bmp/1.bmp", "A:/lwx/bmp/2.bmp", "A:/lwx/bmp/3.bmp",
    "A:/lwx/bmp/4.bmp", "A:/lwx/bmp/7.bmp", "A:/lwx/bmp/9.bmp",
    "A:/lwx/bmp/8.bmp"
};
#define PIC_NUM (sizeof(g_pic_list)/sizeof(g_pic_list[0]))

int get_song_count(void) { return b1 ? b1->num : 0; }

char* get_song_path(int index) {
    if (!b1 || !b1->first || index < 0 || index >= b1->num) return NULL;
    Node *p = b1->first;
    for (int i = 0; i < index; i++) p = p->next;
    return p->data;
}

static void close_tip_cb(lv_timer_t *timer) {
    lv_obj_t *mbox = lv_timer_get_user_data(timer);
    if (mbox) lv_obj_del(mbox);
}

void chuanjianlist(char *buf, Hnode *list) {
    if (list->first) {
        Node *p = list->first;
        for (int i = 0; i < list->num; i++) {
            if (strcmp(p->data, buf) == 0) {
                free(buf);  /* 重复，释放内存 */
                return;
            }
            p = p->next;
        }
    }
    Node *pnew = malloc(sizeof(Node));
    pnew->data = buf;
    pnew->next = pnew->pre = NULL;
    if (list->num == 0) {
        list->first = list->last = pnew;
        pnew->pre = pnew->next = pnew;
    } else {
        list->last->next = pnew;
        pnew->pre = list->last;
        list->last = pnew;
        list->last->next = list->first;
        list->first->pre = list->last;
    }
    list->num++;
}

 int is_video_file(const char *name) {
    const char *ext[] = {".mp4", ".avi", ".mkv", ".mov", ".flv", ".wmv"};
    int len = strlen(name);
    for (int i = 0; i < 6; i++) {
        int ext_len = strlen(ext[i]);
        if (len > ext_len && strcmp(name + len - ext_len, ext[i]) == 0) return 1;
    }
    return 0;
}

char* get_video_path_by_index(int index) {
    if (!video_list || !video_list->first || index < 0 || index >= video_list->num) return NULL;
    Node *p = video_list->first;
    for (int i = 0; i < index; i++) p = p->next;
    return p->data;
}

void getlist(char *path, Hnode *list) {
    DIR *pf = opendir(path);
    if (!pf) { perror("opendir"); return; }
    while (1) {
        struct dirent *dt = readdir(pf);
        if (!dt) break;
        char buf[1024];
        sprintf(buf, "%s/%s", path, dt->d_name);
        struct stat sb;
        if (stat(buf, &sb) == -1) continue;
        if (S_ISDIR(sb.st_mode)) {
            if (strcmp(dt->d_name, ".") == 0 || strcmp(dt->d_name, "..") == 0) continue;
            getlist(buf, list);
        } else {
            int len = strlen(dt->d_name);
            if (len > 4 && strcmp(dt->d_name + len - 4, ".mp3") == 0)
                chuanjianlist(strdup(buf), list);
        }
    }
    closedir(pf);
}

void get_video_list(char *path, Hnode *list) {
    DIR *pf = opendir(path);
    if (!pf) { perror("opendir"); return; }
    while (1) {
        struct dirent *dt = readdir(pf);
        if (!dt) break;
        char buf[1024];
        sprintf(buf, "%s/%s", path, dt->d_name);
        struct stat sb;
        if (stat(buf, &sb) == -1) continue;
        if (S_ISDIR(sb.st_mode)) {
            if (strcmp(dt->d_name, ".") == 0 || strcmp(dt->d_name, "..") == 0) continue;
            get_video_list(buf, list);
        } else {
            if (is_video_file(dt->d_name))
                chuanjianlist(strdup(buf), list);
        }
    }
    closedir(pf);
}

#define TYPE_MUSIC 1
#define TYPE_VIDEO 2

void append_media_from_stdin(int type) {
    Hnode *list = (type == TYPE_MUSIC) ? b1 : video_list;
    if (!list) return;
    const char *hint = (type == TYPE_MUSIC) ?
        "MUSIC files (one per line, .mp3 only)" :
        "VIDEO files (one per line, .mp4/.avi/.mkv/.mov/.flv/.wmv)";
    printf("\n=== Append %s ===\n", hint);
    printf("Current count: %d\n", list->num);
    printf("Enter paths, type 'done' on a new line to finish.\n> ");
    char line[1024];
    int added = 0;
    while (fgets(line, sizeof(line), stdin)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        if (strlen(line) == 0) continue;
        if (strcmp(line, "done") == 0) break;
        char *path = strdup(line);
        if (!path) continue;
        char *ext = strrchr(path, '.');
        int valid = 0;
        if (type == TYPE_MUSIC && ext && strcmp(ext, ".mp3") == 0) valid = 1;
        else if (type == TYPE_VIDEO && ext && is_video_file(path)) valid = 1;
        if (valid) { chuanjianlist(path, list); added++; }
        else { free(path); printf("Ignored (invalid format): %s\n", line); }
        printf("> ");
    }
    printf("\nAdded %d file(s). Total: %d\n", added, list->num);
    clearerr(stdin);
}

/* ========== 音乐控制 ========== */
 void stop_music(void) {
    if (g_play_pid > 0) { kill(g_play_pid, SIGTERM); waitpid(g_play_pid, NULL, 0); g_play_pid = -1; }
    g_is_playing = 0;
    if (music_play_btn) {
        lv_obj_t *label = lv_obj_get_child(music_play_btn, 0);
        if (label) lv_label_set_text(label, "Play");
    }
}
void play_music_by_index(int index) {
    char *path = get_song_path(index);
    if (!path) return;
    stop_music();
    pid_t pid = fork();
    if (pid == 0) {
        execlp("mpg123", "mpg123", "-o", "alsa", path, NULL);
        exit(0);
    } else if (pid > 0) {
        g_play_pid = pid;
        g_is_playing = 1;
        g_song_index = index;
        if (music_play_btn) {
            lv_obj_t *label = lv_obj_get_child(music_play_btn, 0);
            if (label) lv_label_set_text(label, "Pause");
        }
    }
}

void music_prev_click(lv_event_t *e) {
    if (b1->num == 0) { printf("No music.\n"); return; }
    g_song_index = (g_song_index - 1 + b1->num) % b1->num;
    if (g_is_playing) play_music_by_index(g_song_index);
}

void music_next_click(lv_event_t *e) {
    if (b1->num == 0) { printf("No music.\n"); return; }
    g_song_index = (g_song_index + 1) % b1->num;
    if (g_is_playing) play_music_by_index(g_song_index);
}

void music_play_click(lv_event_t *e) {
    if (g_is_playing) { stop_music(); return; }
    if (b1->num == 0) { printf("No music files.\n"); return; }
    if (g_song_index >= b1->num) g_song_index = 0;
    play_music_by_index(g_song_index);
}

void music_add_click(lv_event_t *e) { add_scan_show(1); }

/* ========== 图片浏览 ========== */
void pre_click(lv_event_t *e) {
    if (!g_gif) return;
    g_gif_index = (g_gif_index - 1 + PIC_NUM) % PIC_NUM;
    lv_image_set_src(g_gif, g_pic_list[g_gif_index]);
}

void next_click(lv_event_t *e) {
    if (!g_gif) return;
    g_gif_index = (g_gif_index + 1) % PIC_NUM;
    lv_image_set_src(g_gif, g_pic_list[g_gif_index]);
}

void my_test10(void) {
    if (!bw) return;
    if (!g_gif) {
        g_gif = lv_image_create(bw);
        if (!g_gif) return;
        lv_obj_set_pos(g_gif, 0, 0);
        lv_obj_set_size(g_gif, 1024, 600);
    }
    lv_obj_move_to_index(g_gif, 1);
    lv_image_set_src(g_gif, "A:/lwx/1.bmp");
    lv_obj_remove_flag(g_gif, LV_OBJ_FLAG_HIDDEN);
    if (pre_btn) lv_obj_remove_flag(pre_btn, LV_OBJ_FLAG_HIDDEN);
    if (next_btn) lv_obj_remove_flag(next_btn, LV_OBJ_FLAG_HIDDEN);
    if (return_btn) lv_obj_remove_flag(return_btn, LV_OBJ_FLAG_HIDDEN);
}

/* 删除确认后的执行 */
static void do_delete_file(void) {
    if (!del_selected || !b1) return;
    Node *p = (Node *)lv_obj_get_user_data(del_selected);
    if (!p) return;
    /* 从双向循环链表中删除节点 */
    if (p->next == p) {  /* 只有一个节点 */
        b1->first = b1->last = NULL;
    } else {
        p->pre->next = p->next;
        p->next->pre = p->pre;
        if (b1->first == p) b1->first = p->next;
        if (b1->last == p) b1->last = p->pre;
    }
    b1->num--;
    free(p->data);
    free(p);
    /* 关闭窗口，刷新列表 */
    if (del_win) { lv_obj_del(del_win); del_win = NULL; del_list = NULL; del_selected = NULL; }
    printf("已删除文件\n");
}

/* 删除按钮回调 */
static void del_btn_click(lv_event_t *e) {
    if (!del_selected) {
        /* 没选文件，弹出提示框 */
        lv_obj_t *mbox = lv_obj_create(lv_screen_active());
        lv_obj_set_size(mbox, 300, 150); lv_obj_center(mbox);
        lv_obj_set_style_bg_color(mbox, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(mbox, 2, 0);
        lv_obj_set_style_radius(mbox, 10, 0);
        lv_obj_t *label = lv_label_create(mbox);
        lv_label_set_text(label, "请先选择文件");
        lv_obj_center(label);
        lv_obj_set_style_text_font(label, &lv_myfont_30, 0);
        lv_timer_t *t = lv_timer_create(close_tip_cb, 1500, mbox);
        lv_timer_set_repeat_count(t, 1);
        return;
    }
    do_delete_file();
}

/* 列表项点击回调 */
static void list_item_click(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    /* 取消上一个选中 */
    if (del_selected) lv_obj_set_style_bg_color(del_selected, lv_color_hex(0xFFFFFF), 0);
    /* 选中当前 */
    del_selected = btn;
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x87CEEB), 0);  /* 浅蓝高亮 */
}

/* 关闭窗口回调 */
static void del_close_click(lv_event_t *e) {
    if (del_win) { lv_obj_del(del_win); del_win = NULL; del_list = NULL; del_selected = NULL; }
}

/* 打开删除界面 */
void music_delete_click(lv_event_t *e) {
    if (del_win) return;
    if (!b1 || b1->num == 0) {
        printf("没有音乐文件\n");
        return;
    }
    del_win = lv_obj_create(lv_screen_active());
    lv_obj_set_size(del_win, 500, 500);
    lv_obj_center(del_win);
    lv_obj_set_style_bg_color(del_win, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(del_win, 2, 0);
    lv_obj_set_style_radius(del_win, 10, 0);

    lv_obj_t *title = lv_label_create(del_win);
    lv_label_set_text(title, "删除音乐文件");
    lv_obj_set_style_text_font(title, &lv_myfont_30, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* 可滚动的文件列表 */
    del_list = lv_list_create(del_win);
    lv_obj_set_size(del_list, 440, 340);
    lv_obj_align(del_list, LV_ALIGN_TOP_MID, 0, 50);

    Node *p = b1->first;
    if (p) {
        for (int i = 0; i < b1->num; i++) {
            /* 只显示文件名，不显示路径 */
            char *name = strrchr(p->data, '/');
            if (!name) name = p->data; else name++;
            lv_obj_t *btn = lv_list_add_btn(del_list, NULL, name);
            lv_obj_set_style_text_font(lv_obj_get_child(btn, 0), &lv_myfont_30, 0);
            lv_obj_set_user_data(btn, p);
            lv_obj_add_event_cb(btn, list_item_click, LV_EVENT_CLICKED, NULL);
            p = p->next;
        }
    }

    /* 删除按钮 */
    lv_obj_t *del_btn = lv_button_create(del_win);
    lv_obj_set_size(del_btn, 120, 50);
    lv_obj_align(del_btn, LV_ALIGN_BOTTOM_LEFT, 40, -20);
    lv_obj_t *del_lb = lv_label_create(del_btn);
    lv_label_set_text(del_lb, "删除");
    lv_obj_center(del_lb);
    lv_obj_set_style_text_font(del_lb, &lv_myfont_30, 0);
    lv_obj_add_event_cb(del_btn, del_btn_click, LV_EVENT_CLICKED, NULL);

    /* 关闭按钮 */
    lv_obj_t *close_btn = lv_button_create(del_win);
    lv_obj_set_size(close_btn, 120, 50);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_RIGHT, -40, -20);
    lv_obj_t *close_lb = lv_label_create(close_btn);
    lv_label_set_text(close_lb, "关闭");
    lv_obj_center(close_lb);
    lv_obj_set_style_text_font(close_lb, &lv_myfont_30, 0);
    lv_obj_add_event_cb(close_btn, del_close_click, LV_EVENT_CLICKED, NULL);
}