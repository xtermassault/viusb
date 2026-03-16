#include <QApplication>
#include <QActionGroup>
#include <QHeaderView>
#include <QMainWindow>
#include <QMessageBox>
#include <QSettings>
#include <QDir>
#include <memory>

#include "ui_main_window.h"
#include "usb_interface.h"

namespace {
constexpr auto kConfigDir = ".config/viusb";
constexpr auto kConfigFile = ".config/viusb/viusb.conf";
constexpr auto kViewModeKey = "ui/view_mode";

QString viewModeToString(USBViewMode mode) {
    return mode == USBViewMode::Classes ? "classes" : "classic";
}

USBViewMode viewModeFromString(const QString &value) {
    return value == "classes" ? USBViewMode::Classes : USBViewMode::Classic;
}
}

void setupMenuActions(Ui::MainWindow &ui, QMainWindow &mainWindow, USBInterface &usbInterface, QSettings &settings) {
    QObject::connect(ui.actionExit, &QAction::triggered, &mainWindow, &QMainWindow::close);

    QObject::connect(ui.actionAbout, &QAction::triggered, [&]() {
        QMessageBox::about(&mainWindow, "About cuteUSB",
                           "cuteUSB v.0.2a\n\n"
                           "by 0x64\n\n"
                           "A visual 'lsusb' implementation. \n\n"
                           "Powered by Qt. More info in About->About Qt section.\n\n");
    });

    QObject::connect(ui.actionAboutQt, &QAction::triggered, [&]() {
        QApplication::aboutQt();
    });

    auto *viewActions = new QActionGroup(&mainWindow);
    viewActions->addAction(ui.actionViewClassic);
    viewActions->addAction(ui.actionViewClasses);
    viewActions->setExclusive(true);

    QObject::connect(ui.actionViewClassic, &QAction::triggered, [&]() {
        usbInterface.setViewMode(USBViewMode::Classic);
        settings.setValue(kViewModeKey, viewModeToString(USBViewMode::Classic));
        settings.sync();
    });
    QObject::connect(ui.actionViewClasses, &QAction::triggered, [&]() {
        usbInterface.setViewMode(USBViewMode::Classes);
        settings.setValue(kViewModeKey, viewModeToString(USBViewMode::Classes));
        settings.sync();
    });
}

void setupUsbTree(QTreeWidget *usbTree) {
    usbTree->setSortingEnabled(true);
    usbTree->sortByColumn(0, Qt::AscendingOrder);
    usbTree->setAlternatingRowColors(true);
    usbTree->setRootIsDecorated(false);

    auto *header = usbTree->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QDir::home().mkpath(kConfigDir);
    QSettings settings(QDir::home().filePath(kConfigFile), QSettings::IniFormat);
    const USBViewMode initialViewMode = viewModeFromString(settings.value(kViewModeKey, "classic").toString());

    QMainWindow mainWindow;
    Ui::MainWindow ui;
    ui.setupUi(&mainWindow);

    std::unique_ptr<USBInterface> usbInterface(createSysFS());
    setupMenuActions(ui, mainWindow, *usbInterface, settings);
    setupUsbTree(ui.usbTreeWidget);
    ui.actionViewClassic->setChecked(initialViewMode == USBViewMode::Classic);
    ui.actionViewClasses->setChecked(initialViewMode == USBViewMode::Classes);
    usbInterface->setViewMode(initialViewMode);
    usbInterface->populateUSBTree(ui.usbTreeWidget, ui.statusBar);

    mainWindow.show();
    return app.exec();
}
