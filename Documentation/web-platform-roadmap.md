# Epidemic Web Platform — план по реальному проекту

Актуализация 2026-10-21 по `EpidemicDev/Epidemic` 1.0.1 и исходникам ASCENT
0.1.20. Основание: переданный `Epidemic_Web_Platform_Roadmap.md` от этой даты.
В исходнике слово «Готово» описывает критерий приёмки каждого будущего этапа,
а не подтверждённую реализацию. Данный документ заменяет предположения аудита
архива фактическими границами проекта. Исходный файл пользователя не изменяется.

## Выводы по коду

1. `Application::Tick()` уже существует. Переписывать Windows Run-loop или
   вводить второй Application для браузера не нужно: RAF вызывает этот Tick.
2. Gameplay ASCENT отделён от desktop entry/rendering. Его можно подключить
   отдельной библиотекой, не добавляя Web-условия в gameplay. Обнаружена
   дополнительная зависимость combat → enemy rigs → header-only cutout math
   из Presentation: это переносимая математика, но её отдельный SDK-компонент
   нужно оформить при W2, не подтягивая Windows renderer для расчёта bounds.
3. Публичный Presentation façade в основном переносим; реализации renderer,
   host и audio зависят от Direct2D/DirectWrite/D3D11/Win32/XAudio.
   Web backend теперь реализует тот же 2D façade через Canvas2D; полный
   игровой target использует существующий код всех экранов и отрисовки.
4. AssetCatalog, ResourceManager и Streaming уже есть. Нужны адаптеры доставки
   и асинхронных завершений, а не второй набор менеджеров.
5. Игра загружает Windows resources/RCDATA. Web нужен отдельный экспорт
   ресурсов и отображение логических идентификаторов, а не копирование EXE/DLL.
6. Фактический `Game/Assets/Enemies/atlas.png`: **4096 × 12288**, 26 764 254
   байта PNG, **192 МиБ RGBA8 до mipmaps/копий**. Высота 12288 не должна
   считаться поддержанной без проверки `MAX_TEXTURE_SIZE`: нужны страницы
   атласа, укладывающиеся в заявленные limits, с сохранением sprite metadata.
   `menu.wav`: 148,98 с, 13 139 836 байт; decoded stereo float занимает
   26 279 424 байта. `comic.wav`: 60 с, decoded float 21 168 000 байт.
   Нужны разделение boot/menu/zone и streaming длинного audio. Это замеры
   исходных файлов, не замер браузерной памяти или сетевого startup.
7. `wchar_t`, размер `size_t`, системные шрифты, пути и файловые сохранения
   требуют явных переносимых контрактов. Особенно важны текст/кириллица.
8. Windows Auditor ETW/exact tracing не переносится в браузер. Браузеру нужны
   собственные frame/download/memory метрики; native проверки сохраняются.

## Реализация и границы

| Этап | Текущий объём | Оставшаяся приёмка |
| --- | --- | --- |
| W0 baseline | Изолированный Web cache; Windows 4/4; EXE игры 1.0.0 не изменён | Продолжать native regression при изменениях |
| W1 platform | Web preset, canvas lifecycle/input, RAF; WASM/Node/Edge smoke прошли | Матрица браузеров, lifecycle, смешанный compilation database для Verify-Changed |
| W2 graphics | Canvas2D Presentation: спрайты, текст, clipping, effects; WebGL2 RHI v1 отдельно | Pixel parity, performance, RHI v2/3D |
| W3 IO | Async fetch bootstrap, FilePackage и IDBFS profile | Общий async byte source/range/cancellation |
| W4 packs | RC → pages/Ogg/manifest export для настоящей игры | Cooker/dependencies/pack format |
| W5 streaming | Существующие менеджеры не дублируются | Подключение async IO/decode/upload с бюджетами |
| W6 cache/save | Профиль/настройки в IDBFS, hydrate до Game, статус sync | Quota/migration stress, versioned asset cache |
| W7 export | Автономный static WebBuild игры без EXE/DLL | Installed Web SDK, hosting/iframe qualification |
| W8 real game | Полный ASCENT 0.1.20; Edge: комикс 1×, saves, UI, семь локаций и пять активных боссов — PASS | Длительный плейтест, browser/mobile matrix |
| W9 3D | Отложено | После принятого 2D среза |
| W10 3 ГБ | Не проверено и не обещается | Отдельная нагрузочная квалификация |

