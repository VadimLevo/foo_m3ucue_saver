#pragma once
#include "pch.h"

class export_manager {
public:
    // Главные функции, которые мы будем вызывать из контекстного меню
    static void generate_m3u(metadb_handle_list_cref p_data, bool use_extinf);
    static void generate_cue_with_dialog(metadb_handle_list_cref p_data);

private:
    // --- Вспомогательные функции алгоритма ---

    // Поиск общей родительской папки для списка треков
    static pfc::string8 get_common_dir(metadb_handle_list_cref p_data);

    // Определение итоговой папки сохранения с учетом настроек (0, 1 или 2)
    static pfc::string8 get_target_dir(metadb_handle_list_cref p_data, int save_mode, const char* custom_dir);

    // Вычисление относительного пути (от папки плейлиста до файла трека)
    static pfc::string8 make_relative_path(const char* playlist_dir, const char* track_path);

    // Генерация имени файла на основе пользовательского шаблона (например, %artist% - %album%)
    static pfc::string8 format_filename(metadb_handle_ptr track, const char* tpl, const file_info* custom_info = nullptr);
};