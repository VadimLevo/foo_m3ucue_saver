#include "pch.h"
#include "resource.h"
#include "config.h"
#include <atldlgs.h>
#include <uxtheme.h>

#pragma comment(lib, "uxtheme.lib")

// ============================================================
//  Тёмная тема (кисти и вспомогательные функции)
// ============================================================

static HBRUSH g_hbrDarkBackground = NULL;
static HBRUSH g_hbrDarkEdit = NULL;

static bool IsDarkMode() {
    return ui_config_manager::g_is_dark_mode();
}

static HBRUSH GetDarkBackgroundBrush() {
    if (!g_hbrDarkBackground) g_hbrDarkBackground = CreateSolidBrush(RGB(45, 45, 45));
    return g_hbrDarkBackground;
}

static HBRUSH GetDarkEditBrush() {
    if (!g_hbrDarkEdit) g_hbrDarkEdit = CreateSolidBrush(RGB(30, 30, 30));
    return g_hbrDarkEdit;
}

void cue_m3u_prefs_cleanup_dark_brushes() {
    if (g_hbrDarkBackground) { DeleteObject(g_hbrDarkBackground); g_hbrDarkBackground = NULL; }
    if (g_hbrDarkEdit) { DeleteObject(g_hbrDarkEdit);       g_hbrDarkEdit = NULL; }
}

static BOOL CALLBACK EnumChildApplyDarkTheme(HWND hChild, LPARAM lParam) {
    (void)lParam;
    wchar_t cls[64] = { 0 };
    GetClassNameW(hChild, cls, 64);

    if (_wcsicmp(cls, L"Button") == 0) {
        LONG style = GetWindowLong(hChild, GWL_STYLE);
        LONG type = style & 0x0F;

        if (type == BS_AUTOCHECKBOX || type == BS_CHECKBOX ||
            type == BS_AUTO3STATE || type == BS_3STATE ||
            type == BS_GROUPBOX) {
            SetWindowTheme(hChild, L"", L"");
        }
        else {
            SetWindowTheme(hChild, L"DarkMode_Explorer", NULL);
        }
    }
    else if (_wcsicmp(cls, L"Edit") == 0 || _wcsicmp(cls, L"ComboBox") == 0) {
        SetWindowTheme(hChild, L"DarkMode_CFD", NULL);
    }
    else if (_wcsicmp(cls, L"SysListView32") == 0 || _wcsicmp(cls, L"msctls_trackbar32") == 0) {
        SetWindowTheme(hChild, L"DarkMode_Explorer", NULL);
    }
    else if (_wcsicmp(cls, L"Static") == 0) {
        SetWindowTheme(hChild, L"", L"");
    }
    return TRUE;
}

static void ApplyDarkThemeToDialog(HWND hDlg) {
    if (!hDlg) return;
    SetWindowTheme(hDlg, L"DarkMode_Explorer", NULL);
    EnumChildWindows(hDlg, EnumChildApplyDarkTheme, 0);
    RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

// ============================================================
//  Класс диалога настроек
// ============================================================

class CPreferencesDialog : public CDialogImpl<CPreferencesDialog>, public preferences_page_instance {
public:
    enum { IDD = IDD_PREFERENCES };

    CPreferencesDialog(preferences_page_callback::ptr callback) : m_callback(callback) {}

    ~CPreferencesDialog() {
        cue_m3u_prefs_cleanup_dark_brushes();
    }

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::dark_mode_supported;
        state |= preferences_state::resettable;
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
        MESSAGE_HANDLER(WM_CTLCOLORDLG, OnCtlColorDlg)
        MESSAGE_HANDLER(WM_CTLCOLORBTN, OnCtlColorBtn)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnCtlColorEdit)
        MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, OnCtlColorListBox)

        COMMAND_HANDLER_EX(IDC_COMBO_M3U_SAVE_MODE, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_COMBO_M3U_PATH_MODE, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_COMBO_M3U_EXT, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_EDIT_M3U_TPL, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_EDIT_M3U_DIR, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_BTN_M3U_DIR, BN_CLICKED, OnBtnM3uDir)

        COMMAND_HANDLER_EX(IDC_BTN_M3U_TPL_HELP, BN_CLICKED, OnBtnM3uTplHelp)
        COMMAND_HANDLER_EX(IDC_BTN_CUE_TPL_HELP, BN_CLICKED, OnBtnCueTplHelp)

        COMMAND_HANDLER_EX(IDC_COMBO_CUE_SAVE_MODE, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_COMBO_CUE_PATH_MODE, CBN_SELCHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_EDIT_CUE_TPL, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_EDIT_CUE_DIR, EN_CHANGE, OnUIChange)
        COMMAND_HANDLER_EX(IDC_BTN_CUE_DIR, BN_CLICKED, OnBtnCueDir)

        COMMAND_HANDLER_EX(IDC_CHECK_SHOW_SUCCESS, BN_CLICKED, OnUIChange)
    END_MSG_MAP()

