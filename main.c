#include "globals.h"
#include "globals.h"
#include "lv_myfont_30.h"
#include "hammer_img.c"
#include "star_img.c"
/* ========== 网络通信全局变量 ========== */
#define SERVER_IP "172.40.0.113"
#define SERVER_PORT 12345
int sockfd = -1;
int net_running = 0;
static pthread_t net_thread;
char cmd_queue[CMD_QUEUE_SIZE][CMD_LEN];
int cmd_head = 0;
int cmd_tail = 0;
pthread_mutex_t cmd_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ========== 网络线程 ========== */
 void update_comm_btn_cb(void *data) {
    if (comm_btn) { lv_obj_t *label = lv_obj_get_child(comm_btn, 0); if (label) lv_label_set_text(label, "Connect"); }
}

void *net_thread_func(void *arg) {
    char buffer[1024]; int n;
    while (net_running && sockfd >= 0) {
        memset(buffer, 0, sizeof(buffer));
        n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            char *line = strtok(buffer, "\n");
            while (line) {
                pthread_mutex_lock(&cmd_mutex);
                int next_tail = (cmd_tail + 1) % CMD_QUEUE_SIZE;
                if (next_tail != cmd_head) {
                    strncpy(cmd_queue[cmd_tail], line, CMD_LEN - 1);
                    cmd_queue[cmd_tail][CMD_LEN - 1] = '\0';
                    cmd_tail = next_tail;
                }
                pthread_mutex_unlock(&cmd_mutex);
                line = strtok(NULL, "\n");
            }
        } else {
            if (n == 0) printf("Server closed.\n"); else perror("recv");
            break;
        }
    }
    if (sockfd >= 0) { shutdown(sockfd, SHUT_RDWR); close(sockfd); sockfd = -1; }
    net_running = 0;
    lv_async_call(update_comm_btn_cb, NULL);
    return NULL;
}

 void start_network_client(void) {
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return; }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) { perror("inet_pton"); close(sockfd); sockfd = -1; return; }
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { perror("connect"); close(sockfd); sockfd = -1; return; }
    printf("Connected to %s:%d\n", SERVER_IP, SERVER_PORT);
    net_running = 1;
    if (pthread_create(&net_thread, NULL, net_thread_func, NULL) != 0) { perror("pthread_create"); close(sockfd); sockfd = -1; net_running = 0; return; }
    pthread_detach(net_thread);
    if (comm_btn) { lv_obj_t *label = lv_obj_get_child(comm_btn, 0); if (label) lv_label_set_text(label, "Disconnect"); }
}

void comm_btn_click(lv_event_t *e) {
    if (net_running && sockfd >= 0) {
        send(sockfd, "CHAT_EXIT\n", 10, 0);
        net_running = 0; shutdown(sockfd, SHUT_RDWR); close(sockfd); sockfd = -1;
        if (comm_btn) { lv_obj_t *label = lv_obj_get_child(comm_btn, 0); if (label) lv_label_set_text(label, "Connect"); }
        if (chat_active) chat_exit_cb(NULL);
        printf("Disconnected.\n");
    } else { start_network_client(); }
}

