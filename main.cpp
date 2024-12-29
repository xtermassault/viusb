#include <QApplication>
#include <QMainWindow>
#include "usb_interface.h"
#include "ui_main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    Ui::MainWindow ui;
    ui.setupUi(&mainWindow);

    USBInterface *usbInterface = createSysFS();
    usbInterface->populateUSBTree(ui.usbTreeWidget);

    mainWindow.show();
    return app.exec();
}
