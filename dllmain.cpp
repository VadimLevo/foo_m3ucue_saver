#include "pch.h"

// Декларация информации о плагине (название, версия, описание)
DECLARE_COMPONENT_VERSION(
    "CUE & M3U Saver",
    "1.1.6",
    "Компонент для продвинутого сохранения плейлистов и CUE-файлов.\n"
    "Скомпилирован: " __DATE__
);

// Защита от переименования DLL (фубар любит, чтобы имя файла совпадало с задуманным)
VALIDATE_COMPONENT_FILENAME("foo_m3ucue_saver.dll");

// Инициализация WTL
CAppModule _Module;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        _Module.Init(NULL, hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        _Module.Term();
    }
    return TRUE;
}