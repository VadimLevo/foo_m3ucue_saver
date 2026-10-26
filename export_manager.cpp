#include "pch.h"
#include "export_manager.h"
#include "config.h"
#include "resource.h"
#include <shlwapi.h>
#include <map>
#include <vector>

#pragma comment(lib, "shlwapi.lib")

// --- Вспомогательная функция: экранирование и очистка строк для CUE ---
static pfc::string8 escape_cue_string(const char* input) {
    pfc::string8 result = input;
    result.replace_char('\"', '\'');

    pfc::string8 cleaned;
    for (t_size i = 0; i < result.length(); ++i) {
        char c = result[i];
        if (c == '\r' || c == '\n') {
            cleaned.add_char(' ');
        }
        else {
            cleaned.add_char(c);
        }
    }
    return cleaned;
}

// --- Генерация уникального пути временного файла без коллизий ---
static pfc::string8 make_unique_temp_path(const pfc::string8& target_path) {
    pfc::string8 temp = target_path;
    temp.add_string(".tmp_");

    static volatile LONG s_counter = 0;
    LONG current_counter = InterlockedIncrement(&s_counter);

    char buf[128];
    sprintf_s(buf, sizeof(buf), "%llu_%lu_%ld",
        (unsigned long long)GetTickCount64(),
        (unsigned long)GetCurrentProcessId(),
        (long)current_counter);
    temp.add_string(buf);
    return temp;
}

// --- 1. Форматирование имени файла по шаблону ---
pfc::string8 export_manager::format_filename(metadb_handle_ptr track, const char* tpl, const file_info* custom_info) {
    titleformat_object::ptr script;
    titleformat_compiler::get()->compile_safe(script, tpl);

    pfc::string8 filename;

    if (custom_info != nullptr) {
        script->run_simple(track->get_location(), custom_info, filename);
    }
    else {
        track->format_title(nullptr, filename, script, nullptr);
    }

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

    pfc::string8 common_path = p_data.get_item(0)->get_path();
    if (strncmp(common_path.get_ptr(), "file://", 7) == 0) common_path = common_path.get_ptr() + 7;

    t_size slash_idx = common_path.find_last('\\');
    if (slash_idx == pfc::infinite_size) slash_idx = common_path.find_last('/');
    if (slash_idx != pfc::infinite_size) common_path.truncate(slash_idx + 1);

    for (t_size i = 1; i < p_data.get_count(); ++i) {
        pfc::string8 current_path = p_data.get_item(i)->get_path();
        if (strncmp(current_path.get_ptr(), "file://", 7) == 0) current_path = current_path.get_ptr() + 7;

        t_size match_len = 0;
        while (match_len < common_path.length() && match_len < current_path.length() &&
            tolower((unsigned char)common_path[match_len]) == tolower((unsigned char)current_path[match_len])) {
            match_len++;
        }

        common_path.truncate(match_len);

        slash_idx = common_path.find_last('\\');
        if (slash_idx == pfc::infinite_size) slash_idx = common_path.find_last('/');
        if (slash_idx != pfc::infinite_size) {
            common_path.truncate(slash_idx + 1);
        }
        else {
            common_path = "";
            break;
        }
    }

    if (common_path.is_empty()) {
        common_path = p_data.get_item(0)->get_path();
        if (strncmp(common_path.get_ptr(), "file://", 7) == 0) common_path = common_path.get_ptr() + 7;
        slash_idx = common_path.find_last('\\');
        if (slash_idx == pfc::infinite_size) slash_idx = common_path.find_last('/');
        if (slash_idx != pfc::infinite_size) common_path.truncate(slash_idx + 1);
    }

    return common_path;
}

