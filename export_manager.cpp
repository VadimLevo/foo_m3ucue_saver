#include "pch.h"
#include "export_manager.h"
#include "config.h"
#include "resource.h"
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

// --- 1. Форматирование имени файла по шаблону ---
pfc::string8 export_manager::format_filename(metadb_handle_ptr track, const char* tpl, const file_info* custom_info) {
    titleformat_object::ptr script;
    titleformat_compiler::get()->compile_safe(script, tpl);

    pfc::string8 filename;

    // НОВОЕ: Если переданы кастомные теги из интерфейса, используем их
    if (custom_info != nullptr) {
        script->run_simple(track->get_location(), custom_info, filename);
    }
    else {
        // Иначе читаем напрямую из metadb (поведение по умолчанию для M3U и т.д.)
        track->format_title(nullptr, filename, script, nullptr);
    }

    // --- Дальше вся твоя текущая очистка (без изменений) ---
    filename.replace_char(':', '-');
    filename.replace_char('/', '-');
    filename.replace_char('\\', '-');
    filename.replace_char('?', '-');
    filename.replace_char('*', '-');
    filename.replace_char('<', '[');
    filename.replace_char('>', ']');
    filename.replace_char('|', '-');
    filename.replace_char('\"', '\'');

    while (filename.length() > 0 &&
        (filename[filename.length() - 1] == ' ' || filename[filename.length() - 1] == '.')) {
        filename.truncate(filename.length() - 1);
    }

    return filename;
}

// --- 2. Поиск общей родительской папки для всех файлов ---
pfc::string8 export_manager::get_common_dir(metadb_handle_list_cref p_data) {
    if (p_data.get_count() == 0) return "";

    // Берем путь первого файла как базовый
    pfc::string8 common_path = p_data.get_item(0)->get_path();
    if (strncmp(common_path.get_ptr(), "file://", 7) == 0) common_path = common_path.get_ptr() + 7;

    t_size slash_idx = common_path.find_last('\\');
    if (slash_idx == pfc::infinite_size) slash_idx = common_path.find_last('/');
    if (slash_idx != pfc::infinite_size) common_path.truncate(slash_idx + 1);

    // Сравниваем базовый путь с путями всех остальных файлов
    for (t_size i = 1; i < p_data.get_count(); ++i) {
        pfc::string8 current_path = p_data.get_item(i)->get_path();
        if (strncmp(current_path.get_ptr(), "file://", 7) == 0) current_path = current_path.get_ptr() + 7;

        t_size match_len = 0;
        // Ищем длину совпадающей части пути (без учета регистра)
        while (match_len < common_path.length() && match_len < current_path.length() &&
            tolower(common_path[match_len]) == tolower(current_path[match_len])) {
            match_len++;
        }

        common_path.truncate(match_len);

        // Откатываемся до последнего целого слеша, чтобы не обрезать слово посередине
        slash_idx = common_path.find_last('\\');
        if (slash_idx == pfc::infinite_size) slash_idx = common_path.find_last('/');
        if (slash_idx != pfc::infinite_size) {
            common_path.truncate(slash_idx + 1);
        }
        else {
            common_path = ""; // Файлы на разных логических дисках (C: и D:)
            break;
        }
    }

    // Запасной план: если файлы на разных дисках, просто сохраняем в папку первого файла
    if (common_path.is_empty()) {
        common_path = p_data.get_item(0)->get_path();
        if (strncmp(common_path.get_ptr(), "file://", 7) == 0) common_path = common_path.get_ptr() + 7;
        slash_idx = common_path.find_last('\\');
        if (slash_idx == pfc::infinite_size) slash_idx = common_path.find_last('/');
        if (slash_idx != pfc::infinite_size) common_path.truncate(slash_idx + 1);
    }

    return common_path;
}

