#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include "usb_interface.h"
#include "ui_main_window.h"

void setupMenuActions(Ui::MainWindow &ui, QMainWindow &mainWindow) {
    QObject::connect(ui.actionExit, &QAction::triggered, &mainWindow, &QMainWindow::close);

    QObject::connect(ui.actionAbout, &QAction::triggered, [&]() {
        QMessageBox::about(&mainWindow, "About cuteUSB",
                           "cuteUSB (formerly viusb)\n\n"
                           "A USB device viewer application.\n\n"
                           "Built using Qt for cross-platform development.");
    });

    QObject::connect(ui.actionAboutQt, &QAction::triggered, [&]() {
        QApplication::aboutQt();
    });
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    Ui::MainWindow ui;
    ui.setupUi(&mainWindow);

    setupMenuActions(ui, mainWindow);

    USBInterface *usbInterface = createSysFS();
    usbInterface->populateUSBTree(ui.usbTreeWidget);

    mainWindow.show();
    return app.exec();
}
