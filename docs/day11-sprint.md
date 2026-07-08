# Спринт 1 · День 11 — выдача фич (по одной на контур)

День 11: каждый контур берёт свою **11-ю фичу** (в порядке реализации из
ролевого ТЗ). Одна фича = одна ветка `feature/<название>` = один запрос на
слияние. У методов — **сигнатура**, **что делает**, **параметры**, **что
возвращает**; тела методов с построчной «Реализацией» — в `docs/role-E?.md`.

## Что берут в этот день

| Контур | Фича | Этап (неделя) |
|---|---|---|
| **E1** Ядро/данные | `feature/skyb-serialization` | Этап 3 (Недели 3–4, веха M2). Отмена операций и формат сцены SKYB |
| **E2** Рендеринг | — (фичи этого контура закончились) | — |
| **E3** Редактор(.NET) | `feature/script-inspector` | Этап 5 (Недели 7–8, веха M4). Выбор класса скрипта и его поля |
| **E4** Рантайм/скриптинг | — (фичи этого контура закончились) | — |
| **E5** Пайплайн/пакеты | `feature/package-tests` | Этап 5 (Недели 7–8, веха M4). Менеджер пакетов |
| **E6** Data-oriented(ECS) | — (фичи этого контура закончились) | — |

---

## E1 · `feature/skyb-serialization`

*Этап: Этап 3 (Недели 3–4, веха M2). Отмена операций и формат сцены SKYB.*

#### Дополнение файла `engine/scene/src/scene_world.cpp`
- `saveSceneAs` реализуется как настоящий SKYB: магия "SKYB", схема `sky.scene`,
  обход корней → имя, трансформ, компоненты с полями (через `ByteWriter`); чтение
  обратно с прямой миграцией версий.

---

## E3 · `feature/script-inspector`

*Этап: Этап 5 (Недели 7–8, веха M4). Выбор класса скрипта и его поля.*

#### Дополнение файла `editor/avalonia/Engine/EditorSession.cs`
- `public List<string> ScriptClasses(string current)`
  Что делает: получает через C-интерфейс имена классов-скриптов. Параметры: `current`. Возвращает: список классов для выпадающего списка.
- `public void SetScriptField(ulong id, int component, int field, string value)` — записывает поле скрипта.
- `ComponentField.IsScriptClass` — поле `class` рендерится выпадающим списком; сериализуемые поля скрипта добавляются к списку у компонента `sky.script`.

#### Дополнение файла `editor/avalonia/Engine/EngineInterop.cs`
P/Invoke: `sky_editor_script_class_count`, `sky_editor_script_class_name`,
`sky_editor_script_field_count`, `sky_editor_script_field_name`,
`sky_editor_script_field_value`, `sky_editor_set_script_field`.

---

## E5 · `feature/package-tests`

*Этап: Этап 5 (Недели 7–8, веха M4). Менеджер пакетов.*

#### Файл `tests/package_tests.cpp`
semver, MVS, конфликты, lock round-trip, установка из трёх источников.

---
