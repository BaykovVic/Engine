#pragma once

#include <QIcon>
#include <QString>

namespace sky::editor {

/// Crisp vector icons drawn at runtime (no asset files), so the editor ships
/// a coherent icon set without external dependencies. Each icon carries a
/// muted "off" state and a bright "on" state for checked tool buttons.
QIcon toolbarIcon(const QString& name);

} // namespace sky::editor
