#include "usb_interface.h"
#include <QTreeWidget>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QDebug>

class USBSysFS : public QObject, public USBInterface {
    Q_OBJECT

public:
    USBSysFS() : usbTree(nullptr) {
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &USBSysFS::refreshUSBTree);
        timer->start(1000);
    }

    void populateUSBTree(QTreeWidget *tree) override {
        usbTree = tree;
        refreshUSBTree();
    }

private:
    QTreeWidget *usbTree;
    QTimer *timer;
    QMap<QString, QTreeWidgetItem*> currentDevices;

    void refreshUSBTree() {
        if (!usbTree) return;

        QDir dir("/sys/bus/usb/devices");
        QStringList devices = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        QSet<QString> detectedKeys;

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

            QString description = (!product.isEmpty() && !manufacturer.isEmpty())
                                      ? QString("%1 %2").arg(manufacturer, product)
                                      : (!manufacturer.isEmpty() ? manufacturer : (!product.isEmpty() ? product : "Unknown"));

            QString key = QString("%1-%2").arg(busNumber, deviceNumber);
            detectedKeys.insert(key);

            QTreeWidgetItem *item = currentDevices.value(key, nullptr);
            if (!item) {
                item = new QTreeWidgetItem(usbTree);
                currentDevices[key] = item;
            }

            item->setText(0, busNumber);
            item->setText(1, deviceNumber);
            item->setText(2, description);
            item->setText(3, vendorId);
            item->setText(4, productId);
        }

        for (auto it = currentDevices.begin(); it != currentDevices.end();) {
            if (!detectedKeys.contains(it.key())) {
                delete it.value();
                it = currentDevices.erase(it);
            } else {
                ++it;
            }
        }
    }

    QString readUsbInfo(const QString &devicePath, const QString &fileName) {
        QFile file(devicePath + "/" + fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            return in.readLine().trimmed();
        }
        return "";
    }
};

USBInterface* createSysFS() {
    return new USBSysFS();
}

#include "usb_sysfs.moc"
