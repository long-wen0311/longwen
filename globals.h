#ifndef GLOBALS_H
#define GLOBALS_H

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
//#include "lv_myfont_30.h"
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
//#include "hammer_img.c"
//#include "star_img.c"
extern const lv_image_dsc_t hammer_img;
extern const lv_image_dsc_t star_img;
extern const lv_font_t lv_myfont_30;
extern const lv_image_dsc_t hammer_img;
extern const lv_image_dsc_t star_img;
#define TYPE_MUSIC 1
#define TYPE_VIDEO 2
/* ========== 结构体 ========== */
typedef char *ElemType;
typedef struct node {
    ElemType data;
    struct node *next;
    struct node *pre;
} Node;
typedef struct hnode {
    struct node *first;
    struct node *last;
    int num;
} Hnode;

/* ========== 全局变量 ========== */
extern int i;
extern lv_obj_t *dw, *aw, *bw, *cw;
extern lv_obj_t *album_btn, *music_btn, *alenda_btn;
extern lv_obj_t *pre_btn, *next_btn, *video_btn, *return_btn;
extern lv_obj_t *game_btn;
extern Hnode *b1;
extern Hnode *video_list;
extern lv_obj_t *comm_btn;
extern lv_obj_t *back_to_aw_btn;

/* 图片浏览 */
extern int g_gif_index;
extern lv_obj_t *g_gif;

/* 视频 */
extern lv_obj_t *video_win;
extern lv_obj_t *video_play_btn, *video_prev_btn, *video_next_btn, *video_return_btn;
extern lv_obj_t *video_time_label;
extern pid_t g_video_pid;
extern int g_video_playing;
extern lv_timer_t *video_timer;
extern int g_video_seconds;
extern int g_video_index;
extern char *g_video_path;
extern lv_disp_t *g_saved_disp;

/* 音乐 */
extern int g_song_index;
extern pid_t g_play_pid;
extern int g_is_playing;
extern lv_obj_t *music_play_btn;

/* 游戏 */
extern lv_obj_t *game_win;
extern lv_obj_t *game_return_btn;
extern lv_obj_t *game_score_label;
extern lv_obj_t *game_timer_label;
extern lv_obj_t *game_holes[16];
extern int game_score;
extern int game_time_left;
extern lv_timer_t *game_timer;
extern lv_timer_t *game_spawn_timer;
extern int game_running;
extern int high_score;
extern lv_obj_t *game_dialog;
extern lv_obj_t *game_start_btn;

/* 网络 */
extern int sockfd;
extern int net_running;
#define CMD_QUEUE_SIZE 16
#define CMD_LEN 128
extern char cmd_queue[CMD_QUEUE_SIZE][CMD_LEN];
extern int cmd_head;
extern int cmd_tail;
extern pthread_mutex_t cmd_mutex;
extern lv_obj_t *vctrl_win;

/* 聊天 */
extern int chat_active;
extern lv_obj_t *chat_win;
extern lv_obj_t *chat_textarea;
extern lv_obj_t *chat_send_btn;
extern lv_obj_t *chat_exit_btn;
extern lv_obj_t *chat_keyboard;
extern lv_obj_t *chat_msg_area;
extern lv_obj_t *chat_btn;
extern lv_obj_t *chat_request_dialog;
extern char pending_chat_msg[512];

/* 日历 */
extern lv_obj_t *g_calendar;
extern lv_obj_t *cal_input_ta;
extern lv_calendar_date_t g_cal_mark_dates[50];
extern int g_cal_mark_count;

/* 密码 */
extern char g_pwd1[64];
extern char g_pwd2[64];
extern int login_stage;
extern lv_obj_t *login_ta;
extern lv_obj_t *login_kb;
extern lv_obj_t *login_tip;

extern lv_obj_t *setting_win;
extern lv_obj_t *music_play_btn;
extern int high_score;

/* ========== 函数声明 ========== */
void music_prev_click(lv_event_t *e);
void music_next_click(lv_event_t *e);
void music_play_click(lv_event_t *e);
void music_add_click(lv_event_t *e);
void pre_click(lv_event_t *e);
void next_click(lv_event_t *e);
void my_test10(void);
void chuanjianlist(char *buf, Hnode *list);
void getlist(char *path, Hnode *list);
void get_video_list(char *path, Hnode *list);
void append_media_from_stdin(int type);
char* get_song_path(int index);
char* get_video_path_by_index(int index);
void video_play_click(lv_event_t *e);
void video_prev_click(lv_event_t *e);
void video_next_click(lv_event_t *e);
void video_btn_handler(lv_event_t *e);
void hide_video_window(void);
void stop_video(void);
void create_video_window(void);            /* ★新增 */
void game_btn_handler(lv_event_t *e);
void game_start_cb(lv_event_t *e);
void load_high_score(void);
void chat_exit_cb(lv_event_t *e);
void chat_btn_click(lv_event_t *e);
void create_chat_ui(void);                 /* ★新增 */
void show_chat_request_dialog(const char *msg);
void trigger_chat_request(const char *msg);
void switch_to_music(void);
void switch_to_video(void);
void my_test8(void);
void create_setting_win(void);
void setting_btn_click(lv_event_t *e);
void execute_command(char *cmd);
void comm_btn_click(lv_event_t *e);        /* ★新增 */
void lv_example_keyboard_1(void);
void music_delete_click(lv_event_t *e);
void video_add_click(lv_event_t *e);
void video_return_click(lv_event_t *e);
void vol_down_cb(lv_event_t *e);
void vol_up_cb(lv_event_t *e);
void vctrl_play_click(lv_event_t *e);
void add_scan_show(int type);
#endif