#include "globals.h"

int chat_active = 0;
lv_obj_t *chat_win = NULL;
lv_obj_t *chat_textarea = NULL;
lv_obj_t *chat_send_btn = NULL;
lv_obj_t *chat_exit_btn = NULL;
lv_obj_t *chat_keyboard = NULL;
lv_obj_t *chat_msg_area = NULL;
lv_obj_t *chat_btn = NULL;
lv_obj_t *chat_request_dialog = NULL;
char pending_chat_msg[512] = {0};

 void get_time_str(char *buf, int size) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    snprintf(buf, size, "%02d:%02d", tm->tm_hour, tm->tm_min);
}

 void chat_send_cb(lv_event_t *e) {
    if (!chat_active || sockfd < 0) return;
    const char *text = lv_textarea_get_text(chat_textarea);
    if (strlen(text) == 0) return;
    char buf[256];
    if (text[0] == '@') snprintf(buf, sizeof(buf), "%s\n", text);
    else snprintf(buf, sizeof(buf), "CHAT:%s\n", text);
    int ret = send(sockfd, buf, strlen(buf), 0);
    if (ret < 0) perror("send chat");
    else {
        if (chat_msg_area) {
            char ts[16]; get_time_str(ts, sizeof(ts));
            lv_textarea_add_text(chat_msg_area, ts);
            lv_textarea_add_text(chat_msg_area, " 我: ");
            lv_textarea_add_text(chat_msg_area, text);
            lv_textarea_add_text(chat_msg_area, "\n");
        }
        printf("Sent chat: %s", text);
        lv_textarea_set_text(chat_textarea, "");
    }
}

