#include "custom.h"
#include "ui_objects.h"
#include "lv_aic_player.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <strings.h>
#include <dirent.h>
#endif

media_list_t player_list;

static bool is_video_file(const char *filename) {
    const char *extensions[] = { ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", ".png", NULL };
    const char **ext = extensions;
    const char *dot = strrchr(filename, '.');
    if (!dot) {
        return false;
    }
    while (*ext) {
#ifdef _WIN32
        if (_stricmp(dot, *ext) == 0)
#else
        if (strcasecmp(dot, *ext) == 0)
#endif
        {
            return true;
        }
        ext++;
    }
    return false;
}
static char *extract_name(const char *filename) {
    const char *last_slash = strrchr(filename, '/');
    const char *last_backslash = strrchr(filename, '\\');
    const char *base = filename;

    if (last_slash || last_backslash) {
        const char *last_sep = (last_slash > last_backslash) ? last_slash : last_backslash;
        base = last_sep + 1;
    }

    const char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);

    char *name = malloc(len + 1);
    if (name) {
        strncpy(name, base, len);
        name[len] = '\0';
    }
    return name;
}
static int add_file_to_list(const char *path, const char *name) {
    char **new_paths = realloc(player_list.paths, (player_list.count + 1) * sizeof(char *));
    if (!new_paths) {
        printf("Memory allocation failed for paths\n");
        return 0;
    }

    char **new_names = realloc(player_list.names, (player_list.count + 1) * sizeof(char *));
    if (!new_names) {
        printf("Memory allocation failed for names\n");
        free(new_paths);
        return 0;
    }

    player_list.paths = new_paths;
    player_list.names = new_names;

    player_list.paths[player_list.count] = strdup(path);
    if (!player_list.paths[player_list.count]) {
        printf("Memory allocation failed for path string\n");
        return 0;
    }

    player_list.names[player_list.count] = strdup(name);
    if (!player_list.names[player_list.count]) {
        printf("Memory allocation failed for name string\n");
        free(player_list.paths[player_list.count]);
        return 0;
    }

    player_list.count++;
    return 1;
}
static int scan_directory(const char *dirpath) {
    int file_count = 0;

    char *real_path = (char *)dirpath;
    if (real_path[1] == ':' && ((real_path[0] >= 'A' && real_path[0] <= 'Z') || (real_path[0] >= 'a' && real_path[0] <= 'z'))) {
        real_path += 2;
    }

    char normalized_path[256];
    strncpy(normalized_path, real_path, sizeof(normalized_path) - 2);
    normalized_path[sizeof(normalized_path) - 1] = '\0';

    size_t len = strlen(normalized_path);
    if (len > 0) {
        if (normalized_path[len - 1] != '/') {
            strcat(normalized_path, "/");
        }
    }

    printf("Scanning directory: %s\n", normalized_path);

#ifdef _WIN32
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s*", normalized_path);

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Failed to open directory: %s, error: %lu\n", normalized_path, GetLastError());
        return 0;
    }

    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            is_video_file(findData.cFileName)) {
            char fullPath[MAX_PATH];
            snprintf(fullPath, sizeof(fullPath), "%s%s", normalized_path, findData.cFileName);

            char *name = extract_name(findData.cFileName);
            if (name) {
                printf("Found video file: %s -> %s\n", fullPath, name);
                if (add_file_to_list(fullPath, name)) {
                    file_count++;
                }
                free(name);
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

#else
    DIR *dir = opendir(normalized_path);
    if (!dir) {
        printf("Failed to open directory: %s\n", normalized_path);
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && is_video_file(entry->d_name)) {
            char fullPath[256];
            snprintf(fullPath, sizeof(fullPath), "%s%s", normalized_path, entry->d_name);

            char *name = extract_name(entry->d_name);
            if (name) {
                printf("Found video file: %s -> %s\n", fullPath, name);
                if (add_file_to_list(fullPath, name)) {
                    file_count++;
                }
                free(name);
            }
        }
    }

    closedir(dir);
#endif

    return file_count;
}

