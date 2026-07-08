# День 7 — C ABI и мост редактора, сведение недели

## feature/c-abi-seed

Цель фичи: первичный плоский C-интерфейс движка для .NET-редактора.
Описание фичи (для чего): набор sky_editor_*, через который редактор общается с движком; минимум — сессия и перечисление корней. Зависит от объектной модели. Контур E5 (совместно с E1).
Пошаговое описание действий:
Сделай файл editor/native_bridge/include/sky/editor/bridge/editor_bridge.h
В файле editor/native_bridge/include/sky/editor/bridge/editor_bridge.h должны быть функции sky_editor_create, sky_editor_destroy, sky_editor_root_count, sky_editor_root_at, sky_editor_object_name.
В функциях должна быть реализована логика (объявления): создание/уничтожение сессии, перечисление корневых объектов, чтение имени объекта в буфер.
Сделай файл editor/native_bridge/src/editor_bridge.cpp
В файле editor/native_bridge/src/editor_bridge.cpp должны быть реализации sky_editor_create, sky_editor_destroy, sky_editor_root_count, sky_editor_root_at, sky_editor_object_name.
В функциях должна быть реализована логика: create собирает движок и демо-сцену и возвращает сессию; destroy уничтожает; root_count/root_at перечисляют корни; object_name пишет имя в буфер и возвращает длину.
На выходе должно получиться:
- editor/native_bridge/include/sky/editor/bridge/editor_bridge.h
- editor/native_bridge/src/editor_bridge.cpp
- библиотека libsky_editor_bridge.so; рабочий C-интерфейс create/destroy/enumerate
КРИТЕРИЙ ПРАВИЛЬНОСТИ: вызов sky_editor_create возвращает не-null сессию; CI зелёный.

## feature/engine-bridge

Цель фичи: первичная связь редактора с движком через C-интерфейс (P/Invoke).
Описание фичи (для чего): загрузка нативного моста и создание/уничтожение сессии движка из .NET — фундамент всех дальнейших вызовов. Зависит от c-abi-seed (.so). Контур E3.
Пошаговое описание действий:
Сделай файл editor/avalonia/Engine/EngineInterop.cs
В файле editor/avalonia/Engine/EngineInterop.cs должен быть static class EngineInterop с объявлениями sky_editor_create(), sky_editor_destroy(IntPtr) и методами Resolve(...), Candidates().
В методах должна быть реализована логика: P/Invoke-объявления; поиск libsky_editor_bridge.so по SKY_BRIDGE_PATH и в дереве сборки.
Сделай файл editor/avalonia/Engine/EditorSession.cs
В файле editor/avalonia/Engine/EditorSession.cs должен быть class EditorSession : IDisposable со свойством Native и методом Dispose().
В методах должна быть реализована логика: конструктор вызывает sky_editor_create с проверкой на не-null; Dispose вызывает sky_editor_destroy.
На выходе должно получиться:
- editor/avalonia/Engine/EngineInterop.cs
- editor/avalonia/Engine/EditorSession.cs
- сессия движка создаётся из .NET и корректно освобождается
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build = 0 ошибок; сессия движка создаётся из .NET (не-null).

---

Сведение недели 1: все модули собираются, ctest и CI зелёные; полдня — на фиксацию контрактов модулей и стабилизацию. Точки соприкосновения проверены: core::Vec3/Transform (E1→E2, E4), engine/CMakeLists.txt (E5), sky_editor_create (E5→E3).
