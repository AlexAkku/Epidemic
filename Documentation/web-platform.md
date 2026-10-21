# WebAssembly и полный ASCENT 0.1.20

Разработка движка ведётся в `EpidemicDev/Epidemic`, ветка `1.0.1`.
Браузерная цель использует полный C++ код демо: меню, комикс, мир, персонажей,
UI, бой, прокачку и профиль. Диагностическая геометрия остаётся отдельным
примером `ascent.html`; полный игровой запуск — `ascent-game.html`.
Установленный игровой SDK `Engine/` остаётся 1.0.0. Сборка Web его не меняет.

## Воспроизводимая сборка

Нужны CMake, Ninja, Node, Emscripten **6.0.13**, Python с Pillow и FFmpeg
в PATH. SDK установлен локально в `.ai/tools/emsdk`; для новой машины:

```powershell
git clone https://github.com/emscripten-core/emsdk.git .ai/tools/emsdk
& .ai/tools/emsdk/emsdk.ps1 install 6.0.13
& .ai/tools/emsdk/emsdk.ps1 activate 6.0.13
```

Из корня игрового проекта:

```powershell
python -m pip install Pillow
& Tools/Build-GameWeb.ps1 -Serve -Port 8091
```

`-Python C:/path/python.exe` выбирает Python с Pillow;
`-EmsdkPath C:/tools/emsdk` выбирает другой SDK. Скрипт не вызывает desktop
Build-Game/Build-Engine и не устанавливает расширения в `Engine/`.

Результат — `WebBuild/Ascent-0.1.20/`, адрес при указанном порте:
`http://127.0.0.1:8091/`. Сервер слушает loopback, Ctrl+C его останавливает.
Для публикации раздайте содержимое этой папки по HTTP(S) с MIME
`application/wasm`; `file://` не подходит. Windows EXE/DLL там не требуются.
Изолированный compiler cache — `build/web-release`, внутри движка.

Низкоуровневый запуск из корня движка:

```powershell
& tools/Build-Web.ps1 -AscentSource E:/ascent_game/Game -Serve
```

Без `-AscentSource` строятся платформенные примеры и WASM-контракты.

## Runtime и ресурсы

- RAF вызывает существующий Application::Tick; браузерный Host владеет
  callback-ами после возврата Run. Начальный focus учитывает уже активный canvas.
- Полный 2D Presentation использует браузерный Canvas2D под тем же Framework
  API. Примитивы, спрайты, трансформации, clipping, текст и эффекты идут
  командным пакетом за кадр. Это не реализация RHI v2: отдельный WebGL2
  backend текущего RHI по-прежнему поддерживает только clear-screen.
- Атлас 4096×12288 экспортируется тремя страницами высотой до 4096;
  спрайт, пересекающий границу страницы, рисуется частями. Исходные PNG и
  sprite metadata не меняются. Все страницы загружаются перед стартом игры.
- `Tools/Export-WebAssets.py` читает те же RC-каталоги, что desktop build.
  Длинные WAV экспортируются в Ogg Vorbis, короткие SFX остаются WAV.
  Идентификаторы ресурсов сохраняются в `ascent-assets.json`.
- Нативные шрифты Windows не распространяются. Включён Noto Sans с кириллицей,
  исходный файл и OFL находятся в `Game/Assets/Fonts`.
- Стартовая кнопка даёт user gesture для AudioContext. SFX имеют лимит
  голосов и cooldown, музыка/комикс используют media cursor; FFT питает
  существующие импульсы меню. Длинное audio не декодируется целиком в WASM.
- `/saves` монтируется в IDBFS до создания Game. Профиль и настройки
  восстанавливаются из IndexedDB. Статус «Сохранено» появляется после
  успешного sync; ошибки записи видны отдельно. Не закрывайте вкладку,
  пока показано «Сохранение…». Данные привязаны к origin браузера; очистка
  данных сайта или приватный режим не дают гарантий сохранности.
- FilePackage вне `/saves` остаётся MEMFS. Это не постоянное хранилище.

## Отличия браузера

Игровые механики и контент 0.1.20 не добавлялись и не заменялись.
Нормальная сборка не включает диагностическую неуязвимость.
Экран растягивается под viewport/DPR; fullscreen подчиняется браузеру,
эксклюзивный Windows fullscreen отсутствует. Управление рассчитано на
клавиатуру и мышь, touch/gamepad в этом переносе не добавлялись.

Gaussian blur использует Canvas filter, contour — SVG convolution,
perspective и displacement — разбитую на треугольники сетку. Они сохраняют
смысл эффектов, но не обещают попиксельного совпадения с Direct2D.
Текст использует другой шрифт; переносы строк могут немного отличаться.

Это поставка небольшой полной 2D игры, а не квалификация платформы на
3 ГБ или mobile. Нет offline/service-worker режима, HTTP Range pipeline,
budgeted zone streaming, Web 3D renderer и установленного Web SDK.
Scheduler в single-thread Web выполняет задачи inline: длительную работу
нельзя отправлять туда в надежде получить фоновый worker.
ETW/exact tracing Auditor остаются Windows-инструментами.

## Адресные проверки

21.10.2026: сборка WASM и Node-контракты — PASS; Framework Presentation
Release и Runtime PresentationTests — PASS; архитектурный self-test с JS —
PASS. На автономной поставке в Edge 155 прошёл прогон с обычной скоростью
комикса: все шесть страниц и логотип, переход в мир, восстановление громкости
после reload, язык, движение/прыжок, удержание и tap Tab, пауза, семь локаций,
десять типов врагов, пять активных боссов, resize и blur. Отдельно проверены
выход, остановка RAF/audio и запись профиля после закрывающего тика.
Это адресный прогон, не прохождение всей демки и не mobile/browser matrix.

`Game/Tests/web_smoke.cjs` запускается Node с пакетом Playwright и установленным
Edge (или `BROWSER_CHANNEL=chrome`). `ASCENT_WEB_URL` выбирает сервер.
Он проверяет настоящий UI, сохранение громкости через reload, локализацию,
полную звуковую шкалу комикса, колоду, локации, боссов и выход через существующие
UI и консоль разработчика. Профиль теста изолирован браузерным context.
`ASCENT_WEB_SCREENSHOTS` необязателен и должен указывать ignored каталог.

Нативные проверки: Framework Presentation build и Runtime PresentationTests.
Архитектурный скан учитывает исполняемые JS backend-файлы; self-test включает
положительный JS fixture и запрет импорта верхнего слоя из Runtime.
Штатный Verify-Changed всё ещё использует Windows compilation database и
не квалифицирует Web-only translation units. Скрипт не ослаблялся; полная
смешанная статическая квалификация остаётся задачей tooling.

[Актуализированный план](web-platform-roadmap.md).
Основа: [Emscripten main loop](https://emscripten.org/docs/api_reference/emscripten.h.html),
[IDBFS](https://emscripten.org/docs/api_reference/Filesystem-API.html),
[Canvas2D](https://developer.mozilla.org/en-US/docs/Web/API/CanvasRenderingContext2D).
