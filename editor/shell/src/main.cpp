#include <QApplication>
#include <QPixmap>
#include <QSurfaceFormat>
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
            // Default to the offscreen platform unless the caller chose one
            // (e.g. xcb under Xvfb, which can create GL contexts).
            if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
                qputenv("QT_QPA_PLATFORM", "offscreen");
            }
        }
    }

    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    sky::editor::applyDarkTheme(app);

    sky::editor::EditorContext context;
    for (int i = 1; i < argc - 1; ++i) {
        if (qstrcmp(argv[i], "--renderer") == 0) {
            context.config->set("engine.renderer", argv[i + 1]);
        }
    }
    sky::editor::MainWindow window(context);
    window.show();

    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(0, &window, [&] {
            // A generated landscape makes the verification shot meaningful.
            context.generateTerrain(1337);
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
