#include "sky/editor/tools/console_panel.hpp"

#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace sky::editor {
namespace {

class ConsoleLoggerAdapter final : public core::ILogger {
public:
    explicit ConsoleLoggerAdapter(ConsolePanel& panel) : panel_(panel) {}

    void log(core::LogLevel level, std::string_view category,
             std::string_view message) override {
        panel_.appendEntry(level, QString::fromUtf8(category.data(),
                                                    static_cast<int>(category.size())),
                           QString::fromUtf8(message.data(),
                                             static_cast<int>(message.size())));
    }

private:
    ConsolePanel& panel_;
};

} // namespace

ConsolePanel::ConsolePanel(QWidget* parent)
    : QWidget(parent), logger_(std::make_unique<ConsoleLoggerAdapter>(*this)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* toolbar = new QWidget(this);
    toolbar->setObjectName("consoleToolbar");
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(4, 2, 4, 2);
    auto* clearButton = new QPushButton(tr("Clear"), toolbar);
    clearButton->setObjectName("consoleClearButton");
    toolbarLayout->addWidget(clearButton);
    toolbarLayout->addStretch(1);
    layout->addWidget(toolbar);

    output_ = new QPlainTextEdit(this);
    output_->setObjectName("consoleOutput");
    output_->setReadOnly(true);
    output_->setMaximumBlockCount(2000);
    connect(clearButton, &QPushButton::clicked, output_, &QPlainTextEdit::clear);
    layout->addWidget(output_, 1);
}

ConsolePanel::~ConsolePanel() = default;

void ConsolePanel::appendEntry(core::LogLevel level, const QString& category,
                               const QString& message) {
    const char* color = "#d2d2d2";
    if (level == core::LogLevel::Warning) {
        color = "#e0c558";
    } else if (level >= core::LogLevel::Error) {
        color = "#e06c60";
    }
    output_->appendHtml(QString("<span style=\"color:%1\">[%2] %3</span>")
                            .arg(color, category.toHtmlEscaped(),
                                 message.toHtmlEscaped()));
}

} // namespace sky::editor
