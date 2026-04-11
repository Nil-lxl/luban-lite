#pragma once

typedef struct _media_list {
    char **paths;
    char **names;
    int count;
    int current;
} media_list_t;

extern media_list_t player_list;

void player_list_init(void);
void player_list_cleanup(void);
int player_list_get_count(void);
const char *player_list_get_path(int index);
const char *player_list_get_name(int index);
int player_list_set_current(int index);
int player_list_get_current(void);