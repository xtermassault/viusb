#include <QApplication>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>
#include "usb_interface.h"

class USBWidget : public QWidget {
public:
    USBWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        usbTree = new QTreeWidget(this);
        usbTree->setHeaderLabels(QStringList() << "Vendor ID" << "Product ID" << "Device Description");
        layout->addWidget(usbTree);

        USBInterface *usbImpl = detectUSBImplementation();
        if (usbImpl) {
            usbImpl->populateUSBTree(usbTree);
            delete usbImpl; // Освобождаем ресурсы
        } else {
            QTreeWidgetItem *item = new QTreeWidgetItem(usbTree);
            item->setText(0, "Error");
            item->setText(1, "");
            item->setText(2, "No USB implementation found");
        }
    }

private:
    QTreeWidget *usbTree;

    USBInterface* detectUSBImplementation() {
        USBInterface *impl = createLibUSB();
        if (impl) return impl;

        return createSysFS();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    USBWidget widget;
    widget.setWindowTitle("USB Viewer");
    widget.resize(600, 400);
    widget.show();
    return app.exec();
}