## Последовательность дальнейших работ

**W0–W1.** Сначала принять isolated configure/build, минимальный WebGL кадр,
ввод/release-on-blur, resize/DPR, скрытие вкладки, ошибки загрузки и reload.
Нативные изменения проверять адресно; не заменять игровой SDK. TaskScheduler
в single-thread Web выполняет ограниченные CPU-задачи inline. Это явно
временная политика: blocking decode/network на main thread запрещён.

**W2a — реальная 2D игра.** Backend существующего façade реализован на Canvas2D,
с пакетной передачей команд, atlas regions, transforms/clipping/targets и
эффектами. D2D сохранён. Noto Sans заменяет системный шрифт; WebAudio и media
cursor запускаются после user gesture. Blur использует Canvas filter,
contour — SVG convolution, perspective/displacement — triangle mesh.
Попиксельное совпадение с D2D не заявляется. Следующая работа — визуальная
сверка сложных сцен и производительность, а не переписывание gameplay.

**W2b.** Развивать RHI buffers/textures/shaders/pipelines/commands/capabilities
по потребностям переносимого renderer. Полный 3D API не должен блокировать
первый 2D результат. Не протаскивать WebGL/D3D типы в Scene/Gameplay.

**W3.** Async byte source с uint64 offsets/sizes, request token, cancellation,
bounded completion queue и доставкой завершений в Tick. Native file,
memory и HTTP range реализации. Проверять 206 и Content-Range; ответ 200
не должен случайно скачивать многогигабайтный pack. Retry/backoff, timeout,
CORS и backpressure. Никакого синхронного ожидания fetch в кадре.

**W4.** Offline export/cook/pack: стабильные logical IDs, schema/ABI,
hashes/dependencies/variants, uint64 ranges, независимо декодируемые chunks.
Сначала небольшой boot/menu/initial-zone набор ASCENT, затем зоны.
Не добавлять всю игру через глобальный `--preload-file`.

**W5.** Подключить доставку к существующим ResourceManager/Streaming.
Отдельные бюджеты network, decode, CPU residency, GPU и upload-per-frame;
приоритеты видимости/камеры, отмена устаревших demands, lease ownership,
fallbacks и безопасное освобождение GPU. Появление workers не должно
требовать shared memory для базового режима запуска.

**W6.** Версионированный cache в IDB/OPFS с quota/eviction/коррупцией,
сохранения отдельно, подтверждение durable записи до успеха UI.
Reload/offline/очистка кеша не должны уничтожать saves. MEMFS не годится.

**W7–W8.** Installed Web SDK, воспроизводимый game export, правильный
application/wasm MIME, HTTPS/CORS, itch-compatible bootstrap без обязательных
COOP/COEP. Полная ASCENT: меню/комикс/audio, upgrades, несколько зон,
смерть/restart/save. Плейтест 30–60 минут и таблица отличий Windows/Web;
Chrome/Firefox/Edge, desktop/mobile память и медленная сеть. Диагностическая
геометрия и Node smoke не заменяют эту приёмку.

**W9–W10.** 3D и 2–3+ ГБ серверного контента — отдельные этапы после W8.
wasm32 достаточно для выборочной доставки; 3 ГБ контента не означают
3 ГБ heap/GPU. Qualification: cold/warm cache, переходы зон, eviction,
ограниченные память/сеть, p95/p99 кадров, ошибки range и отмена. До измерений
не заявлять поддержку такого объёма или production-ready Web platform.

Команды и ограничения текущего среза: [WebAssembly](web-platform.md).
Полный target небольшой игры не означает завершения W1–W10 или поддержки
многогигабайтного контента: он загружает все страницы атласа перед стартом.