// --- 3. Определение финальной папки для сохранения ---
pfc::string8 export_manager::get_target_dir(metadb_handle_list_cref p_data, int save_mode, const char* custom_dir) {
    if (save_mode == 0) {
        return get_common_dir(p_data);
    }
    else if (save_mode == 1) {
        pfc::string8 dir = custom_dir;
        if (dir.length() > 0) {
            char last_char = dir.get_ptr()[dir.length() - 1];
            if (last_char != '\\' && last_char != '/') dir.add_string("\\");
        }
        return dir;
    }
    else {
        CFolderDialog dlg(NULL, L"Select folder to save playlist / CUE", BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
        if (dlg.DoModal() == IDOK) {
            pfc::string8 dir = pfc::stringcvt::string_utf8_from_os(dlg.GetFolderPath()).get_ptr();
            if (dir.length() > 0) {
                char last_char = dir.get_ptr()[dir.length() - 1];
                if (last_char != '\\' && last_char != '/') dir.add_string("\\");
            }
            return dir;
        }
        return "";
    }
}

// --- 4. Вычисление относительного пути ---
pfc::string8 export_manager::make_relative_path(const char* base_dir, const char* track_path) {
    pfc::string8 track_clean = track_path;
    if (strncmp(track_clean.get_ptr(), "file://", 7) == 0) {
        track_clean = track_clean.get_ptr() + 7;
    }

    pfc::stringcvt::string_wide_from_utf8 w_base(base_dir);
    pfc::stringcvt::string_wide_from_utf8 w_track(track_clean.get_ptr());

    std::vector<wchar_t> relPath(MAX_PATH * 4);

    if (PathRelativePathToW(relPath.data(),
        w_base.get_ptr(), FILE_ATTRIBUTE_DIRECTORY,
        w_track.get_ptr(), FILE_ATTRIBUTE_NORMAL))
    {
        pfc::string8 result = pfc::stringcvt::string_utf8_from_wide(relPath.data()).get_ptr();
        if (strncmp(result.get_ptr(), ".\\", 2) == 0) {
            result = result.get_ptr() + 2;
        }
        return result;
    }

    return track_clean;
}

// --- Генерация плейлиста M3U / M3U8 с безопасной атомарной заменой ---
void export_manager::generate_m3u(metadb_handle_list_cref p_data, bool use_extinf) {
    if (p_data.get_count() == 0) return;

    pfc::string8 target_dir = get_target_dir(p_data, cfg_m3u_save_mode, cfg_m3u_custom_dir);
    if (target_dir.is_empty()) return;

    pfc::string8 playlist_name = format_filename(p_data.get_item(0), cfg_m3u_filename_tpl);
    if (cfg_m3u_extension == 1) playlist_name.add_string(".m3u8");
    else playlist_name.add_string(".m3u");

    pfc::string8 full_path = target_dir;
    full_path.add_string(playlist_name);

    pfc::string8 temp_path = make_unique_temp_path(full_path);

    pfc::stringcvt::string_wide_from_utf8 w_temp_path(temp_path.get_ptr());
    pfc::stringcvt::string_wide_from_utf8 w_full_path(full_path.get_ptr());

    FILE* f = nullptr;
    errno_t err = _wfopen_s(&f, w_temp_path.get_ptr(), L"wt, ccs=UTF-8");

    if (err != 0 || f == nullptr) {
        popup_message::g_show("Не удалось создать временный файл. Проверьте права доступа к папке.", "Ошибка");
        return;
    }

    bool write_failed = false;

    if (use_extinf) {
        if (fwprintf(f, L"#EXTM3U\n") < 0) write_failed = true;
    }

    titleformat_object::ptr script_extinf;
    titleformat_compiler::get()->compile_safe(script_extinf, "%artist% - %title%");

    titleformat_object::ptr script_length;
    titleformat_compiler::get()->compile_safe(script_length, "%length_seconds%");

    pfc::string8 source_common_dir = get_common_dir(p_data);

    for (t_size i = 0; i < p_data.get_count() && !write_failed; ++i) {
        metadb_handle_ptr track = p_data.get_item(i);

        if (use_extinf) {
            pfc::string8 extinf_text, length_text;
            track->format_title(nullptr, extinf_text, script_extinf, nullptr);
            track->format_title(nullptr, length_text, script_length, nullptr);

            int length_sec = atoi(length_text.get_ptr());
            if (length_sec == 0) length_sec = -1;

            pfc::stringcvt::string_wide_from_utf8 w_extinf(extinf_text.get_ptr());
            if (fwprintf(f, L"#EXTINF:%d,%s\n", length_sec, w_extinf.get_ptr()) < 0) write_failed = true;
        }

        pfc::string8 track_path = track->get_path();
        t_size pipe_idx = track_path.find_first('|');
        if (pipe_idx != pfc::infinite_size) track_path.truncate(pipe_idx);

        pfc::string8 out_path;

        if (cfg_m3u_path_mode == 0) {
            out_path = make_relative_path(source_common_dir.get_ptr(), track_path.get_ptr());
        }
        else if (cfg_m3u_path_mode == 1) {
            out_path = make_relative_path(target_dir.get_ptr(), track_path.get_ptr());
        }
        else {
            out_path = track_path;
            if (strncmp(out_path.get_ptr(), "file://", 7) == 0) {
                out_path = out_path.get_ptr() + 7;
            }
        }

        pfc::stringcvt::string_wide_from_utf8 w_out_path(out_path.get_ptr());
        if (fwprintf(f, L"%s\n", w_out_path.get_ptr()) < 0) write_failed = true;
    }

    if (ferror(f)) write_failed = true;
    if (fclose(f) != 0) write_failed = true;
    f = nullptr;

    if (write_failed) {
        _wremove(w_temp_path.get_ptr());
        popup_message::g_show("Произошла ошибка при записи файла (возможно, не хватает места на диске).", "Ошибка");
        return;
    }

    if (!MoveFileExW(w_temp_path.get_ptr(), w_full_path.get_ptr(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        _wremove(w_temp_path.get_ptr());
        popup_message::g_show("Не удалось финализировать сохранение плейлиста.", "Ошибка");
        return;
    }

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

        pfc::string8 target_dir = get_target_dir(p_data, cfg_cue_save_mode, cfg_cue_custom_dir);
        if (target_dir.is_empty()) return;

        metadb_handle_ptr first_track = p_data.get_item(0);
        file_info_impl info;
        first_track->get_info(info);

        auto set_or_remove_meta = [&info](const char* field, const pfc::string8& val) {
            if (val.is_empty()) {
                info.meta_remove_field(field);
            }
            else {
                info.meta_set(field, val);
            }
            };

        set_or_remove_meta("ALBUM ARTIST", dlg.m_artist);
        set_or_remove_meta("ARTIST", dlg.m_artist);
        set_or_remove_meta("ALBUM", dlg.m_album);
        set_or_remove_meta("DATE", dlg.m_date);
        set_or_remove_meta("GENRE", dlg.m_genre);
        set_or_remove_meta("COMMENT", dlg.m_comment);

        pfc::string8 cue_name = format_filename(first_track, cfg_cue_filename_tpl, &info);
        cue_name.add_string(".cue");

        pfc::string8 full_path = target_dir;
        full_path.add_string(cue_name);

        pfc::string8 temp_path = make_unique_temp_path(full_path);

        pfc::stringcvt::string_wide_from_utf8 w_temp_path(temp_path.get_ptr());
        pfc::stringcvt::string_wide_from_utf8 w_full_path(full_path.get_ptr());

        FILE* f = nullptr;
        errno_t err = _wfopen_s(&f, w_temp_path.get_ptr(), L"wt, ccs=UTF-8");

        if (err != 0 || f == nullptr) {
            popup_message::g_show("Не удалось создать временный CUE файл. Проверьте права доступа к папке.", "Ошибка");
            return;
        }

        bool write_failed = false;

        auto write_tag = [&](const wchar_t* prefix, pfc::string8& val, bool is_rem) {
            if (!val.is_empty() && !write_failed) {
                pfc::string8 escaped = escape_cue_string(val.get_ptr());
                pfc::stringcvt::string_wide_from_utf8 w_val(escaped.get_ptr());
                if (is_rem) {
                    if (fwprintf(f, L"REM %s \"%s\"\n", prefix, w_val.get_ptr()) < 0) write_failed = true;
                }
                else {
                    if (fwprintf(f, L"%s \"%s\"\n", prefix, w_val.get_ptr()) < 0) write_failed = true;
                }
            }
            };

        write_tag(L"GENRE", dlg.m_genre, true);
        write_tag(L"DATE", dlg.m_date, true);
        write_tag(L"COMMENT", dlg.m_comment, true);
        write_tag(L"PERFORMER", dlg.m_artist, false);
        write_tag(L"TITLE", dlg.m_album, false);

        bool is_cue_image = false;
        if (p_data.get_count() > 0) {
            pfc::string8 base_path0 = p_data.get_item(0)->get_path();
            t_size p0 = base_path0.find_first('|');
            if (p0 != pfc::infinite_size) base_path0.truncate(p0);

            is_cue_image = true;
            for (t_size i = 1; i < p_data.get_count(); ++i) {
                pfc::string8 base_path_i = p_data.get_item(i)->get_path();
                t_size pi = base_path_i.find_first('|');
                if (pi != pfc::infinite_size) base_path_i.truncate(pi);

                if (_stricmp(base_path0.get_ptr(), base_path_i.get_ptr()) != 0) {
                    is_cue_image = false;
                    break;
                }
            }
        }

        titleformat_object::ptr script_track_title;
        titleformat_compiler::get()->compile_safe(script_track_title, "[%title%]");

        titleformat_object::ptr script_track_artist;
        titleformat_compiler::get()->compile_safe(script_track_artist, "[%artist%]");

        if (is_cue_image && !write_failed) {
            pfc::string8 track_path = first_track->get_path();
            t_size pipe_idx = track_path.find_first('|');
            if (pipe_idx != pfc::infinite_size) track_path.truncate(pipe_idx);
            if (strncmp(track_path.get_ptr(), "file://", 7) == 0) track_path = track_path.get_ptr() + 7;

            t_size slash_idx = track_path.find_last('\\');
            if (slash_idx == pfc::infinite_size) slash_idx = track_path.find_last('/');

            pfc::string8 check_dir = track_path;
            if (slash_idx != pfc::infinite_size) {
                check_dir.truncate(slash_idx + 1);
            }
            else {
                check_dir = "";
            }

            pfc::string8 full_audio_path;
            titleformat_object::ptr script_path;
            titleformat_compiler::get()->compile_safe(script_path, "%path%");
            first_track->format_title(nullptr, full_audio_path, script_path, nullptr);
            if (strncmp(full_audio_path.get_ptr(), "file://", 7) == 0) full_audio_path = full_audio_path.get_ptr() + 7;
            t_size p_idx = full_audio_path.find_first('|');
            if (p_idx != pfc::infinite_size) full_audio_path.truncate(p_idx);

            std::map<int, pfc::string8> track_to_file_map;
            std::map<int, pfc::string8> track_to_index_map; // Сохранение оригинальных таймкодов INDEX
            pfc::string8 current_file_path = "";
            int current_track_idx = -1;

            t_size r_dot = full_audio_path.find_last('.');
            bool is_source_cue = (r_dot != pfc::infinite_size && _stricmp(full_audio_path.get_ptr() + r_dot, ".cue") == 0);

            if (is_source_cue) {
                pfc::stringcvt::string_wide_from_utf8 w_full_cue(full_audio_path.get_ptr());
                FILE* cue_in = nullptr;
                if (_wfopen_s(&cue_in, w_full_cue.get_ptr(), L"rt, ccs=UTF-8") == 0 && cue_in != nullptr) {
                    std::vector<wchar_t> line_buf(2048);
                    while (fgetws(line_buf.data(), (int)line_buf.size(), cue_in) != nullptr) {
                        wchar_t* ptr = line_buf.data();
                        while (*ptr == L' ' || *ptr == L'\t') ptr++;

                        if (_wcsnicmp(ptr, L"FILE ", 5) == 0) {
                            wchar_t* first_q = wcschr(ptr, L'\"');
                            if (first_q) {
                                wchar_t* second_q = wcschr(first_q + 1, L'\"');
                                if (second_q) {
                                    *second_q = L'\0';
                                    current_file_path = pfc::stringcvt::string_utf8_from_wide(first_q + 1).get_ptr();
                                }
                            }
                            else {
                                // FILE без кавычек: FILE track01.wav WAVE
                                wchar_t* start = ptr + 5;
                                while (*start == L' ' || *start == L'\t') start++;
                                wchar_t* end = start;
                                while (*end && *end != L' ' && *end != L'\t' && *end != L'\r' && *end != L'\n') end++;
                                wchar_t old_c = *end;
                                *end = L'\0';
                                current_file_path = pfc::stringcvt::string_utf8_from_wide(start).get_ptr();
                                *end = old_c;
                            }
                        }
                        else if (_wcsnicmp(ptr, L"TRACK ", 6) == 0) {
                            current_track_idx++;
                            if (!current_file_path.is_empty()) {
                                track_to_file_map[current_track_idx] = current_file_path;
                            }
                        }
                        else if (_wcsnicmp(ptr, L"INDEX ", 6) == 0) {
                            wchar_t* p = ptr + 6;
                            while (*p == L' ' || *p == L'\t') p++;
                            while (*p && *p != L' ' && *p != L'\t') p++; // пропуск типа индекса (например, 01)
                            while (*p == L' ' || *p == L'\t') p++;
                            if (*p) {
                                wchar_t* end = p;
                                while (*end && *end != L' ' && *end != L'\t' && *end != L'\r' && *end != L'\n') end++;
                                wchar_t old_c = *end;
                                *end = L'\0';
                                if (current_track_idx >= 0) {
                                    track_to_index_map[current_track_idx] = pfc::stringcvt::string_utf8_from_wide(p).get_ptr();
                                }
                                *end = old_c;
                            }
                        }
                    }
                    fclose(cue_in);
                    cue_in = nullptr;
                }
            }

            if (track_to_file_map.empty()) {
                t_size s_idx = full_audio_path.find_last('\\');
                if (s_idx == pfc::infinite_size) s_idx = full_audio_path.find_last('/');

                pfc::string8 default_fname;
                if (s_idx != pfc::infinite_size) {
                    default_fname = pfc::string8(full_audio_path.get_ptr() + s_idx + 1);
                }
                else {
                    default_fname = full_audio_path;
                }

                if (is_source_cue) {
                    pfc::string8 base_name;
                    base_name.set_string(default_fname.get_ptr(), default_fname.find_last('.'));
                    const wchar_t* candidate_exts[] = { L".flac", L".ape", L".wv", L".m4a", L".mp3", L".wav", L".ogg", L".dsf", L".dff" };
                    bool found = false;
                    for (auto ce : candidate_exts) {
                        pfc::string8 test_path = check_dir;
                        test_path.add_string(base_name);
                        pfc::stringcvt::string_utf8_from_wide u_ce(ce);
                        test_path.add_string(u_ce.get_ptr());

                        pfc::stringcvt::string_wide_from_utf8 w_test(test_path.get_ptr());
                        if (PathFileExistsW(w_test.get_ptr())) {
                            base_name.add_string(u_ce.get_ptr());
                            default_fname = base_name;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        fclose(f);
                        f = nullptr;
                        _wremove(w_temp_path.get_ptr());
                        popup_message::g_show("Не удалось определить реальный аудиофайл для образа.", "Ошибка экспорта");
                        return;
                    }
                }
                for (t_size i = 0; i < p_data.get_count(); ++i) {
                    track_to_file_map[(int)i] = default_fname;
                }
            }

            pfc::string8 last_file_path = "";
            double accumulated_seconds = 0.0;

            for (t_size i = 0; i < p_data.get_count() && !write_failed; ++i) {
                metadb_handle_ptr track = p_data.get_item(i);

                pfc::string8 track_file = track_to_file_map.count((int)i) ? track_to_file_map[(int)i] : track_to_file_map.begin()->second;

                if (track_file != last_file_path) {
                    last_file_path = track_file;
                    accumulated_seconds = 0.0;

                    pfc::string8 file_token_path;
                    if (cfg_cue_path_mode == 0) {
                        t_size slash_i = track_file.find_last('\\');
                        if (slash_i == pfc::infinite_size) slash_i = track_file.find_last('/');

                        if (slash_i != pfc::infinite_size) {
                            file_token_path = pfc::string8(track_file.get_ptr() + slash_i + 1);
                        }
                        else {
                            file_token_path = track_file;
                        }
                    }
                    else {
                        if (track_file.find_first('\\') == pfc::infinite_size && track_file.find_first(':') == pfc::infinite_size) {
                            file_token_path = check_dir;
                            file_token_path.add_string(track_file);
                        }
                        else {
                            file_token_path = track_file;
                        }
                    }

                    pfc::stringcvt::string_wide_from_utf8 w_audio_file(file_token_path.get_ptr());
                    if (fwprintf(f, L"FILE \"%s\" WAVE\n", w_audio_file.get_ptr()) < 0) write_failed = true;
                }

                pfc::string8 index_str;
                if (is_source_cue && track_to_index_map.count((int)i)) {
                    // Используем оригинальный таймкод из исходного CUE
                    index_str = track_to_index_map[(int)i];
                }
                else {
                    // Фоллбэк на расчет по длительности
                    int total_frames = (int)(accumulated_seconds * 75.0);
                    int minutes = total_frames / (75 * 60);
                    int seconds = (total_frames / 75) % 60;
                    int frames = total_frames % 75;

                    char index_buf[32];
                    sprintf_s(index_buf, sizeof(index_buf), "%02d:%02d:%02d", minutes, seconds, frames);
                    index_str = index_buf;
                }

                if (fwprintf(f, L"  TRACK %02d AUDIO\n", (int)(i + 1)) < 0) write_failed = true;

                pfc::string8 t_title, t_artist;
                track->format_title(nullptr, t_title, script_track_title, nullptr);
                track->format_title(nullptr, t_artist, script_track_artist, nullptr);

                if (!t_title.is_empty() && strcmp(t_title.get_ptr(), " ") != 0) {
                    pfc::string8 esc_title = escape_cue_string(t_title.get_ptr());
                    pfc::stringcvt::string_wide_from_utf8 w_title(esc_title.get_ptr());
                    if (fwprintf(f, L"    TITLE \"%s\"\n", w_title.get_ptr()) < 0) write_failed = true;
                }

                if (!t_artist.is_empty() && t_artist != dlg.m_artist) {
                    pfc::string8 esc_artist = escape_cue_string(t_artist.get_ptr());
                    pfc::stringcvt::string_wide_from_utf8 w_artist(esc_artist.get_ptr());
                    if (fwprintf(f, L"    PERFORMER \"%s\"\n", w_artist.get_ptr()) < 0) write_failed = true;
                }

                if (fwprintf(f, L"    INDEX 01 %s\n", pfc::stringcvt::string_wide_from_utf8(index_str.get_ptr()).get_ptr()) < 0) write_failed = true;

                accumulated_seconds += track->get_length();
            }
        }
        else if (!write_failed) {
            for (t_size i = 0; i < p_data.get_count() && !write_failed; ++i) {
                metadb_handle_ptr track = p_data.get_item(i);

                pfc::string8 track_path = track->get_path();
                t_size pipe_idx = track_path.find_first('|');
                if (pipe_idx != pfc::infinite_size) track_path.truncate(pipe_idx);
                if (strncmp(track_path.get_ptr(), "file://", 7) == 0) track_path = track_path.get_ptr() + 7;

                pfc::string8 out_path;

                if (cfg_cue_path_mode == 0) {
                    t_size slash_idx = track_path.find_last('\\');
                    if (slash_idx == pfc::infinite_size) slash_idx = track_path.find_last('/');

                    if (slash_idx != pfc::infinite_size) {
                        out_path = pfc::string8(track_path.get_ptr() + slash_idx + 1);
                    }
                    else {
                        out_path = track_path;
                    }
                }
                else {
                    out_path = track_path;
                }

                pfc::stringcvt::string_wide_from_utf8 w_out_path(out_path.get_ptr());

                if (fwprintf(f, L"FILE \"%s\" WAVE\n", w_out_path.get_ptr()) < 0) write_failed = true;
                if (fwprintf(f, L"  TRACK %02d AUDIO\n", (int)(i + 1)) < 0) write_failed = true;

                pfc::string8 t_title, t_artist;
                track->format_title(nullptr, t_title, script_track_title, nullptr);
                track->format_title(nullptr, t_artist, script_track_artist, nullptr);

                if (!t_title.is_empty() && strcmp(t_title.get_ptr(), " ") != 0) {
                    pfc::string8 esc_title = escape_cue_string(t_title.get_ptr());
                    pfc::stringcvt::string_wide_from_utf8 w_title(esc_title.get_ptr());
                    if (fwprintf(f, L"    TITLE \"%s\"\n", w_title.get_ptr()) < 0) write_failed = true;
                }

                if (!t_artist.is_empty() && t_artist != dlg.m_artist) {
                    pfc::string8 esc_artist = escape_cue_string(t_artist.get_ptr());
                    pfc::stringcvt::string_wide_from_utf8 w_artist(esc_artist.get_ptr());
                    if (fwprintf(f, L"    PERFORMER \"%s\"\n", w_artist.get_ptr()) < 0) write_failed = true;
                }

                if (fwprintf(f, L"    INDEX 01 00:00:00\n") < 0) write_failed = true;
            }
        }

        if (ferror(f)) write_failed = true;
        if (fclose(f) != 0) write_failed = true;
        f = nullptr;

        if (write_failed) {
            _wremove(w_temp_path.get_ptr());
            popup_message::g_show("Произошла ошибка при записи CUE файла. Частично записанный файл удален.", "Ошибка");
            return;
        }

        if (!MoveFileExW(w_temp_path.get_ptr(), w_full_path.get_ptr(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            _wremove(w_temp_path.get_ptr());
            popup_message::g_show("Не удалось финализировать сохранение CUE файла.", "Ошибка");
            return;
        }

        if (cfg_show_success_msg) {
            pfc::string8 success_msg = "CUE successfully saved:\n";
            success_msg.add_string(full_path);
            popup_message::g_show(success_msg.get_ptr(), "Done");
        }
    }
}