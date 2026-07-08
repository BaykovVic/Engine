# День 7 — C ABI и мост редактора, сведение недели

Фичи дня (в порядке реализации). Одна фича = ветка `feature/<название>` = один PR в `develop`.

---

## feature/c-abi-seed (E5)
**Цель фичи:** первичный плоский C-интерфейс движка для .NET-редактора (совместно с E1).
**Описание фичи (для чего):** набор `sky_editor_*`, через который редактор общается с движком; минимум — сессия и перечисление корней. Зависит от объектной модели.
**Пошаговое описание действий:**
- Сделай файлы `editor/native_bridge/include/sky/editor/bridge/editor_bridge.h`, `editor/native_bridge/src/editor_bridge.cpp`.
- В файлах должны быть `sky_editor_create`, `sky_editor_destroy`, `sky_editor_root_count`, `sky_editor_root_at`, `sky_editor_object_name`.
- В функциях должна быть реализована логика: `create` собирает движок и демо-сцену и возвращает сессию; `destroy` уничтожает; `root_count`/`root_at` перечисляют корни; `object_name` пишет имя в буфер и возвращает длину.
**На выходе должно получиться:** `editor_bridge.h`, `editor_bridge.cpp`; библиотека `libsky_editor_bridge.so`; рабочий C-интерфейс create/destroy/enumerate.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** редактор вызывает `sky_editor_create` и получает не-null сессию; CI зелёный.

## feature/engine-bridge (E3)
**Цель фичи:** первичная связь редактора с движком через C-интерфейс (P/Invoke).
**Описание фичи (для чего):** загрузка нативного моста и создание/уничтожение сессии движка из .NET — фундамент всех дальнейших вызовов. Зависит от `c-abi-seed` (`.so`).
**Пошаговое описание действий:**
- Сделай файл `editor/avalonia/Engine/EngineInterop.cs` — `static class EngineInterop` с `[DllImport] sky_editor_create()`, `sky_editor_destroy(IntPtr)`, `Resolve(...)`, `Candidates()`.
- В методах должна быть реализована логика: объявления P/Invoke; поиск `libsky_editor_bridge.so` по `SKY_BRIDGE_PATH` и в дереве сборки.
- Сделай файл `editor/avalonia/Engine/EditorSession.cs` — `class EditorSession : IDisposable` со свойством `Native` и `Dispose()`.
- В методах должна быть реализована логика: конструктор вызывает `sky_editor_create` с проверкой на не-null; `Dispose` вызывает `sky_editor_destroy`.
**На выходе должно получиться:** `EngineInterop.cs`, `EditorSession.cs`; сессия создаётся из .NET и корректно освобождается.
**КРИТЕРИЙ ПРАВИЛЬНОСТИ:** `dotnet build` = 0 ошибок; сессия движка создаётся из .NET (не-null).

---

## Сведение недели 1
- Все модули собираются, `ctest` и CI зелёные (полдня на фиксацию контрактов модулей и стабилизацию).
- Точки соприкосновения проверены: `core::Vec3/Transform` (E1→E2, E4), `engine/CMakeLists.txt` (E5 включает модули по мере готовности), `sky_editor_create` (E5→E3).
