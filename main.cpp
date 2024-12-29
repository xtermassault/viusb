#include <QApplication>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>
#include "usb_interface.h"

class USBWidget : public QWidget {
public:
    USBWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        usbTree = new QTreeWidget(this);
        usbTree->setHeaderLabels(QStringList() << "Vendor ID" << "Product ID" << "Description" << "Serial Number");
        layout->addWidget(usbTree);

        USBInterface *usbImpl = createSysFS();
        if (usbImpl) {
            usbImpl->populateUSBTree(usbTree);
            delete usbImpl;
        } else {
            QTreeWidgetItem *item = new QTreeWidgetItem(usbTree);
            item->setText(0, "Error");
            item->setText(1, "");
            item->setText(2, "No USB implementation found");
        }
    }

private:
    QTreeWidget *usbTree;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    USBWidget widget;
    widget.setWindowTitle("cuteUSB (formerly viusb)");
    widget.resize(800, 600);
    widget.show();
    return app.exec();
}
