#include "sky/editor/tools/package_panel.hpp"

#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace sky::editor {

PackagePanel::PackagePanel(package::PackageWorld& packages,
                           std::filesystem::path packagesRoot, QWidget* parent)
    : QWidget(parent), packages_(packages), packagesRoot_(std::move(packagesRoot)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    list_ = new QTreeWidget(this);
    list_->setHeaderLabels({tr("Package"), tr("Version"), tr("Status"),
                            tr("Extensions")});
    list_->setRootIsDecorated(false);
    layout->addWidget(list_, 1);

    auto* buttons = new QWidget(this);
    buttons->setObjectName("consoleToolbar");
    auto* buttonsLayout = new QHBoxLayout(buttons);
    buttonsLayout->setContentsMargins(4, 2, 4, 2);
    activateButton_ = new QPushButton(tr("Activate"), buttons);
    deactivateButton_ = new QPushButton(tr("Deactivate"), buttons);
    auto* refreshButton = new QPushButton(tr("Refresh"), buttons);
    buttonsLayout->addWidget(activateButton_);
    buttonsLayout->addWidget(deactivateButton_);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(refreshButton);
    layout->addWidget(buttons);

    connect(activateButton_, &QPushButton::clicked, this,
            &PackagePanel::activateSelected);
    connect(deactivateButton_, &QPushButton::clicked, this,
            &PackagePanel::deactivateSelected);
    connect(refreshButton, &QPushButton::clicked, this, &PackagePanel::refresh);

    refresh();
}

void PackagePanel::refresh() {
    packages_.discoverPackages(packagesRoot_);
    list_->clear();

    const auto active = packages_.activePackages();
    const auto isActive = [&](const std::string& packageId) {
        return std::ranges::any_of(active, [&](package::PackageHandle handle) {
            return packages_.manifest(handle).packageId == packageId;
        });
    };

    for (const auto& manifest : packages_.discoveredPackages()) {
        auto* item = new QTreeWidgetItem(list_);
        item->setText(0, QString::fromStdString(manifest.displayName));
        item->setText(1, QString::fromStdString(manifest.version));
        item->setText(2, isActive(manifest.packageId) ? tr("Active") : tr("Installed"));
        QStringList extensions;
        for (const auto& extension : manifest.extensionPoints) {
            extensions << QString::fromStdString(extension);
        }
        item->setText(3, extensions.join(", "));
        item->setData(0, Qt::UserRole, QString::fromStdString(manifest.packageId));
    }
    list_->resizeColumnToContents(0);
}

QString PackagePanel::selectedPackageId() const {
    const auto* item = list_->currentItem();
    return item != nullptr ? item->data(0, Qt::UserRole).toString() : QString{};
}

void PackagePanel::activateSelected() {
    const auto packageId = selectedPackageId();
    if (packageId.isEmpty()) {
        return;
    }
    // Resolution returns dependencies first; activate the whole chain.
    for (const auto& manifest : packages_.resolve({packageId.toStdString()})) {
        auto& handle = registered_[manifest.packageId];
        if (!handle.isValid()) {
            handle = packages_.registerPackage(manifest);
        }
        if (packages_.activate(handle)) {
            emit packageActivated(QString::fromStdString(manifest.packageId));
        }
    }
    refresh();
}

void PackagePanel::deactivateSelected() {
    const auto packageId = selectedPackageId();
    const auto it = registered_.find(packageId.toStdString());
    if (it == registered_.end()) {
        return;
    }
    if (packages_.deactivate(it->second)) {
        emit packageDeactivated(packageId);
    }
    refresh();
}

} // namespace sky::editor