private:
    preferences_page_callback::ptr m_callback;

    // Обработчики тёмной темы через сообщения WM_CTLCOLOR*
    LRESULT OnCtlColorDlg(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
        if (IsDarkMode()) {
            bHandled = TRUE;
            return (LRESULT)GetDarkBackgroundBrush();
        }
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnCtlColorBtn(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
        if (IsDarkMode()) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkMode(hdc, TRANSPARENT);
            bHandled = TRUE;
            return (LRESULT)GetDarkBackgroundBrush();
        }
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnCtlColorStatic(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
        if (IsDarkMode()) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkMode(hdc, TRANSPARENT);
            bHandled = TRUE;
            return (LRESULT)GetDarkBackgroundBrush();
        }
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnCtlColorEdit(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
        if (IsDarkMode()) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkColor(hdc, RGB(30, 30, 30));
            bHandled = TRUE;
            return (LRESULT)GetDarkEditBrush();
        }
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnCtlColorListBox(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
        if (IsDarkMode()) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkColor(hdc, RGB(30, 30, 30));
            bHandled = TRUE;
            return (LRESULT)GetDarkEditBrush();
        }
        bHandled = FALSE;
        return 0;
    }

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

        if (IsDarkMode()) {
            ApplyDarkThemeToDialog(m_hWnd);
        }

        return TRUE;
    }

    void OnUIChange(UINT uNotifyCode, int nID, CWindow wndCtl) {
        OnChanged();
    }

    void OnBtnM3uDir(UINT uNotifyCode, int nID, CWindow wndCtl) {
        CFolderDialog dlg(m_hWnd, L"Select custom folder for M3U", BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
        if (dlg.DoModal() == IDOK) {
            pfc::string8 dir = pfc::stringcvt::string_utf8_from_os(dlg.GetFolderPath()).get_ptr();
            uSetDlgItemText(*this, IDC_EDIT_M3U_DIR, dir);
            OnChanged();
        }
    }

    void OnBtnCueDir(UINT uNotifyCode, int nID, CWindow wndCtl) {
        CFolderDialog dlg(m_hWnd, L"Select custom folder for CUE", BIF_RETURNONLYFSDIRS | BIF_USENEWUI);
        if (dlg.DoModal() == IDOK) {
            pfc::string8 dir = pfc::stringcvt::string_utf8_from_os(dlg.GetFolderPath()).get_ptr();
            uSetDlgItemText(*this, IDC_EDIT_CUE_DIR, dir);
            OnChanged();
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

    void ShowPatternMenu(int btn_id, int edit_id) {
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, 1, L"%artist%");
        AppendMenu(hMenu, MF_STRING, 2, L"%album artist%");
        AppendMenu(hMenu, MF_STRING, 3, L"%album%");
        AppendMenu(hMenu, MF_STRING, 4, L"%date%");
        AppendMenu(hMenu, MF_STRING, 8, L"%year%");
        AppendMenu(hMenu, MF_STRING, 5, L"%genre%");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, 6, L"%tracknumber%");
        AppendMenu(hMenu, MF_STRING, 7, L"%title%");

        RECT rc;
        ::GetWindowRect(GetDlgItem(btn_id), &rc);

        int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_TOPALIGN,
            rc.right, rc.bottom, 0, m_hWnd, NULL);
        DestroyMenu(hMenu);

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

            ::SetFocus(GetDlgItem(edit_id));
            SendDlgItemMessage(edit_id, EM_REPLACESEL, TRUE, (LPARAM)insert_text);
            OnChanged();
        }
    }

    void OnBtnM3uTplHelp(UINT uNotifyCode, int nID, CWindow wndCtl) {
        ShowPatternMenu(IDC_BTN_M3U_TPL_HELP, IDC_EDIT_M3U_TPL);
    }

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
        static const GUID guid = { 0xa1b2c3d4, 0xe5f6, 0x7890,{ 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
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