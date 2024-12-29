#include "usb_interface.h"
#include <QTreeWidget>
#include <QDir>
#include <QFile>
#include <QDebug>

class USBSysFS : public USBInterface {
public:
    void populateUSBTree(QTreeWidget *usbTree) override {
        QDir dir("/sys/bus/usb/devices");
        QStringList devices = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString &device : devices) {
            QFile vendorFile(dir.filePath(device + "/idVendor"));
            QFile productFile(dir.filePath(device + "/idProduct"));
            QFile manufacturerFile(dir.filePath(device + "/manufacturer"));
            QFile productFileText(dir.filePath(device + "/product"));

            if (vendorFile.exists() && productFile.exists()) {
                QString vendorId, productId, manufacturer = "Unknown", product = "Unknown";

                if (vendorFile.open(QFile::ReadOnly)) {
                    vendorId = vendorFile.readAll().trimmed();
                }
                if (productFile.open(QFile::ReadOnly)) {
                    productId = productFile.readAll().trimmed();
                }
                if (manufacturerFile.open(QFile::ReadOnly)) {
                    manufacturer = manufacturerFile.readAll().trimmed();
                }
                if (productFileText.open(QFile::ReadOnly)) {
                    product = productFileText.readAll().trimmed();
                }

                QTreeWidgetItem *item = new QTreeWidgetItem(usbTree);
                item->setText(0, vendorId);
                item->setText(1, productId);
                item->setText(2, QString("%1 (%2)").arg(product, manufacturer));
            }
        }
    }
};

USBInterface* createSysFS() {
    return new USBSysFS();
}