void chat_exit_cb(lv_event_t *e) {
    if (!chat_active) { printf("Chat already inactive.\n"); return; }
    if (sockfd >= 0) send(sockfd, "CHAT_EXIT\n", 10, 0);
    if (chat_keyboard) { lv_obj_add_flag(chat_keyboard, LV_OBJ_FLAG_HIDDEN); lv_obj_del(chat_keyboard); chat_keyboard = NULL; }
    if (chat_win) {
        lv_obj_del(chat_win); chat_win = NULL; chat_textarea = NULL;
        chat_send_btn = NULL; chat_exit_btn = NULL; chat_msg_area = NULL;
    }
    chat_active = 0; printf("Chat exited.\n");
}

 void chat_btn_click(lv_event_t *e) {
    if (!(net_running && sockfd >= 0)) { printf("请先点 Connect 连接服务器\n"); return; }
    if (chat_active) { printf("聊天已激活\n"); return; }
    create_chat_ui();
}

 void create_chat_ui(void) {
    if (chat_win) { printf("Chat window already exists.\n"); return; }
    chat_active = 1;
    chat_win = lv_obj_create(lv_screen_active());
    lv_obj_set_size(chat_win, 800, 540); lv_obj_center(chat_win);
    lv_obj_set_style_bg_color(chat_win, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(chat_win, 2, 0);
    lv_obj_set_style_border_color(chat_win, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(chat_win, 10, 0);
    lv_obj_t *title = lv_label_create(chat_win);
    lv_label_set_text(title, "Chat with User");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);
    chat_msg_area = lv_textarea_create(chat_win);
    if (chat_msg_area) {
        lv_obj_set_size(chat_msg_area, 740, 300); lv_obj_align(chat_msg_area, LV_ALIGN_TOP_MID, 0, 50);
        lv_textarea_set_placeholder_text(chat_msg_area, "聊天记录"); lv_textarea_set_text(chat_msg_area, "");
        lv_obj_remove_flag(chat_msg_area, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_set_style_text_font(chat_msg_area, &lv_myfont_30, 0);
    }
    chat_textarea = lv_textarea_create(chat_win);
    lv_obj_set_size(chat_textarea, 560, 60); lv_obj_align(chat_textarea, LV_ALIGN_BOTTOM_MID, 0, -215);
    lv_textarea_set_placeholder_text(chat_textarea, "Type your message...");
    lv_obj_set_style_bg_color(chat_textarea, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(chat_textarea, &lv_myfont_30, 0);
    chat_send_btn = lv_btn_create(chat_win);
    lv_obj_set_size(chat_send_btn, 100, 50); lv_obj_align_to(chat_send_btn, chat_textarea, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_t *send_label = lv_label_create(chat_send_btn); lv_label_set_text(send_label, "Send"); lv_obj_center(send_label);
    lv_obj_add_event_cb(chat_send_btn, chat_send_cb, LV_EVENT_CLICKED, NULL);
    chat_exit_btn = lv_btn_create(chat_win);
    lv_obj_set_size(chat_exit_btn, 100, 50); lv_obj_align(chat_exit_btn, LV_ALIGN_TOP_RIGHT, -10, 8);
    lv_obj_t *exit_label = lv_label_create(chat_exit_btn); lv_label_set_text(exit_label, "Exit"); lv_obj_center(exit_label);
    lv_obj_add_event_cb(chat_exit_btn, chat_exit_cb, LV_EVENT_CLICKED, NULL);
    chat_keyboard = lv_keyboard_create(lv_screen_active());
    lv_keyboard_set_textarea(chat_keyboard, chat_textarea);
    lv_obj_set_size(chat_keyboard, 800, 200); lv_obj_align(chat_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    printf("Chat UI created.\n");
}

 void chat_request_yes_cb(lv_event_t *e) {
    if (chat_request_dialog) { lv_obj_del(chat_request_dialog); chat_request_dialog = NULL; }
    create_chat_ui();
    if (chat_msg_area && strlen(pending_chat_msg) > 0) {
        lv_textarea_add_text(chat_msg_area, "对方: ");
        lv_textarea_add_text(chat_msg_area, pending_chat_msg); lv_textarea_add_text(chat_msg_area, "\n");
        pending_chat_msg[0] = '\0';
    }
}

 void chat_request_no_cb(lv_event_t *e) {
    if (chat_request_dialog) { lv_obj_del(chat_request_dialog); chat_request_dialog = NULL; }
    pending_chat_msg[0] = '\0'; printf("Chat request denied.\n");
}

void show_chat_request_dialog(const char *msg) {
    if (chat_active) { if (chat_msg_area) { lv_textarea_add_text(chat_msg_area, "对方: "); lv_textarea_add_text(chat_msg_area, msg); lv_textarea_add_text(chat_msg_area, "\n"); } return; }
    if (chat_request_dialog) return;
    strncpy(pending_chat_msg, msg, sizeof(pending_chat_msg)-1);
    pending_chat_msg[sizeof(pending_chat_msg)-1] = '\0';
    lv_obj_t *parent = lv_screen_active();
    lv_obj_t *mbox = lv_obj_create(parent); chat_request_dialog = mbox;
    lv_obj_set_size(mbox, 400, 220); lv_obj_center(mbox);
    lv_obj_set_style_bg_color(mbox, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(mbox, 2, 0); lv_obj_set_style_border_color(mbox, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(mbox, 10, 0);
    lv_obj_t *title = lv_label_create(mbox); lv_label_set_text(title, "Chat Request");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0); lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_t *msg_label = lv_label_create(mbox);
    char preview[64]; snprintf(preview, sizeof(preview), "Message: %.40s%s", msg, strlen(msg)>40?"...":"");
    lv_label_set_text(msg_label, preview); lv_obj_align(msg_label, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_font(msg_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_font(msg_label, &lv_myfont_30, 0);
    lv_obj_t *btn_yes = lv_btn_create(mbox); lv_obj_set_size(btn_yes, 80, 40);
    lv_obj_align(btn_yes, LV_ALIGN_BOTTOM_LEFT, 40, -20);
    lv_obj_t *yes_label = lv_label_create(btn_yes); lv_label_set_text(yes_label, "Yes"); lv_obj_center(yes_label);
    lv_obj_add_event_cb(btn_yes, chat_request_yes_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_no = lv_btn_create(mbox); lv_obj_set_size(btn_no, 80, 40);
    lv_obj_align(btn_no, LV_ALIGN_BOTTOM_RIGHT, -40, -20);
    lv_obj_t *no_label = lv_label_create(btn_no); lv_label_set_text(no_label, "No"); lv_obj_center(no_label);
    lv_obj_add_event_cb(btn_no, chat_request_no_cb, LV_EVENT_CLICKED, NULL);
}

void trigger_chat_request(const char *msg) {
    if (chat_active || game_dialog != NULL) { printf("Chat already active or dialog present.\n"); return; }
    show_chat_request_dialog(msg ? msg : "Other user wants to chat.");
}