// --- 3. Определение финальной папки для сохранения (учитывая настройки) ---
pfc::string8 export_manager::get_target_dir(metadb_handle_list_cref p_data, int save_mode, const char* custom_dir) {
    if (save_mode == 0) {
        // Режим: В папке с файлами
        return get_common_dir(p_data);
    }
    else if (save_mode == 1) {
        // Режим: В указанной папке
        pfc::string8 dir = custom_dir;
        // Убеждаемся, что путь заканчивается слешем
        if (dir.length() > 0) {
            char last_char = dir.get_ptr()[dir.length() - 1];
            if (last_char != '\\' && last_char != '/') dir.add_string("\\");
        }
        return dir;
    }
    else {
        // Режим: Спрашивать (Вызываем стандартное диалоговое окно Windows)
        CFolderDialog dlg(NULL, L"Select folder to save playlist / CUE", BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
        if (dlg.DoModal() == IDOK) {
            pfc::string8 dir = pfc::stringcvt::string_utf8_from_os(dlg.GetFolderPath()).get_ptr();
            if (dir.length() > 0) {
                char last_char = dir.get_ptr()[dir.length() - 1];
                if (last_char != '\\' && last_char != '/') dir.add_string("\\");
            }
            return dir;
        }
        return ""; // Если пользователь нажал "Отмена"
    }
}

// --- 4. Вычисление относительного пути от базовой папки к треку ---
pfc::string8 export_manager::make_relative_path(const char* base_dir, const char* track_path) {
    pfc::string8 track_clean = track_path;
    if (strncmp(track_clean.get_ptr(), "file://", 7) == 0) {
        track_clean = track_clean.get_ptr() + 7;
    }

    wchar_t relPath[MAX_PATH];

    // API Windows работает с широкими строками
    pfc::stringcvt::string_wide_from_utf8 w_base(base_dir);
    pfc::stringcvt::string_wide_from_utf8 w_track(track_clean.get_ptr());

    // Магия системной библиотеки: сама считает точки и слеши (..\..\folder\file.flac)
    if (PathRelativePathToW(relPath,
        w_base.get_ptr(), FILE_ATTRIBUTE_DIRECTORY,
        w_track.get_ptr(), FILE_ATTRIBUTE_NORMAL))
    {
        pfc::string8 result = pfc::stringcvt::string_utf8_from_wide(relPath).get_ptr();
        // PathRelativePathTo часто добавляет ".\" в начале файла, уберем это для красоты
        if (strncmp(result.get_ptr(), ".\\", 2) == 0) {
            result = result.get_ptr() + 2;
        }
        return result;
    }

    // Если файлы на разных физических дисках (C: и D:), относительный путь построить нельзя — возвращаем полный
    return track_clean;
}

// --- Генерация плейлиста M3U / M3U8 ---
void export_manager::generate_m3u(metadb_handle_list_cref p_data, bool use_extinf) {
    if (p_data.get_count() == 0) return;

    // 1. Узнаем, куда сохранять
    pfc::string8 target_dir = get_target_dir(p_data, cfg_m3u_save_mode, cfg_m3u_custom_dir);
    if (target_dir.is_empty()) return;

    // 2. Формируем имя файла по шаблону
    pfc::string8 playlist_name = format_filename(p_data.get_item(0), cfg_m3u_filename_tpl);
    if (cfg_m3u_extension == 1) playlist_name.add_string(".m3u8");
    else playlist_name.add_string(".m3u");

    pfc::string8 full_path = target_dir;
    full_path.add_string(playlist_name);

    // 3. Открываем файл для записи (в кодировке UTF-8)
    pfc::stringcvt::string_wide_from_utf8 w_full_path(full_path);

    FILE* f = nullptr;
    errno_t err = _wfopen_s(&f, w_full_path.get_ptr(), L"wt, ccs=UTF-8");

    if (err != 0 || f == nullptr) {
        popup_message::g_show("Не удалось создать файл. Проверьте права доступа к папке.", "Ошибка");
        return;
    }

    // Заголовок для EXTINF
    if (use_extinf) {
        fwprintf(f, L"#EXTM3U\n");
    }

    // Компилируем скрипты для вытягивания тегов Артиста, Названия и Длительности
    titleformat_object::ptr script_extinf;
    titleformat_compiler::get()->compile_safe(script_extinf, "%artist% - %title%");

    titleformat_object::ptr script_length;
    titleformat_compiler::get()->compile_safe(script_length, "%length_seconds%");

    // Вычисляем общую папку исходников один раз для режима Filename Only
    pfc::string8 source_common_dir = get_common_dir(p_data);

    // 4. Проходимся по всем выделенным трекам
    for (t_size i = 0; i < p_data.get_count(); ++i) {
        metadb_handle_ptr track = p_data.get_item(i);

        // Пишем метаданные EXTINF
        if (use_extinf) {
            pfc::string8 extinf_text, length_text;
            track->format_title(nullptr, extinf_text, script_extinf, nullptr);
            track->format_title(nullptr, length_text, script_length, nullptr);

            int length_sec = atoi(length_text.get_ptr());
            if (length_sec == 0) length_sec = -1;

            pfc::stringcvt::string_wide_from_utf8 w_extinf(extinf_text);
            fwprintf(f, L"#EXTINF:%d,%s\n", length_sec, w_extinf.get_ptr());
        }

        // Вычисляем путь к треку согласно настройкам
        pfc::string8 track_path = track->get_path();
        pfc::string8 out_path;

        if (cfg_m3u_path_mode == 0) {
            // "Filename only" для M3U: путь относительно общей папки ИСХОДНИКОВ
            out_path = make_relative_path(source_common_dir.get_ptr(), track_path.get_ptr());
        }
        else if (cfg_m3u_path_mode == 1) {
            // Настоящий Relative: путь относительно папки ПЛЕЙЛИСТА
            out_path = make_relative_path(target_dir.get_ptr(), track_path.get_ptr());
        }
        else {
            // Полный (Абсолютный)
            out_path = track_path;
            if (strncmp(out_path.get_ptr(), "file://", 7) == 0) {
                out_path = out_path.get_ptr() + 7;
            }
        }

        // Записываем путь в файл
        pfc::stringcvt::string_wide_from_utf8 w_out_path(out_path);
        fwprintf(f, L"%s\n", w_out_path.get_ptr());
    }

    fclose(f);

    if (cfg_show_success_msg) {
        pfc::string8 success_msg = "Playlist successfully saved:\n";
        success_msg.add_string(full_path);
        popup_message::g_show(success_msg.get_ptr(), "Done");
    }
}


// --- Класс диалогового окна для тегов CUE ---
class CCueDialog : public CDialogImpl<CCueDialog> {
public:
    enum { IDD = IDD_CUE_DIALOG };

    pfc::string8 m_artist, m_album, m_date, m_genre, m_comment;
    metadb_handle_list_cref m_data;

    CCueDialog(metadb_handle_list_cref p_data) : m_data(p_data) {}

    BEGIN_MSG_MAP(CCueDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_ID_HANDLER_EX(IDOK, OnCloseCmd)
        COMMAND_ID_HANDLER_EX(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam) {
        if (m_data.get_count() > 0) {
            metadb_handle_ptr track = m_data.get_item(0);

            auto get_tag = [&](const char* tpl, pfc::string8& out) {
                titleformat_object::ptr script;
                titleformat_compiler::get()->compile_safe(script, tpl);
                track->format_title(nullptr, out, script, nullptr);
                };

            get_tag("[%album artist%]", m_artist);
            if (m_artist.is_empty()) get_tag("[%artist%]", m_artist);

            get_tag("[%album%]", m_album);
            get_tag("[%date%]", m_date);
            get_tag("[%genre%]", m_genre);
            get_tag("[%comment%]", m_comment);
        }

        uSetDlgItemText(*this, IDC_EDIT_CUE_ARTIST, m_artist);
        uSetDlgItemText(*this, IDC_EDIT_CUE_ALBUM, m_album);
        uSetDlgItemText(*this, IDC_EDIT_CUE_DATE, m_date);
        uSetDlgItemText(*this, IDC_EDIT_CUE_GENRE, m_genre);
        uSetDlgItemText(*this, IDC_EDIT_CUE_COMMENT, m_comment);

        CenterWindow(GetParent());
        return TRUE;
    }

    void OnCloseCmd(UINT uNotifyCode, int nID, CWindow wndCtl) {
        if (nID == IDOK) {
            uGetDlgItemText(*this, IDC_EDIT_CUE_ARTIST, m_artist);
            uGetDlgItemText(*this, IDC_EDIT_CUE_ALBUM, m_album);
            uGetDlgItemText(*this, IDC_EDIT_CUE_DATE, m_date);
            uGetDlgItemText(*this, IDC_EDIT_CUE_GENRE, m_genre);
            uGetDlgItemText(*this, IDC_EDIT_CUE_COMMENT, m_comment);
        }
        EndDialog(nID);
    }
};


// --- Вызов диалога и сборка CUE ---
void export_manager::generate_cue_with_dialog(metadb_handle_list_cref p_data) {
    if (p_data.get_count() == 0) return;

    CCueDialog dlg(p_data);
    if (dlg.DoModal(core_api::get_main_window()) == IDOK) {

        // 1. Узнаем, куда сохранять
        pfc::string8 target_dir = get_target_dir(p_data, cfg_cue_save_mode, cfg_cue_custom_dir);
        if (target_dir.is_empty()) return;

        // =========================================================
        // 2. ФОРМИРУЕМ ИМЯ ФАЙЛА С УЧЕТОМ ДАННЫХ ИЗ ДИАЛОГА (ВСТАВКА)
        // =========================================================
        metadb_handle_ptr first_track = p_data.get_item(0);
        file_info_impl info;
        first_track->get_info(info); // Копируем оригинальные теги первого трека

        // Лямбда для удобного добавления/удаления тегов
        auto set_or_remove_meta = [&info](const char* field, const pfc::string8& val) {
            if (val.is_empty()) {
                info.meta_remove_field(field);
            }
            else {
                info.meta_set(field, val);
            }
            };

        // Подменяем теги теми, что ввел/исправил пользователь в диалоге `dlg`
        set_or_remove_meta("ALBUM ARTIST", dlg.m_artist);
        set_or_remove_meta("ARTIST", dlg.m_artist); // Пишем в оба поля для надежности шаблонов
        set_or_remove_meta("ALBUM", dlg.m_album);
        set_or_remove_meta("DATE", dlg.m_date);
        set_or_remove_meta("GENRE", dlg.m_genre);
        set_or_remove_meta("COMMENT", dlg.m_comment);

        // Теперь вызываем format_filename, передавая ему наш подготовленный info
        pfc::string8 cue_name = format_filename(first_track, cfg_cue_filename_tpl, &info);
        // =========================================================

        cue_name.add_string(".cue");

        pfc::string8 full_path = target_dir;
        full_path.add_string(cue_name);

        // 3. Открываем файл (UTF-8)
        pfc::stringcvt::string_wide_from_utf8 w_full_path(full_path);
        FILE* f = nullptr;
        errno_t err = _wfopen_s(&f, w_full_path.get_ptr(), L"wt, ccs=UTF-8");

        if (err != 0 || f == nullptr) {
            popup_message::g_show("Не удалось создать CUE файл. Проверьте права доступа к папке.", "Ошибка");
            return;
        }

        auto write_tag = [&](const wchar_t* prefix, pfc::string8& val, bool is_rem) {
            if (!val.is_empty()) {
                pfc::stringcvt::string_wide_from_utf8 w_val(val);
                if (is_rem) fwprintf(f, L"REM %s \"%s\"\n", prefix, w_val.get_ptr());
                else fwprintf(f, L"%s \"%s\"\n", prefix, w_val.get_ptr());
            }
            };

        // Пишем глобальные теги альбома
        write_tag(L"GENRE", dlg.m_genre, true);
        write_tag(L"DATE", dlg.m_date, true);
        write_tag(L"COMMENT", dlg.m_comment, true);
        write_tag(L"PERFORMER", dlg.m_artist, false);
        write_tag(L"TITLE", dlg.m_album, false);

        // Подготавливаем скрипты тегов трека
        titleformat_object::ptr script_track_title;
        titleformat_compiler::get()->compile_safe(script_track_title, "[%title%]");

        titleformat_object::ptr script_track_artist;
        titleformat_compiler::get()->compile_safe(script_track_artist, "[%artist%]");

        // 4. Проходимся по трекам
        for (t_size i = 0; i < p_data.get_count(); ++i) {
            metadb_handle_ptr track = p_data.get_item(i);

            pfc::string8 track_path = track->get_path();
            pfc::string8 out_path;

            if (cfg_cue_path_mode == 0) {
                // "Filename only" для CUE: Строго только имя файла с расширением, БЕЗ каких-либо папок
                t_size slash_idx = track_path.find_last('\\');
                if (slash_idx == pfc::infinite_size) slash_idx = track_path.find_last('/');

                if (slash_idx != pfc::infinite_size) {
                    out_path = track_path.get_ptr() + slash_idx + 1;
                }
                else {
                    out_path = track_path;
                }
            }
            else {
                // "Absolute" (Полный путь)
                out_path = track_path;
                if (strncmp(out_path.get_ptr(), "file://", 7) == 0) out_path = out_path.get_ptr() + 7;
            }

            pfc::stringcvt::string_wide_from_utf8 w_out_path(out_path);

            fwprintf(f, L"FILE \"%s\" WAVE\n", w_out_path.get_ptr());
            fwprintf(f, L"  TRACK %02d AUDIO\n", (int)(i + 1));

            pfc::string8 t_title, t_artist;
            track->format_title(nullptr, t_title, script_track_title, nullptr);
            track->format_title(nullptr, t_artist, script_track_artist, nullptr);

            if (!t_title.is_empty()) {
                pfc::stringcvt::string_wide_from_utf8 w_title(t_title);
                fwprintf(f, L"    TITLE \"%s\"\n", w_title.get_ptr());
            }

            if (!t_artist.is_empty() && t_artist != dlg.m_artist) {
                pfc::stringcvt::string_wide_from_utf8 w_artist(t_artist);
                fwprintf(f, L"    PERFORMER \"%s\"\n", w_artist.get_ptr());
            }

            fwprintf(f, L"    INDEX 01 00:00:00\n");
        }

        fclose(f);

        if (cfg_show_success_msg) {
            pfc::string8 success_msg = "CUE successfully saved:\n";
            success_msg.add_string(full_path);
            popup_message::g_show(success_msg.get_ptr(), "Done");
        }
    }
}