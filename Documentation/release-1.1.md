# Epidemic Engine 1.1

Дата: 21.10.2026. Версия CMake/SDK: **1.1.0**. Git tag: **1.1**.
Изменения подготовлены в ветке `1.0.1` и включены в `main`.

## Что вошло

- WebAssembly / Emscripten backend и интеграция полного Ascent 0.1.20;
  2D Presentation через Canvas2D, отдельный WebGL2 RHI-пример, ввод,
  ресурсы, звук, профиль IndexedDB и запуск через пользовательский жест.
- Web loader: logo, Start Game, общий и ресурсный прогресс, ограниченная
  очередь загрузки assets, предзагрузка SFX и явные ошибки загрузки.
  Версия движка в shell подставляется из CMake.
- Platform capabilities для display modes; Browser Host завершает loop/звук
  и вызывает hook выхода. Fullscreen завершается через браузерный API;
  для itch.io iframe предусмотрена явная ссылка возврата, без blind history.back().
- Canvas: ограниченные кеши цвета/текстовой раскладки, исправление baseline текста,
  уменьшение числа save/restore для примитивов; immutable cache для хешированных assets.
- Исправление удержания WebAudio music records и связанных audio nodes.
- Auditor: защита exact recorder от повторного входа, бюджет и отмена xperf этапов,
  видимый progress, обработка явно включённых out-of-tree исходников,
  именованные current/legacy сценарии с версией контента.
- Покадровый текстовый DEBUG выключен по умолчанию. Для включения задайте
  `EPIDEMIC_LOG_FRAME_EVENTS=1` до старта процесса; counters/probes независимы.
- Reusable 2D helpers: pivots, joints/anchors, mirroring и rig preview;
  базовые UI focus/hit-testing/localization; fixed-step/UI/media clocks.
- Руководство [создания игры для разработчика и ИИ-агента](game-authoring.md),
  [архив аудита](<Reports/Archive/Engine Audit 2026-10-21.md>) и [TODO](maintenance-todo.md).

## Совместимость и использование

Основная native-платформа: Windows x64 / C++23. Web backend экспериментальный,
собирается отдельным Emscripten toolchain; Windows EXE/DLL в браузере не используются.
Для новой установки SDK используйте package 1.1.0. SDK ABI остаётся 1;
платформенные static capabilities и новые header-only helpers не меняют layout
существующего `ApplicationOptions`.

Установленный SDK существующей Ascent остаётся Epidemic 1.0.0. Desktop и Web
Ascent развивались отдельно; релиз движка не синхронизирует и не пересобирает игру.

`Examples/MinimalGame --rig-preview` демонстрирует новый toolkit без ассетов игры.
Это пример, а не новый редактор или система игрового контента.

## Проверка и ограничения

Перед публикацией просмотрены изменённые web-пути, выполнена проверка diff и
локальных ссылок документации. По просьбе владельца **новых сборок, тестов и
игровых/профилировочных прогонов после аудита не было**. Бинарный SDK в этот
релиз не прикладывается; исходники доступны через GitHub source archives.

Исторический аудит содержит успешные Full web-прогоны и частичные нативные данные,
но Deep/exact/coverage/ASan не получили полной успешной квалификации.
Внесённые исправления не превращают прежние FAIL/TIMEOUT/INCOMPLETE в PASS.
В частности, устранение exact-таймаута ещё требуется подтвердить запуском.

CSS-развёрнутый cross-origin iframe не равен Fullscreen API: shell не может
автоматически менять CSS страницы-хоста без её сотрудничества. Возврат по ссылке
может вести на страницу автора, если браузер передал только origin в referrer.
Длительные memory/GPU проверки, мобильные браузеры, Firefox/Safari и полноценная
повторная проверка itch.io оболочки в этом релизном проходе не выполнялись.
