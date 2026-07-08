# День 7

## feature/c-abi-seed
Цель фичи: первичный плоский C-интерфейс движка `sky_editor_*` (совместно с E1; контур E5).
Описание фичи (для чего): плоский набор C-функций, через который .NET-редактор общается с C++-движком; на этом этапе — минимум: сессия и перечисление корней.
Пошаговое описание действий:
Сделай файл editor/native_bridge/include/sky/editor/bridge/editor_bridge.h
В файле editor/native_bridge/include/sky/editor/bridge/editor_bridge.h должны быть SkyEditorContext* sky_editor_create(void), void sky_editor_destroy(SkyEditorContext* ctx), int32_t sky_editor_root_count(SkyEditorContext* ctx), SkyObjectId sky_editor_root_at(SkyEditorContext* ctx, int32_t index), int32_t sky_editor_object_name(SkyEditorContext* ctx, SkyObjectId object, char* buffer, int32_t capacity)
В функциях должна быть реализована логика (объявления): create возвращает указатель на сессию (собирает движок и демо-сцену); destroy уничтожает сессию; root_count возвращает число корневых объектов; root_at возвращает id корневого объекта; object_name пишет имя в буфер и возвращает длину.
Сделай файл editor/native_bridge/src/editor_bridge.cpp
В файле editor/native_bridge/src/editor_bridge.cpp должны быть реализации sky_editor_create, sky_editor_destroy, sky_editor_root_count, sky_editor_root_at, sky_editor_object_name
В функциях должна быть реализована логика: сборка движка и демо-сцены в сессии; перечисление корней; чтение имени объекта в буфер.
На выходе должно получиться:
- editor/native_bridge/include/sky/editor/bridge/editor_bridge.h
- editor/native_bridge/src/editor_bridge.cpp
- C-интерфейс create/destroy/enumerate работает
КРИТЕРИЙ ПРАВИЛЬНОСТИ: красный CI блокирует слияние; сломанный тест краснеет; редактор E3 вызывает sky_editor_create.

## feature/engine-bridge
Цель фичи: первичный вызов движка из редактора через P/Invoke (контур E3).
Описание фичи (для чего): загрузка нативного моста и создание/уничтожение сессии движка из .NET — фундамент всех дальнейших вызовов.
Пошаговое описание действий:
Сделай файл editor/avalonia/Engine/EngineInterop.cs
В файле editor/avalonia/Engine/EngineInterop.cs должны быть static class EngineInterop с [DllImport] static extern IntPtr sky_editor_create(), [DllImport] static extern void sky_editor_destroy(IntPtr ctx), static IntPtr Resolve(...), static string[] Candidates()
В методах должна быть реализована логика: sky_editor_create возвращает указатель на сессию движка; sky_editor_destroy уничтожает сессию; Resolve/Candidates находят libsky_editor_bridge.so по SKY_BRIDGE_PATH и в дереве сборки.
Сделай файл editor/avalonia/Engine/EditorSession.cs
В файле editor/avalonia/Engine/EditorSession.cs должен быть class EditorSession : IDisposable со свойством IntPtr Native и методом Dispose()
В методах должна быть реализована логика: конструктор вызывает sky_editor_create и проверяет не-null; Dispose() вызывает sky_editor_destroy; свойство Native отдаёт нативный указатель сессии.
На выходе должно получиться:
- editor/avalonia/Engine/EngineInterop.cs
- editor/avalonia/Engine/EditorSession.cs
- сессия движка создаётся из .NET и корректно освобождается
КРИТЕРИЙ ПРАВИЛЬНОСТИ: dotnet build = 0 ошибок; сессия движка создаётся из .NET (не-null); есть сброс компоновки.
