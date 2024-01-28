//
// Created by gifbadge on 26/01/24.
//

#include <filesystem>
#include <dirent.h>
#include "ui/file_select.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/device_group.h"
#include "ui/style.h"

extern "C" {

struct file_data {
    char top[255];
    char current[255];
};

static void file_list_chdir(lv_obj_t *parent, const std::filesystem::path &path) {
    LV_LOG_USER("chdir Path: %s", (path).c_str());
    auto *d = static_cast<file_data *>(lv_file_list_get_user_data(parent));
    strcpy(d->current, path.c_str());
    lv_file_list_clear(parent);
    file_list(parent);
    lv_file_list_scroll_to_view(parent, 0);
}

static void file_event_handler(lv_event_t *e) {

    auto *container = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    auto *d = static_cast<file_data *>(lv_file_list_get_user_data(container));


    lv_obj_t *obj = lv_event_get_target(e);
    char *text = lv_label_get_text(obj);
    auto path = std::filesystem::path(d->current);
    LV_LOG_USER("Clicked: %s %s", path.c_str(), text);
    if (strcmp(text, "Up") == 0) {
        LV_LOG_USER("Clicked: %s", path.c_str());
        file_list_chdir(container, (path / "..").lexically_normal());
    } else if (strcmp(text, "Entire Folder") == 0) {
        if (!std::filesystem::is_directory(path)) {
            path = (path.parent_path());
        }
        strcpy(d->current, (path).lexically_normal().c_str());
        lv_obj_del(container);
    } else {
        if (std::filesystem::is_regular_file(path)) {
            path = path.parent_path();
        }
        LV_LOG_USER("Save Path: %s", (path / text).c_str());
        if (std::filesystem::is_directory(path / text)) {
            file_list_chdir(container, path / text);
        } else if (std::filesystem::is_regular_file(path / text)) {
            strcpy(d->current, (path / text).lexically_normal().c_str());
            lv_obj_del(container);
        }
    }
}

static lv_obj_t *file_entry(lv_obj_t *parent, const char *icon, const char *text) {
    lv_obj_t *btn = lv_file_list_add(parent, icon);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *label = lv_label_create(btn);
    lv_obj_add_style(label, &menu_font_style, LV_PART_MAIN);
    lv_label_set_text(label, text);
    lv_obj_add_event_cb(label, file_event_handler, LV_EVENT_CLICKED, parent);
    return btn;
}

void file_list(lv_obj_t *parent) {
    auto *d = static_cast<file_data *>(lv_file_list_get_user_data(parent));
    auto top = std::filesystem::path(d->top);
    auto current = std::filesystem::path(d->current);
    if (!std::filesystem::is_directory(current)) {
        current = current.parent_path();
    }

    if (top.compare(current.lexically_normal()) < 0) {
        LV_LOG_USER("Not in Top");
        file_entry(parent, ICON_UP, "Up");
    }
    file_entry(parent, ICON_FOLDER_OPEN, "Entire Folder");

    LV_LOG_USER("Path: %s", current.c_str());

    DIR *dp;
    struct dirent *ep;
    dp = opendir(current.c_str());
    if (dp != nullptr) {
        while ((ep = readdir(dp)) != nullptr) {
            if (ep->d_name[0] == '.')
                continue;
            if (ep->d_type == DT_DIR)
                file_entry(parent, ICON_FOLDER, ep->d_name);
        }
    }
    closedir(dp);
    dp = opendir(current.c_str());
    if (dp != nullptr) {
        while ((ep = readdir(dp)) != nullptr) {
            if (ep->d_name[0] == '.')
                continue;
            if (ep->d_type == DT_REG)
                file_entry(parent, ICON_IMAGE, ep->d_name);
        }
        closedir(dp);
    }
}

lv_obj_t *file_select(const char *top, const char *current) {
    lv_scr_load(lv_obj_create(nullptr));
    new_group();
    lv_obj_t *cont_flex = lv_file_list_create(lv_scr_act());
    lv_file_list_icon_style(cont_flex, &icon_style);

    auto *d = static_cast<file_data *>(malloc(sizeof(file_data)));
    strcpy(d->current, current);
    strcpy(d->top, top);

    lv_file_list_set_user_data(cont_flex, d);

    file_list(cont_flex);

    lv_file_list_scroll_to_view(cont_flex, 0);
    return cont_flex;
}

void FileWindowClose(lv_event_t *e) {
    lv_obj_t *file_window = lv_event_get_target(e);
    lv_obj_t *file_widget = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    auto *d = static_cast<file_data *>(lv_file_list_get_user_data(file_window));
    LV_LOG_USER("File: %s", d->current);
    lv_label_set_text(file_widget, d->current);
    free(lv_file_list_get_user_data(file_window));
    restore_group(lv_obj_get_parent(file_widget));
    lv_scr_load(lv_obj_get_screen(file_widget));
    lv_obj_scroll_to_view(static_cast<lv_obj_t *>(lv_event_get_user_data(e)), LV_ANIM_OFF);
}
}