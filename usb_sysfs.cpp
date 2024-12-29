#include "usb_interface.h"
#include <QTreeWidget>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QMap>

class USBSysFS : public USBInterface {
public:
    void populateUSBTree(QTreeWidget *usbTree) override {
        usbTree->setHeaderLabels(QStringList() << "Bus" << "Device" << "Vendor ID" << "Product ID" << "Description");

        QDir dir("/sys/bus/usb/devices");
        QStringList devices = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        QMap<QString, QTreeWidgetItem*> sortedItems;

        for (const QString &device : devices) {
            QString devicePath = dir.absoluteFilePath(device);

            QString vendorId = readUsbInfo(devicePath, "idVendor");
            QString productId = readUsbInfo(devicePath, "idProduct");
            QString manufacturer = readUsbInfo(devicePath, "manufacturer");
            QString product = readUsbInfo(devicePath, "product");
            QString busNumber = QString("%1").arg(readUsbInfo(devicePath, "busnum").toInt(), 3, 10, QChar('0'));
            QString deviceNumber = QString("%1").arg(readUsbInfo(devicePath, "devnum").toInt(), 3, 10, QChar('0'));

            if (vendorId.isEmpty() || productId.isEmpty()) {
                continue;
            }

            if (manufacturer.isEmpty() || product.isEmpty()) {
                QString fallback = lookupDeviceName(vendorId, productId);
                product = fallback.section('(', 0, 0).trimmed();
                manufacturer = fallback.section('(', 1, 1).remove(')').trimmed();
            }

            QString description;
            if (!product.isEmpty() && !manufacturer.isEmpty()) {
                description = QString("%1 %2").arg(manufacturer, product);
            } else if (!manufacturer.isEmpty()) {
                description = manufacturer;
            } else if (!product.isEmpty()) {
                description = product;
            } else {
                description = "Unknown";
            }

            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, busNumber.isEmpty() ? "Unknown" : busNumber);
            item->setText(1, deviceNumber.isEmpty() ? "Unknown" : deviceNumber);
            item->setText(2, description);
            item->setText(3, vendorId);
            item->setText(4, productId);


            QString key = QString("%1-%2").arg(busNumber, deviceNumber);
            sortedItems[key] = item;
        }

        for (auto it = sortedItems.begin(); it != sortedItems.end(); ++it) {
            usbTree->addTopLevelItem(it.value());
        }
    }

private:
    QString readUsbInfo(const QString &devicePath, const QString &fileName) {
        QFile file(devicePath + "/" + fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            return in.readLine().trimmed();
        }
        return "";
    }

    QString lookupDeviceName(const QString &vendorId, const QString &productId) {
        QFile usbIds("/usr/share/hwdata/usb.ids");
        if (!usbIds.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Cannot open usb.ids file. File not found or permission denied.";
            return "Unknown (Unknown)";
        }

        QTextStream in(&usbIds);
        QString vendor, product;
        bool foundVendor = false;

        while (!in.atEnd()) {
            QString line = in.readLine();

            if (line.isEmpty() || line.startsWith("#")) {
                continue;
            }

            if (!line.startsWith("\t") && line.startsWith(vendorId + " ")) {
                vendor = line.mid(5).trimmed();
                foundVendor = true;
            } else if (foundVendor && line.startsWith("\t" + productId + " ")) {
                product = line.mid(5).trimmed();
                break;
            } else if (!line.startsWith("\t")) {
                foundVendor = false;
            }
        }

        usbIds.close();
        if (!vendor.isEmpty() && !product.isEmpty()) {
            return QString("%1 (%2)").arg(product, vendor);
        } else if (!vendor.isEmpty()) {
            return QString(vendor);
        }
        return "Unknown";
    }
};

USBInterface* createSysFS() {
    return new USBSysFS();
}
