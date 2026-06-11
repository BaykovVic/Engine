#include <QApplication>
#include <QPixmap>
#include <QTimer>

#include "editor_context.hpp"
#include "main_window.hpp"
#include "theme.hpp"

int main(int argc, char** argv) {
    // Headless screenshot mode (CI / verification): render the window
    // offscreen and save it as an image.
    QString screenshotPath;
    bool screenshotInPlayMode = false;
    for (int i = 1; i < argc - 1; ++i) {
        if (qstrcmp(argv[i], "--screenshot") == 0 ||
            qstrcmp(argv[i], "--screenshot-play") == 0) {
            screenshotPath = QString::fromLocal8Bit(argv[i + 1]);
            screenshotInPlayMode = qstrcmp(argv[i], "--screenshot-play") == 0;
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }
    }

    QApplication app(argc, argv);
    sky::editor::applyDarkTheme(app);

    sky::editor::EditorContext context;
    sky::editor::MainWindow window(context);
    window.show();

    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(0, &window, [&] {
            window.selectObjectByName("Crate B");
            if (screenshotInPlayMode) {
                window.playFrames(150);
            }
            QApplication::processEvents();
            const bool saved = window.grab().save(screenshotPath);
            QApplication::exit(saved ? 0 : 1);
        });
    }
    return QApplication::exec();
}
