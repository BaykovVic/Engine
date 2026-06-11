#pragma once

#include <QWidget>
#include <filesystem>
#include <unordered_map>

#include "sky/package/package_world.hpp"

class QPushButton;
class QTreeWidget;

namespace sky::editor {

/// Unity-like Package Manager: discovered local packages with their
/// versions, dependencies, extensions and activation state. Everything goes
/// through the Package System contracts; no external registry exists.
class PackagePanel final : public QWidget {
    Q_OBJECT

public:
    PackagePanel(package::PackageWorld& packages, std::filesystem::path packagesRoot,
                 QWidget* parent = nullptr);

    void refresh();

signals:
    void packageActivated(QString packageId);
    void packageDeactivated(QString packageId);

private:
    void activateSelected();
    void deactivateSelected();
    [[nodiscard]] QString selectedPackageId() const;

    package::PackageWorld& packages_;
    std::filesystem::path packagesRoot_;
    QTreeWidget* list_ = nullptr;
    QPushButton* activateButton_ = nullptr;
    QPushButton* deactivateButton_ = nullptr;
    std::unordered_map<std::string, package::PackageHandle> registered_;
};

} // namespace sky::editor
