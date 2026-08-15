#include "pch.h"
#include "export_manager.h"

// 1. Создаем уникальный GUID для нашей новой папки в меню
static const GUID guid_m3ucue_group = { 0x8f2a1b3c, 0x4d5e, 0x6f7a, { 0x8b, 0x9c, 0x0d, 0x1e, 0x2f, 0x3a, 0x4b, 0x5c } };

// 2. Регистрируем выпадающее меню (popup) прямо в корне (root) контекстного меню
// Последний параметр (0) - это приоритет сортировки. 
static contextmenu_group_popup_factory g_my_group(guid_m3ucue_group, contextmenu_groups::root, "CUE & M3U Saver", 0);

class custom_export_context : public contextmenu_item_simple {
public:
    enum {
        cmd_m3u = 0,
        cmd_m3u_extinf,
        cmd_cue,
        cmd_total
    };

    unsigned get_num_items() override {
        return cmd_total;
    }

    void get_item_name(unsigned p_index, pfc::string_base& p_out) override {
        switch (p_index) {
        case cmd_m3u: p_out = "Save as M3U"; break;
        case cmd_m3u_extinf: p_out = "Save as M3U (EXTINF)"; break;
        case cmd_cue: p_out = "Save as CUE"; break;
        default: uBugCheck();
        }
    }

    // 3. УКАЗЫВАЕМ РОДИТЕЛЯ: Теперь наши команды лежат в нашей кастомной группе, а не в Legacy
    GUID get_parent() override {
        return guid_m3ucue_group;
    }

    // ВАЖНО: Функцию get_item_default_path мы полностью удалили!

    void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) override {
        switch (p_index) {
        case cmd_m3u:
            export_manager::generate_m3u(p_data, false);
            break;
        case cmd_m3u_extinf:
            export_manager::generate_m3u(p_data, true);
            break;
        case cmd_cue:
            export_manager::generate_cue_with_dialog(p_data);
            break;
        }
    }

    GUID get_item_guid(unsigned p_index) override {
        static const GUID guid_m3u = { 0x12a8b34c, 0x56d7, 0x4e8f, { 0x9a, 0x1b, 0x2c, 0x3d, 0x4e, 0x5f, 0x6a, 0x7b } };
        static const GUID guid_m3u_extinf = { 0x23b9c45d, 0x67e8, 0x5f90, { 0xab, 0x2c, 0x3d, 0x4e, 0x5f, 0x6a, 0x7b, 0x8c } };
        static const GUID guid_cue = { 0x34cad56e, 0x78f9, 0x60a1, { 0xbc, 0x3d, 0x4e, 0x5f, 0x6a, 0x7b, 0x8c, 0x9d } };

        switch (p_index) {
        case cmd_m3u: return guid_m3u;
        case cmd_m3u_extinf: return guid_m3u_extinf;
        case cmd_cue: return guid_cue;
        default: uBugCheck(); return pfc::guid_null;
        }
    }

    bool get_item_description(unsigned p_index, pfc::string_base& p_out) override {
        switch (p_index) {
        case cmd_m3u: p_out = "Save selected tracks to standard M3U"; return true;
        case cmd_m3u_extinf: p_out = "Save selected tracks to extended M3U"; return true;
        case cmd_cue: p_out = "Save selected tracks to CUE sheet"; return true;
        default: return false;
        }
    }
};

static contextmenu_item_factory_t<custom_export_context> g_custom_export_context_factory;