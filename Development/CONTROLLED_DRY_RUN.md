# CONTROLLED DRY RUN — RuntimeFoundation quaternion API

Дата: 2026-10-08. Только чтение; ни одна приведённая build/test команда не выполнялась. Создан только запрошенный отчёт. PASS не присваивается.

## ПРОВЕРЕНО: компонент и зависимости

Выбран quaternion/spatial API библиотеки `EpidemicRuntimeFoundation`. [CMakeLists.txt:1](E:/Epidemic/EngineRuntime/RuntimeFoundation/CMakeLists.txt:1) объявляет STATIC target, production TU `src/runtime_foundation.cpp`, PUBLIC include-directory и PUBLIC зависимость `EpidemicFoundation`; строки 13–38 запрещают прямые связи с перечисленными Runtime majors. Production TU содержит только пустую директиву `#`; поведение реализовано inline в публичном [spatial.h](E:/Epidemic/EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/spatial.h:22), подключаемом umbrella [runtime_foundation.h:10](E:/Epidemic/EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_foundation.h:10).

[tests/CMakeLists.txt:1](E:/Epidemic/EngineRuntime/RuntimeFoundation/tests/CMakeLists.txt:1) создаёт `EpidemicRuntimeFoundationTests`, связывает библиотеку и вызывает `epidemic_add_test`. [Корневой CMakeLists.txt:18](E:/Epidemic/CMakeLists.txt:18) регистрирует CTest под именем target с timeout 300. Прочитан также существующий generated CTestfile; это дополнительная информация, а не замена source CMake. Транзитивные зависимости `EpidemicFoundation` не исследовались.

## ПРОВЕРЕНО: три инварианта и реальные тесты

1. **Нормализация и безопасный fallback.** [Normalize:79](E:/Epidemic/EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/spatial.h:79) вычисляет norm² в double; неподходящие значения возвращают identity `Quat{}`. `TestQuaternionNormalizationAndFallbackContract` в [tests:532](E:/Epidemic/EngineRuntime/RuntimeFoundation/tests/runtime_foundation_tests.cpp:532) проверяет нормализованный результат, zero, NaN/Inf, near-overflow и overflow во всех координатах. Вызван из test main на строке 805; ошибка даёт код 30.
2. **Согласованная геометрия обычного поворота.** [RotateVector:106](E:/Epidemic/EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/spatial.h:106) сначала нормализует quaternion. [TestQuaternionAndTransformMath:210](E:/Epidemic/EngineRuntime/RuntimeFoundation/tests/runtime_foundation_tests.cpp:210) проверяет Z-поворот 90°: X → Y, а также позицию/масштаб композиции и нормализованную rotation. Main:744, ошибка 11. Это подтверждает конкретный сценарий, не всю численную область.
3. **Невалидная rotation отвергается.** [IsValidTransform:191](E:/Epidemic/EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/spatial.h:191) требует finite position/rotation/scale, normalized rotation и ненулевые scale-компоненты. [TestInvalidAndZeroQuaternionValidation:246](E:/Epidemic/EngineRuntime/RuntimeFoundation/tests/runtime_foundation_tests.cpp:246) проверяет rejection zero/Inf rotation и identity fallback нормализации zero. Main:754, ошибка 13.

Существование assertions и регистрации подтверждено. Их прохождение сегодня — **НЕ ПРОВЕРЕНО**.

## НЕ ПРОВЕРЕНО: гипотеза дефекта

В [RotateVector:120](E:/Epidemic/EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/spatial.h:120) промежуточное `uuv * 2.0f` может переполниться для finite вектора с очень большой компонентой, даже если конечный математический поворот представим. Кандидат: quaternion `{0,0,1,0}` (180° вокруг Z), вектор `{FLT_MAX,0,0}`, ожидается `{-FLT_MAX,0,0}`. Это гипотеза, **не подтверждённая ошибка**.

Точный поиск `RotateVector(` в тестовом файле нашёл только обычный вызов на строке 215. Косвенные сценарии через другие функции полностью не исследованы; отсутствие покрытия во всех тестах не заявляю.

Минимальная будущая проверка: один case в существующем `runtime_foundation_tests.cpp`: вызвать RotateVector с указанными входами, проверить finite результат и ожидаемые компоненты; сравнить с double-reference. Запустить только этот test target. Перед изменением уточнить контракт для экстремально больших finite векторов. Сейчас case не добавлялся и не выполнялся.

## Команды для будущего изменения

```powershell
Development/AgentsRules/Scripts/Invoke-Dev.ps1 -Action Configure -Profile runtime -Configuration Debug
Development/AgentsRules/Scripts/Invoke-Dev.ps1 -Action Build -Profile runtime -Configuration Debug -Target EpidemicRuntimeFoundationTests
Development/AgentsRules/Scripts/Invoke-Dev.ps1 -Action Test -Profile runtime -Configuration Debug -Target EpidemicRuntimeFoundationTests -TestRegex '^EpidemicRuntimeFoundationTests$'
```

Изменение public header дополнительно потребует сборки затронутых потребителей; их список и соответствующие integration tests — **НЕ ПРОВЕРЕНО** в данном лимите.

## Бюджет и итог

5 исследовательских операций плюс 2 общие операции записи/проверки отчётов: 7 из 12. Шесть поисков: один `graft ask --source --no-refresh`, пять rg-запросов; один поиск определения helper в `cmake/` не дал результата, после чего найдено определение в корневом CMake. Выведены фрагменты 8 файлов, менее 350 строк, предел 400. Использованы только exec_command, Graft, rg, Get-Content и Test-Path; clangd, retrieval и тяжёлые процессы не поднимались.

**ПРОБЛЕМА для оценки:** проверка численной гипотезы и транзитивного blast radius не выполнена. **Вердикт: READY WITH RISKS для планирования focused-изменения; runtime корректность НЕ ПРОВЕРЕНО.**
