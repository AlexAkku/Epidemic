# Создание игры на Epidemic 1.0.1

Практическая точка входа для разработчика и ИИ-агента, обновлена 21.10.2026.
Архитектура и публичные заголовки определяют контракт; исторические API-страницы
Vault не гарантируют совпадения с текущей веткой. Этот документ не разрешает
возобновлять разработку замороженного Ascent.

## Сначала выберите способ интеграции

| Задача | Начальная точка |
|---|---|
| Новая Windows 2D-игра | `Examples/MinimalGame`, установленный SDK, Presentation + Session |
| Браузерная игра | `Examples/Web`, Emscripten; [Web guide](web-platform.md) |
| Собственная низкоуровневая композиция | EngineBase Application, Runtime contracts и Support adapters |
| Игра на старом SDK | Сохранить её SDK; миграцию делать отдельно от сборки игры |

Не начинайте с чтения всех majors. Сначала найдите готовый работающий composition
root, используемые им headers и CMake targets. Не копируйте в новую игру исходники
Ascent, его `.rc`, профиль, боевые правила или абсолютные пути разработчика.

SDK устанавливается отдельно. Для Windows см. [интеграцию SDK](<Epidemic Engine Vault/03 Работа с движком/Практическая интеграция игры.md>).
Минимальный внешний CMake-клиент:

```cmake
cmake_minimum_required(VERSION 3.25)
project(MyGame LANGUAGES CXX)
set(EPIDEMIC_REQUIRED_ABI 1)
find_package(Epidemic 1.0.1 CONFIG REQUIRED COMPONENTS Presentation Session)
add_executable(MyGame main.cpp)
target_compile_features(MyGame PRIVATE cxx_std_23)
target_link_libraries(MyGame PRIVATE
    Epidemic::EpidemicGameFrameworkPresentation
    Epidemic::EpidemicGameFrameworkSession)
```

`CMAKE_PREFIX_PATH` указывает на установленный SDK. Не подменяйте его путем к
исходникам или диагностическому build. Не выполняйте install автоматически при
обычной сборке клиента. Native, Emscripten, coverage и exact используют разные caches.

## Границы ответственности

- EngineBase: lifecycle, платформа, ввод и базовая диагностика.
- EngineRuntime: независимые технические majors; их соединения находятся в Support.
- EngineFramework: переиспользуемые фасады и игровые механизмы.
- Game: правила, контент, состояния экрана, прогрессия, формат сохранения и composition root.

Разрешённое направление зависимостей: Game → Framework → Runtime → Base.
Нижний слой не знает имён игры. Runtime major не подключает соседний major
напрямую; используйте его public contract и Support adapter. Не создавайте второй
Application lifecycle поверх Presentation Host.

Практичная структура небольшого клиента: `main` собирает зависимости; `App`
переключает экраны; `World` содержит симуляцию; `View` рисует; `Assets` владеет
ресурсами; `Profile` сериализует данные. Это роли, а не требование создать шесть
иерархий классов или по отдельному файлу на каждый метод.

## Владение и кадр

Объявляйте Host раньше Canvas, Audio и объектов, которые на него ссылаются.
Они разрушаются раньше Host. Снимайте callbacks, захватывающие игровые объекты,
до уничтожения этих объектов: например, `host.SetAudioTick({})`.

`Host::Run(events, tick)` обслуживает события и кадр. В событиях обрабатывайте
закрытие и resize; вызывайте `Canvas::Resize()` при изменении размеров.
`RequestStop()` завершает loop. Для инструментов есть `ConfigureStartup(false)`.
На Web callback продолжает жить после возврата Run: состояние не должно ссылаться
на уже уничтоженные stack-объекты; следуйте lifetime-композиции Web-примера.

Порядок одного кадра:

1. Снять `host.CurrentSnapshot()`, определить нажатия и удержания.
2. Перевести мышь в логические координаты через `canvas.MousePoint()`.
3. Обновить UI и сформировать команды для симуляции.
4. Выполнить ограниченное число фиксированных шагов симуляции.
5. Один раз `Canvas::Begin()`, нарисовать мир и UI, один раз `End()`.

