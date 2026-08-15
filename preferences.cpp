#include "pch.h"
#include "resource.h"
#include "config.h"
#include <atldlgs.h> // Системная библиотека WTL для диалога выбора папок

class CPreferencesDialog : public CDialogImpl<CPreferencesDialog>, public preferences_page_instance {
public:
    enum { IDD = IDD_PREFERENCES };

    CPreferencesDialog(preferences_page_callback::ptr callback) : m_callback(callback) {}

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable;
        if (HasChanged()) state |= preferences_state::changed;
        return state;
    }

    void apply() override {
        cfg_m3u_save_mode = SendDlgItemMessage(IDC_COMBO_M3U_SAVE_MODE, CB_GETCURSEL, 0, 0);
        cfg_m3u_path_mode = SendDlgItemMessage(IDC_COMBO_M3U_PATH_MODE, CB_GETCURSEL, 0, 0);
        cfg_m3u_extension = SendDlgItemMessage(IDC_COMBO_M3U_EXT, CB_GETCURSEL, 0, 0);

        pfc::string8 temp;
        uGetDlgItemText(*this, IDC_EDIT_M3U_TPL, temp);
        cfg_m3u_filename_tpl = temp;

        uGetDlgItemText(*this, IDC_EDIT_M3U_DIR, temp);
        cfg_m3u_custom_dir = temp;

        cfg_cue_save_mode = SendDlgItemMessage(IDC_COMBO_CUE_SAVE_MODE, CB_GETCURSEL, 0, 0);
        cfg_cue_path_mode = SendDlgItemMessage(IDC_COMBO_CUE_PATH_MODE, CB_GETCURSEL, 0, 0);

        uGetDlgItemText(*this, IDC_EDIT_CUE_TPL, temp);
        cfg_cue_filename_tpl = temp;

        uGetDlgItemText(*this, IDC_EDIT_CUE_DIR, temp);
        cfg_cue_custom_dir = temp;

        cfg_show_success_msg = uButton_GetCheck(*this, IDC_CHECK_SHOW_SUCCESS);

        OnChanged();
    }

    void reset() override {
        SendDlgItemMessage(IDC_COMBO_M3U_SAVE_MODE, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(IDC_COMBO_M3U_PATH_MODE, CB_SETCURSEL, 1, 0);
        SendDlgItemMessage(IDC_COMBO_M3U_EXT, CB_SETCURSEL, 1, 0);
        uSetDlgItemText(*this, IDC_EDIT_M3U_TPL, "%artist% - %album%");
        uSetDlgItemText(*this, IDC_EDIT_M3U_DIR, "C:\\");

        SendDlgItemMessage(IDC_COMBO_CUE_SAVE_MODE, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(IDC_COMBO_CUE_PATH_MODE, CB_SETCURSEL, 0, 0);
        uSetDlgItemText(*this, IDC_EDIT_CUE_TPL, "%artist% - %album%");
        uSetDlgItemText(*this, IDC_EDIT_CUE_DIR, "C:\\");

        uButton_SetCheck(*this, IDC_CHECK_SHOW_SUCCESS, true);

        OnChanged();
    }

    HWND get_wnd() override { return m_hWnd; }

    BEGIN_MSG_MAP(CPreferencesDialog)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_COMBO_M3U_SAVE_MODE, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_COMBO_M3U_PATH_MODE, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_COMBO_M3U_EXT, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_EDIT_M3U_TPL, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_EDIT_M3U_DIR, EN_CHANGE, OnUIChange) // Отслеживаем ручной ввод пути
        COMMAND_HANDLER_EX(IDC_BTN_M3U_DIR, BN_CLICKED, OnBtnM3uDir) // Клик по кнопке M3U

        COMMAND_HANDLER_EX(IDC_BTN_M3U_TPL_HELP, BN_CLICKED, OnBtnM3uTplHelp)
        COMMAND_HANDLER_EX(IDC_BTN_CUE_TPL_HELP, BN_CLICKED, OnBtnCueTplHelp)

        COMMAND_HANDLER_EX(IDC_COMBO_CUE_SAVE_MODE, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_COMBO_CUE_PATH_MODE, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_EDIT_CUE_TPL, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_EDIT_CUE_DIR, EN_CHANGE, OnUIChange) // Отслеживаем ручной ввод пути
        COMMAND_HANDLER_EX(IDC_BTN_CUE_DIR, BN_CLICKED, OnBtnCueDir) // Клик по кнопке CUE

        COMMAND_HANDLER_EX(IDC_CHECK_SHOW_SUCCESS, BN_CLICKED, OnUIChange)
    END_MSG_MAP()

private:
    preferences_page_callback::ptr m_callback;

    void AddComboItem(int id, const wchar_t* text) {
        SendDlgItemMessage(id, CB_ADDSTRING, 0, (LPARAM)text);
    }

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam) {
        AddComboItem(IDC_COMBO_M3U_SAVE_MODE, L"With source files");
        AddComboItem(IDC_COMBO_M3U_SAVE_MODE, L"Custom folder");
        AddComboItem(IDC_COMBO_M3U_SAVE_MODE, L"Always ask");

        AddComboItem(IDC_COMBO_M3U_PATH_MODE, L"Filename only");
        AddComboItem(IDC_COMBO_M3U_PATH_MODE, L"Relative");
        AddComboItem(IDC_COMBO_M3U_PATH_MODE, L"Absolute");

        AddComboItem(IDC_COMBO_M3U_EXT, L".m3u");
        AddComboItem(IDC_COMBO_M3U_EXT, L".m3u8");

        AddComboItem(IDC_COMBO_CUE_SAVE_MODE, L"With source files");
        AddComboItem(IDC_COMBO_CUE_SAVE_MODE, L"Custom folder");
        AddComboItem(IDC_COMBO_CUE_SAVE_MODE, L"Always ask");

        AddComboItem(IDC_COMBO_CUE_PATH_MODE, L"Filename only");
        AddComboItem(IDC_COMBO_CUE_PATH_MODE, L"Absolute");

        SendDlgItemMessage(IDC_COMBO_M3U_SAVE_MODE, CB_SETCURSEL, cfg_m3u_save_mode, 0);
        SendDlgItemMessage(IDC_COMBO_M3U_PATH_MODE, CB_SETCURSEL, cfg_m3u_path_mode, 0);
        SendDlgItemMessage(IDC_COMBO_M3U_EXT, CB_SETCURSEL, cfg_m3u_extension, 0);
        uSetDlgItemText(*this, IDC_EDIT_M3U_TPL, cfg_m3u_filename_tpl);
        uSetDlgItemText(*this, IDC_EDIT_M3U_DIR, cfg_m3u_custom_dir);

        SendDlgItemMessage(IDC_COMBO_CUE_SAVE_MODE, CB_SETCURSEL, cfg_cue_save_mode, 0);
        SendDlgItemMessage(IDC_COMBO_CUE_PATH_MODE, CB_SETCURSEL, cfg_cue_path_mode, 0);
        uSetDlgItemText(*this, IDC_EDIT_CUE_TPL, cfg_cue_filename_tpl);
        uSetDlgItemText(*this, IDC_EDIT_CUE_DIR, cfg_cue_custom_dir);

        uButton_SetCheck(*this, IDC_CHECK_SHOW_SUCCESS, cfg_show_success_msg);

        return TRUE;
    }

    void OnUIChange(UINT uNotifyCode, int nID, CWindow wndCtl) {
        OnChanged();
    }

    // Обработчик кнопки обзора для M3U
    void OnBtnM3uDir(UINT uNotifyCode, int nID, CWindow wndCtl) {
        CFolderDialog dlg(m_hWnd, L"Select custom folder for M3U", BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
        if (dlg.DoModal() == IDOK) {
            pfc::string8 dir = pfc::stringcvt::string_utf8_from_os(dlg.GetFolderPath()).get_ptr();
            uSetDlgItemText(*this, IDC_EDIT_M3U_DIR, dir);
            OnChanged(); // Включаем кнопку Apply
        }
    }

    // Обработчик кнопки обзора для CUE
    void OnBtnCueDir(UINT uNotifyCode, int nID, CWindow wndCtl) {
        CFolderDialog dlg(m_hWnd, L"Select custom folder for CUE", BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
        if (dlg.DoModal() == IDOK) {
            pfc::string8 dir = pfc::stringcvt::string_utf8_from_os(dlg.GetFolderPath()).get_ptr();
            uSetDlgItemText(*this, IDC_EDIT_CUE_DIR, dir);
            OnChanged(); // Включаем кнопку Apply
        }
    }

    bool HasChanged() {
        if (SendDlgItemMessage(IDC_COMBO_M3U_SAVE_MODE, CB_GETCURSEL, 0, 0) != cfg_m3u_save_mode) return true;
        if (SendDlgItemMessage(IDC_COMBO_M3U_PATH_MODE, CB_GETCURSEL, 0, 0) != cfg_m3u_path_mode) return true;
        if (SendDlgItemMessage(IDC_COMBO_M3U_EXT, CB_GETCURSEL, 0, 0) != cfg_m3u_extension) return true;
        if (SendDlgItemMessage(IDC_COMBO_CUE_SAVE_MODE, CB_GETCURSEL, 0, 0) != cfg_cue_save_mode) return true;
        if (SendDlgItemMessage(IDC_COMBO_CUE_PATH_MODE, CB_GETCURSEL, 0, 0) != cfg_cue_path_mode) return true;
        if (uButton_GetCheck(*this, IDC_CHECK_SHOW_SUCCESS) != cfg_show_success_msg) return true;

        pfc::string8 temp;
        uGetDlgItemText(*this, IDC_EDIT_M3U_TPL, temp);
        if (temp != cfg_m3u_filename_tpl) return true;

        uGetDlgItemText(*this, IDC_EDIT_M3U_DIR, temp);
        if (temp != cfg_m3u_custom_dir) return true;

        uGetDlgItemText(*this, IDC_EDIT_CUE_TPL, temp);
        if (temp != cfg_cue_filename_tpl) return true;

        uGetDlgItemText(*this, IDC_EDIT_CUE_DIR, temp);
        if (temp != cfg_cue_custom_dir) return true;

        return false;
    }

    // Универсальная функция показа меню и вставки текста
    void ShowPatternMenu(int btn_id, int edit_id) {
        // Создаем системное всплывающее меню
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, 1, L"%artist%");
        AppendMenu(hMenu, MF_STRING, 2, L"%album artist%");
        AppendMenu(hMenu, MF_STRING, 3, L"%album%");
        AppendMenu(hMenu, MF_STRING, 4, L"%date%");
        AppendMenu(hMenu, MF_STRING, 8, L"%year%");
        AppendMenu(hMenu, MF_STRING, 5, L"%genre%");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL); // Разделитель
        AppendMenu(hMenu, MF_STRING, 6, L"%tracknumber%");
        AppendMenu(hMenu, MF_STRING, 7, L"%title%");

        // Получаем координаты кнопки, чтобы показать меню прямо под ней
        RECT rc;
        ::GetWindowRect(GetDlgItem(btn_id), &rc);

        // Показываем меню и ждем выбора пользователя (возвращает ID выбранного пункта)
        int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_TOPALIGN,
            rc.right, rc.bottom, 0, m_hWnd, NULL);
        DestroyMenu(hMenu);

        // Если пользователь что-то выбрал (не кликнул мимо)
        if (cmd > 0) {
            const wchar_t* insert_text = L"";
            switch (cmd) {
            case 1: insert_text = L"%artist%"; break;
            case 2: insert_text = L"%album artist%"; break;
            case 3: insert_text = L"%album%"; break;
            case 4: insert_text = L"%date%"; break;
            case 5: insert_text = L"%genre%"; break;
            case 6: insert_text = L"%tracknumber%"; break;
            case 7: insert_text = L"%title%"; break;
            case 8: insert_text = L"%year%"; break;
            }

            // Возвращаем фокус текстовому полю (чтобы курсор мигал там, где был)
            ::SetFocus(GetDlgItem(edit_id));

            // Системная команда EM_REPLACESEL: вставляет текст на место курсора или заменяет выделенное
            SendDlgItemMessage(edit_id, EM_REPLACESEL, TRUE, (LPARAM)insert_text);

            OnChanged(); // Включаем кнопку Apply
        }
    }

    // Обработчик кнопки меню для M3U
    void OnBtnM3uTplHelp(UINT uNotifyCode, int nID, CWindow wndCtl) {
        ShowPatternMenu(IDC_BTN_M3U_TPL_HELP, IDC_EDIT_M3U_TPL);
    }

    // Обработчик кнопки меню для CUE
    void OnBtnCueTplHelp(UINT uNotifyCode, int nID, CWindow wndCtl) {
        ShowPatternMenu(IDC_BTN_CUE_TPL_HELP, IDC_EDIT_CUE_TPL);
    }

    void OnChanged() {
        m_callback->on_state_changed();
    }
};

class preferences_page_myimpl : public preferences_page_v3 {
public:
    const char* get_name() override { return "CUE & M3U Saver"; }
    GUID get_guid() override {
        static const GUID guid = { 0xa1b2c3d4, 0xe5f6, 0x7890, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
        return guid;
    }
    GUID get_parent_guid() override { return guid_tools; }

    preferences_page_instance::ptr instantiate(HWND parent, preferences_page_callback::ptr callback) override {
        auto p = new service_impl_t<CPreferencesDialog>(callback);
        p->Create(parent);
        return p;
    }
};

static preferences_page_factory_t<preferences_page_myimpl> g_pref_factory;