void player_list_init(void) {
    player_list.paths = NULL;
    player_list.names = NULL;
    player_list.count = 0;
    player_list.current = 0;

    const char *video_dir = LVGL_DIR"video/";
    printf("Video directory: %s\n", video_dir);

    int file_count = scan_directory(video_dir);
    printf("Found %d video files\n", file_count);

    if (file_count == 0) {
        printf("Warning: No video files found. Check if directory exists and contains video files.\n");
    }
}
void player_list_cleanup(void) {
    for (int i = 0; i < player_list.count; i++) {
        free(player_list.paths[i]);
        free(player_list.names[i]);
    }
    free(player_list.paths);
    free(player_list.names);

    player_list.paths = NULL;
    player_list.names = NULL;
    player_list.count = 0;
    player_list.current = 0;
}

int player_list_get_count(void) {
    return player_list.count;
}

const char *player_list_get_path(int index) {
    if (index >= 0 && index < player_list.count) {
        return player_list.paths[index];
    }
    printf("Invalid path index: %d (count: %d)\n", index, player_list.count);
    return NULL;
}

const char *player_list_get_name(int index) {
    if (index >= 0 && index < player_list.count) {
        return player_list.names[index];
    }
    printf("Invalid name index: %d (count: %d)\n", index, player_list.count);
    return NULL;
}

int player_list_set_current(int index) {
    if (index >= 0 && index < player_list.count) {
        player_list.current = index;
        return 1;
    }
    printf("Invalid current index: %d (count: %d)\n", index, player_list.count);
    return 0;
}

int player_list_get_current(void) {
    return player_list.current;
}

void scr_video_play_control_custom_clicked(void) {
    scr_video_t *scr = scr_video_get(&ui_manager);

    lv_obj_t *player = scr->video_player;
    lv_obj_t *play_img = scr->play_control;

    if (strstr(lv_img_get_src(play_img), "pause.png")) {
        lv_img_set_src(play_img, LVGL_IMAGE_PATH(video/play.png));
        lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_PAUSE, NULL);
    } else {
        lv_img_set_src(play_img, LVGL_IMAGE_PATH(video/pause.png));
        lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_RESUME, NULL);
    }
}


void scr_video_video_player_custom_clicked(void) {
    scr_video_t *scr = scr_video_get(&ui_manager);
    lv_obj_t *control_bar = scr->control_bar;
    lv_obj_t *file_name = scr->file_name;
    lv_obj_t *author = scr->author;
    lv_obj_t *back = scr->back;

    if (lv_obj_has_flag(control_bar, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_clear_flag(control_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(file_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(author, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(back, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(control_bar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(file_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(author, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(back, LV_OBJ_FLAG_HIDDEN);
    }
}

void scr_video_zoom_custom_clicked(void) {
    scr_video_t *scr = scr_video_get(&ui_manager);
    lv_obj_t *zoom_img = scr->zoom;
    lv_obj_t *player = scr->video_player;
    printf("%s", lv_img_get_src(zoom_img));

    if (strstr(lv_img_get_src(zoom_img), "zoom_out.png")) {
        lv_img_set_src(zoom_img, LVGL_IMAGE_PATH(video/zoom_in.png));
        lv_obj_center(player);
        lv_aic_player_set_scale(player, 128);
    } else {
        lv_img_set_src(zoom_img, LVGL_IMAGE_PATH(video/zoom_out.png));
        lv_obj_center(player);
        lv_aic_player_set_scale(player, 256);
    }
}
static void set_player(void) {
    lv_obj_t *player = scr_video_get(&ui_manager)->video_player;
    lv_obj_t *file_name_label = scr_video_get(&ui_manager)->file_name;
    lv_obj_t *play_img = scr_video_get(&ui_manager)->play_control;

    const char *path = player_list_get_path(player_list.current);
    const char *name = player_list_get_name(player_list.current);

    if (path && name) {
        lv_aic_player_set_src(player, path);
        lv_aic_player_set_cmd(player, LV_AIC_PLAYER_CMD_START, NULL);
        lv_label_set_text(file_name_label, name);
//        lv_img_set_src(play_img, LVGL_IMAGE_PATH(video/play.png));
    } else {
        printf("Error: Invalid path or name at index %d\n", player_list.current);
    }
}

void scr_video_next_custom_clicked(void) {
    player_list.current++;
    if (player_list.current >= player_list.count)
        player_list.current = 0;
    set_player();
}

void scr_video_last_custom_clicked(void) {
    player_list.current--;
    if (player_list.current < 0)
        player_list.current = player_list.count - 1;
    set_player();
}