/* ========== 命令处理 ========== */
void execute_command(char *cmd) {
    int len = strlen(cmd);
    while (len > 0 && (cmd[len-1] == '\n' || cmd[len-1] == '\r' || cmd[len-1] == ' ' || cmd[len-1] == '\t')) cmd[--len] = '\0';
    if (strlen(cmd) == 0) return;
    printf("CMD: '%s'\n", cmd); fflush(stdout);
    if (strcmp(cmd, "music") == 0 || strcmp(cmd, "switch_music") == 0) { switch_to_music(); return; }
    if (strcmp(cmd, "video") == 0 || strcmp(cmd, "switch_video") == 0) { switch_to_video(); return; }
    if (strcmp(cmd, "game") == 0 || strcmp(cmd, "switch_game") == 0) { hide_video_window(); game_btn_handler(NULL); return; }
    if (strcmp(cmd, "start") == 0) {
        if (game_win && !lv_obj_has_flag(game_win, LV_OBJ_FLAG_HIDDEN) && game_running == 0 && game_start_btn && !lv_obj_has_flag(game_start_btn, LV_OBJ_FLAG_HIDDEN)) { game_start_cb(NULL); }
        else printf("Game not ready.\n"); return;
    }
    if (strncmp(cmd, "CHAT:", 5) == 0) {
        char *msg = cmd + 5;
        if (chat_active) { if (chat_msg_area) { lv_textarea_add_text(chat_msg_area, "对方: "); lv_textarea_add_text(chat_msg_area, msg); lv_textarea_add_text(chat_msg_area, "\n"); } }
        else show_chat_request_dialog(msg);
        return;
    }
    if (strcmp(cmd, "play") == 0) {
        int mv = (cw && !lv_obj_has_flag(cw, LV_OBJ_FLAG_HIDDEN));
        int vv = (video_win && !lv_obj_has_flag(video_win, LV_OBJ_FLAG_HIDDEN));
        int vc = (vctrl_win && !lv_obj_has_flag(vctrl_win, LV_OBJ_FLAG_HIDDEN));
        if (mv) { music_play_click(NULL); }
        else if (vv) { video_play_click(NULL); }
        else if (vc) { vctrl_play_click(NULL); }
        else {
            if (video_list && video_list->num > 0) switch_to_video();
            else if (b1 && b1->num > 0) switch_to_music();
        }
        return;
    }
    if (strcmp(cmd, "pause") == 0) {
        if (cw && !lv_obj_has_flag(cw, LV_OBJ_FLAG_HIDDEN) && g_is_playing) music_play_click(NULL);
        else if (video_win && !lv_obj_has_flag(video_win, LV_OBJ_FLAG_HIDDEN) && g_video_playing) video_play_click(NULL);
        else if (vctrl_win && !lv_obj_has_flag(vctrl_win, LV_OBJ_FLAG_HIDDEN)) video_play_click(NULL);
        return;
    }
    if (strcmp(cmd, "next") == 0) {
        if (cw && !lv_obj_has_flag(cw, LV_OBJ_FLAG_HIDDEN)) music_next_click(NULL);
        else if (video_win && !lv_obj_has_flag(video_win, LV_OBJ_FLAG_HIDDEN)) video_next_click(NULL);
        return;
    }
    if (strcmp(cmd, "prev") == 0) {
        if (cw && !lv_obj_has_flag(cw, LV_OBJ_FLAG_HIDDEN)) music_prev_click(NULL);
        else if (video_win && !lv_obj_has_flag(video_win, LV_OBJ_FLAG_HIDDEN)) video_prev_click(NULL);
        return;
    }
    if (strncmp(cmd, "add_music ", 10) == 0) { char *p = cmd + 10; char *d = strdup(p); if (d) chuanjianlist(d, b1); return; }
    if (strncmp(cmd, "add_video ", 10) == 0) { char *p = cmd + 10; char *d = strdup(p); if (d) chuanjianlist(d, video_list); return; }
    if (strcmp(cmd, "status") == 0) { printf("Music: %s, Video: %s, Chat: %s\n", g_is_playing?"playing":"stopped", g_video_playing?"playing":"stopped", chat_active?"active":"inactive"); return; }
    if (!chat_active) show_chat_request_dialog(cmd);
    else { if (chat_msg_area) { lv_textarea_add_text(chat_msg_area, "对方: "); lv_textarea_add_text(chat_msg_area, cmd); lv_textarea_add_text(chat_msg_area, "\n"); } printf("收到: %s\n", cmd); }
}

/* ========== 空测试函数 ========== */
void my_test9(void) {}
void button_handler(lv_event_t *e) {}
void my_test7() {} void my_test6() {} void my_test4() {}
void my_test3(int a,int b,int c) {} void my_test2(int a, int b, int c,int d) {} void my_test1(void) {}

/* ========== main ========== */
int main(int argc, char *argv[]) {
    srand(time(NULL));
    lv_init();
    lv_display_t *disp = lv_linux_fbdev_create();
    if (disp) lv_linux_fbdev_set_file(disp, "/dev/fb0");
    lv_indev_t *indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event6");

    load_high_score();

    b1 = malloc(sizeof(Hnode)); if (b1) { b1->first = b1->last = NULL; b1->num = 0; }
    video_list = malloc(sizeof(Hnode)); if (video_list) { video_list->first = video_list->last = NULL; video_list->num = 0; }

    if (argc == 2) {
        char *path = argv[1];
        printf("Scanning: %s\n", path);
        getlist(path, b1); get_video_list(path, video_list);
        printf("Loaded %d music, %d video\n", b1->num, video_list->num);
    } else printf("Usage: %s <media_dir>\n", argv[0]);

    lv_example_keyboard_1();

    while (1) {
        if (g_video_playing && g_video_pid > 0) {
            int status;
            pid_t ret = waitpid(g_video_pid, &status, WNOHANG);
            if (ret > 0) {
                stop_video();
                if (video_play_btn) { lv_obj_t *label = lv_obj_get_child(video_play_btn, 0); if (label) lv_label_set_text(label, "Play"); }
                g_video_seconds = 0;
                if (video_time_label) lv_label_set_text_fmt(video_time_label, "00:00");
                printf("Video finished.\n");
            }
        }
        lv_timer_handler();
        pthread_mutex_lock(&cmd_mutex);
        while (cmd_head != cmd_tail) {
            char cmd[CMD_LEN]; strcpy(cmd, cmd_queue[cmd_head]);
            cmd_head = (cmd_head + 1) % CMD_QUEUE_SIZE;
            pthread_mutex_unlock(&cmd_mutex);
            execute_command(cmd);
            pthread_mutex_lock(&cmd_mutex);
        }
        pthread_mutex_unlock(&cmd_mutex);
        usleep(5000);
    }
}