#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include "usb_interface.h"
#include "ui_main_window.h"

void setupMenuActions(Ui::MainWindow &ui, QMainWindow &mainWindow) {
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