Переходы pause/focus/menu очищают устаревший ввод и временной долг. Не допускайте
многократного выполнения одного edge-triggered нажатия на нескольких fixed steps.
Используйте отдельные состояния экранов вместо разбросанных глобальных флагов.

## Часы: simulation, UI, audio

`<Epidemic/GameFramework/Presentation/frame_clocks.h>` содержит `FrameClocks`.
Он принимает wall delta в секундах, time scale, pause и позицию медиа:

```cpp
namespace presentation = epidemic::gameplay::presentation;
presentation::FrameClocks clocks(1.0 / 120, 8);
// wallDelta измеряется steady_clock; musicPosition — реальный cursor аудио.
auto frame = clocks.Advance(wallDelta, timeScale, paused, musicPosition);
for (unsigned i = 0; i < frame.simulation_steps; ++i)
    world.Step(frame.simulation_step);
ui.Update(frame.ui_delta);
view.Draw(frame.interpolation);
```

UI delta не масштабируется и не останавливается паузой симуляции. Fixed-step долг
ограничен числом шагов; `dropped_simulation_seconds` показывает отброшенное время.
Такой cap — явная политика локальной игры, а не обещание lockstep-детерминизма.
`simulation_time` получается из количества шагов. `ClearDebt()` нужен при
разрыве времени; `Reset()` также сбрасывает накопленные часы.

`media_position` только передаёт положение реального аудио. Для синхронизации
озвучки используйте cursor Music/Audio, а не сумму кадров. Helper не приостанавливает
звук и не заменяет audio bus. Пауза звука и смена трека остаются явными действиями клиента.

## Составные 2D-персонажи и anchors

`cutout_animation.h`: `CutoutBone`, `CutoutKey`, tracks, `EvaluateCutoutPose`,
`PlantCutoutFoot`. Parent должен предшествовать child. Углы задаются в градусах,
ось Y соответствует Canvas; ключи упорядочены по phase. Loop track должен иметь
согласованные начальный и конечный ключи.

`rig_2d.h` добавляет:

- `CutoutSpriteMatrix(pose, pivot, mirrored)`: pivot в пикселях исходного изображения;
  внутри transform рисуйте изображение от `(0,0)` в его исходном размере.
- `TransformCutoutAnchor(matrix, sourcePixel)`: attachment для руки, оружия или света
  вычисляется тем же transform, что и рисунок.
- `MirrorCutoutPose(pose, rootX)`: отражение уже вычисленных world joints;
  отражённое изображение также рисуется с `mirrored=true`.
- `DrawCutoutRigPreview`: рисует joints и parent links, без игровых текстур.

Храните канонические части, pivots и tracks вместе с воспроизводимым рецептом
экспорта. Bounds/столкновения не выводите автоматически из анимированного рисунка.
Не считайте порядок спрайтов эквивалентом parent hierarchy: порядок рисования
задаётся отдельно, чтобы руки и оружие не пересекались случайно.

Пример использования без ресурсов игры — `Examples/MinimalGame --rig-preview`.
Он показывает анимированную цепочку joints и anchor, переключение отражения и паузы.
Tab / Shift+Tab меняют focus, Enter или мышь активируют кнопку, Escape закрывает окно.
Это небольшой SDK-пример, а не редактор персонажей. В этой работе пример не запускался.

## UI и локализация

`ui_2d.h`: `UiTarget` содержит стабильный ненулевой id и `UiRect` в логических
координатах. Передавайте targets в порядке рисования в `UiFocusModel::Update`.
`HitTest` предпочитает последний видимый enabled target; `Move` пропускает disabled
и non-focusable элементы; исчезнувший target теряет focus. Нулевой id означает отсутствие.

`FocusAt` возвращает hit id; `Focused()` используется для keyboard activation.
Сам helper не запускает игровые команды. Выполняйте действие один раз по событию,
а не каждый кадр удержания. Для модального окна передавайте только его targets:
элементы под ним не должны получать ввод. Отдельный hidden-флаг не нужен —
скрытый элемент исключается из списка. Границы прямоугольника полуоткрытые.

