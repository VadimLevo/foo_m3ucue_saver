# foo_m3ucue_saver

*[Read in Russian / Читать на русском](#русский)*

A foobar2000 component for exporting playlists (M3U, M3U8) and generating CUE sheets directly from the context menu with flexible path and tagging options.

![CUE Dialog Screenshot](screenshot.png)

## Features
* **M3U / M3U8 Export:** Save selected tracks as standard M3U or M3U8 playlists (includes `#EXTINF` metadata support).
* **CUE Sheet Generation:** Generate CUE sheets on the fly with a custom dialog to edit global tags (Album, Artist, Date, Genre, Comment) before saving.
* **Smart Filenames:** Customize the output filename using standard foobar2000 title formatting scripts (e.g., `%artist% - %album%`).
* **Flexible Save Locations:** 
  * Save directly in the source files' directory.
  * Save to a predefined custom directory.
  * Prompt for a folder using a standard Windows dialog.
* **Advanced Path Handling:** Supports writing paths as absolute, strictly relative (calculated automatically based on save location), or filename-only.

## Usage
Select one or more tracks in your playlist, right-click, and navigate to the context menu (by default, it should be under the main context menu or `Utils`). Select **Save as M3U**, **Save as M3U8 (EXTINF)**, or **Save as CUE Sheet...**. Configuration for default behaviors and paths can be found in the foobar2000 Advanced Preferences.

---

<a id="русский"></a>
# foo_m3ucue_saver (Русский)

Компонент для foobar2000, предназначенный для экспорта плейлистов (M3U, M3U8) и генерации CUE-файлов (CUE sheet) прямо из контекстного меню с гибкими настройками путей, имен и тегов.

## Основные возможности
* **Экспорт M3U / M3U8:** Сохранение выбранных треков в стандартные плейлисты (с поддержкой записи метаданных `#EXTINF`).
* **Генерация CUE файлов:** Удобное диалоговое окно при экспорте CUE для быстрого редактирования глобальных тегов релиза (Альбом, Артист, Год, Жанр, Комментарий) без изменения исходных файлов.
* **Умные имена файлов:** Настройка шаблона имени сохраняемого файла с использованием синтаксиса foobar2000 (например, `%artist% - %album%`).
* **Выбор места сохранения:** 
  * В общую папку с исходными треками.
  * В заранее заданную директорию.
  * Выбор папки вручную через стандартное окно Windows.
* **Обработка путей внутри плейлистов:** Поддержка записи абсолютных путей, вычисление корректных относительных путей (от плейлиста до трека) или запись только имен файлов.

## Использование
Выделите нужные треки в плейлисте, нажмите правую кнопку мыши и выберите соответствующий пункт в контекстном меню: **Save as M3U**, **Save as M3U8 (EXTINF)** или **Save as CUE Sheet...**. Поведение по умолчанию, шаблоны имен и пути настраиваются в настройках foobar2000.