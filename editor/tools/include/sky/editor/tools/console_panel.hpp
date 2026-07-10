#pragma once

#include <QWidget>
#include <memory>

#include "sky/core/logger.hpp"

class QPlainTextEdit;

namespace sky::editor {

/// Unity-like Console: engine log output inside the editor. The exposed
/// logger() adapter can be handed to any engine service.
class ConsolePanel final : public QWidget {
    Q_OBJECT

public:
    explicit ConsolePanel(QWidget* parent = nullptr);
    ~ConsolePanel() override;

    void appendEntry(core::LogLevel level, const QString& category,
                     const QString& message);
    [[nodiscard]] core::ILogger& logger() { return *logger_; }

private:
    QPlainTextEdit* output_ = nullptr;
    std::unique_ptr<core::ILogger> logger_;
};

} // namespace sky::editor
