#pragma once

// --- Настройки M3U ---
// 0: сохранять в папке с файлами, 1: в указанной папке, 2: спрашивать
extern cfg_int cfg_m3u_save_mode;
// 0: короткий путь, 1: относительный, 2: полный
extern cfg_int cfg_m3u_path_mode;
// Шаблон имени файла (например, %artist% - %album%)
extern cfg_string cfg_m3u_filename_tpl;
// 0: .m3u, 1: .m3u8
extern cfg_int cfg_m3u_extension;
// Путь для режима "в указанной папке"
extern cfg_string cfg_m3u_custom_dir;


// --- Настройки CUE ---
// 0: сохранять в папке с файлами, 1: в указанной папке, 2: спрашивать
extern cfg_int cfg_cue_save_mode;
// 0: короткий путь (только имя файла), 1: полный путь
extern cfg_int cfg_cue_path_mode;
// Шаблон имени файла
extern cfg_string cfg_cue_filename_tpl;
// Путь для режима "в указанной папке"
extern cfg_string cfg_cue_custom_dir;
extern cfg_bool cfg_show_success_msg;