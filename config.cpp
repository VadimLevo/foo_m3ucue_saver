#include "pch.h"
#include "config.h"

// Уникальные GUID для каждой настройки (сгенерированы специально для твоего компонента)
static const GUID guid_cfg_m3u_save_mode = { 0x1a2b3c4d, 0x5e6f, 0x7a8b, { 0x9c, 0x0d, 0x1e, 0x2f, 0x3a, 0x4b, 0x5c, 0x6d } };
static const GUID guid_cfg_m3u_path_mode = { 0x2b3c4d5e, 0x6f7a, 0x8b9c, { 0x0d, 0x1e, 0x2f, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e } };
static const GUID guid_cfg_m3u_filename_tpl = { 0x3c4d5e6f, 0x7a8b, 0x9c0d, { 0x1e, 0x2f, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f } };
static const GUID guid_cfg_m3u_extension = { 0x4d5e6f7a, 0x8b9c, 0x0d1e, { 0x2f, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90 } };
static const GUID guid_cfg_m3u_custom_dir = { 0x5e6f7a8b, 0x9c0d, 0x1e2f, { 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90, 0xa1 } };

static const GUID guid_cfg_cue_save_mode = { 0x6f7a8b9c, 0x0d1e, 0x2f3a, { 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90, 0xa1, 0xb2 } };
static const GUID guid_cfg_cue_path_mode = { 0x7a8b9c0d, 0x1e2f, 0x3a4b, { 0x5c, 0x6d, 0x7e, 0x8f, 0x90, 0xa1, 0xb2, 0xc3 } };
static const GUID guid_cfg_cue_filename_tpl = { 0x8b9c0d1e, 0x2f3a, 0x4b5c, { 0x6d, 0x7e, 0x8f, 0x90, 0xa1, 0xb2, 0xc3, 0xd4 } };
static const GUID guid_cfg_cue_custom_dir = { 0x9c0d1e2f, 0x3a4b, 0x5c6d, { 0x7e, 0x8f, 0x90, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5 } };


// Инициализация переменных значениями по умолчанию
cfg_int cfg_m3u_save_mode(guid_cfg_m3u_save_mode, 0); // По умолчанию "в папке с файлами"
cfg_int cfg_m3u_path_mode(guid_cfg_m3u_path_mode, 1); // По умолчанию "относительный"
cfg_string cfg_m3u_filename_tpl(guid_cfg_m3u_filename_tpl, "%artist% - %album%");
cfg_int cfg_m3u_extension(guid_cfg_m3u_extension, 1); // По умолчанию .m3u8 (поддержка юникода)
cfg_string cfg_m3u_custom_dir(guid_cfg_m3u_custom_dir, "C:\\");

cfg_int cfg_cue_save_mode(guid_cfg_cue_save_mode, 0);
cfg_int cfg_cue_path_mode(guid_cfg_cue_path_mode, 0); // По умолчанию "короткий путь"
cfg_string cfg_cue_filename_tpl(guid_cfg_cue_filename_tpl, "%artist% - %album%");
cfg_string cfg_cue_custom_dir(guid_cfg_cue_custom_dir, "C:\\");
static const GUID guid_cfg_show_success_msg = { 0x11223344, 0x5566, 0x7788, { 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00 } };
cfg_bool cfg_show_success_msg(guid_cfg_show_success_msg, true);