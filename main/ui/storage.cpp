#include "ui/storage.h"
#include "ui/menu.h"
#include "ui/device_group.h"
#include "ui/widgets/file_list/file_list.h"
#include "ui/style.h"
#include "hw_init.h"

static void StorageExit(lv_event_t *e) {
    lv_obj_del(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

lv_obj_t *storage_menu(){
    if (lvgl_lock(-1)) {

        auto config = ImageConfig();

        new_group();
        lv_obj_t *cont_flex = lv_file_list_create(lv_scr_act());
        lv_file_list_icon_style(cont_flex, &icon_style);

        StorageInfo info = get_board()->storageInfo();

        if(info.name){
            lv_obj_t *name_btn = lv_file_list_add(cont_flex, nullptr);
            lv_obj_t *name_label = lv_label_create(name_btn);
            lv_label_set_text(name_label, info.name);
        }
        lv_obj_t *type_btn = lv_file_list_add(cont_flex, nullptr);
        lv_obj_t *type_label = lv_label_create(type_btn);
        switch(info.type){

            case StorageType_None:
                lv_label_set_text(type_label, "None");
                break;
            case StorageType_SPI:
                lv_label_set_text(type_label, "Internal Flash");
                break;
            case StorageType_SDIO:
                lv_label_set_text(type_label, "SDIO");
                break;
            case StorageType_MMC:
                lv_label_set_text(type_label, "MMC");
                break;
            case StorageType_SD:
                lv_label_set_text(type_label, "SD");
                break;
            case StorageType_SDHC:
                lv_label_set_text(type_label, "SDHC/SDXC");
                break;
        }
        lv_obj_t *speed_btn = lv_file_list_add(cont_flex, nullptr);
        lv_obj_t *speed_label = lv_label_create(speed_btn);
        lv_label_set_text_fmt(speed_label, "%.0fMhz", info.speed);

        lv_obj_t *size_btn = lv_file_list_add(cont_flex, nullptr);
        lv_obj_t *size_label = lv_label_create(size_btn);
        lv_label_set_text_fmt(size_label, "%.2f/%.2f", (double)(info.total_bytes-info.free_bytes)/1000/1000, (double)(info.total_bytes)/1000/1000);

        lv_obj_t *format_btn = lv_file_list_add(cont_flex, nullptr);
        lv_obj_t *format_label = lv_label_create(format_btn);
        lv_label_set_text(format_label, "Format Storage");

        lv_obj_t *exit_button = lv_file_list_add(cont_flex, "\ue5c9");
        lv_obj_t *exit = lv_obj_create(exit_button);
        lv_obj_add_flag(exit, LV_OBJ_FLAG_HIDDEN);
        lv_group_remove_obj(exit);

        lv_obj_add_event_cb(exit, StorageExit, LV_EVENT_CLICKED, cont_flex);

        lvgl_unlock();
        return cont_flex;
    }
    return nullptr;
}