`LocalizeUi(entries, locale, key, fallback)` возвращает текст выбранного языка,
затем fallback (по умолчанию `en`), затем key. Строки принадлежат клиенту и должны
жить дольше возвращённого view. ID управления не зависит от перевода текста.
Загрузка файлов, plural rules, RTL и сложный layout в этот базовый helper не входят.
Повторно измеряйте/раскладывайте текст при смене языка и размера окна.

## Ресурсы, звук и сохранения

Для native используйте явные пути относительно приложения; пользовательские данные
храните отдельно через `Host::UserDirectory(application)`. Windows `.rc` — один
из способов доставки, а не обязательный формат всех игр. Canvas загружает atlas
и metadata, переданные клиентом. Не удаляйте случайно/динамически выбранные sprites
по отсутствию строкового имени в одном профиле.

Декодированная текстура занимает существенно больше файла на диске. Задавайте
бюджет размеров/числа atlas pages и учитывайте браузерные ограничения. Web assets
нужно раздавать HTTP(S); для WASM требуется правильный MIME. Не копируйте EXE/DLL
в Web-поставку. AudioContext запускается по пользовательскому жесту; длинную музыку
предпочтительно стримить, SFX — декодировать с ограничением одновременных voices.

Музыкальный объект освобождайте после использования; JS backend теперь использует
непрозрачные числовые handles и Map живых объектов. Не обращайтесь из игры к
внутреннему `Module.epidemicAudio.music`: публичный интерфейс — C++ Audio/Music.

Session `SaveProfilePayload` / `LoadProfilePayload` сохраняют payload, а не игровой
мир автоматически. Формат, schema version, миграции и recovery принадлежат Game.
Текущий payload непустой и не больше 32768 байт. На Web дождитесь завершения IDBFS sync;
закрытие вкладки до подтверждения записи может потерять последние изменения.
Не сериализуйте указатели и render/audio handles.

## Принципы работы ИИ-агента

1. Зафиксировать платформу, версию SDK, allowed scope и способ сборки из запроса
   и локального AGENTS. Сначала определить, исходники это или установленный package.
2. Взять ближайший SDK-пример; прочитать только задействованные headers и composition
   root. Сигнатуры проверять в текущем коде, а не придумывать по исторической статье.
3. Сделать один вертикальный срез: bootstrap → input → state → draw → stop.
   Потом добавлять одобренные требования небольшими независимыми изменениями.
4. Держать игровое значение в Game. Не менять слой движка для каждого частного правила.
   Новую reusable-возможность добавлять только с явным контрактом и реальным потребителем.
5. Сохранять authored originals; производные atlas/audio генерировать из них.
   Не превращать Temp в единственное хранилище исходников и не переустанавливать SDK скрытно.
6. Обычно проверять только затронутый путь: state, профиль, window/UI, audio или package.
   При запрете запусков ограничиться чтением/diff и честно отметить отсутствие runtime-проверки.
7. Отделять current, legacy, preview и стресс-сценарии. Фиксировать content version,
   seed/подготовку, backend и ограничения, прежде чем сравнивать числа.
8. Завершать результатом: код, актуальная короткая инструкция, реальные проверки
   и оставшиеся ограничения. Не создавать коллекции служебных отчётов вместо продукта.

Новые helpers от 21.10.2026 добавлены без сборки и прогонов по явному требованию
владельца. Документ описывает реализованные интерфейсы, а не квалификацию релиза.

## Навигация

- [Минимальный клиент](../Examples/MinimalGame/main.cpp), [CMake](../Examples/MinimalGame/CMakeLists.txt).
- [Presentation headers](../EngineFramework/RuntimeBoundary/Presentation/include/Epidemic/GameFramework/Presentation/).
- [Session payload](../EngineFramework/IntegrationLayer/Session/include/Epidemic/GameFramework/Session/profile_store.h).
- [Auditor](<Epidemic Engine Vault/08 Auditor/Запуск Auditor.md>).
- [Оставшиеся задачи](maintenance-todo.md), [архив отчётов](Reports/Archive/README.md).
