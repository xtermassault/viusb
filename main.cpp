#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTimer>
#include <QProcess>
#include <QRegularExpression>
#include <QMap>
#include <QPalette>

class USBWidget : public QWidget {
public:
    USBWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        usbTree = new QTreeWidget(this);
        usbTree->setHeaderLabels(QStringList() << "Device" << "Bus" << "Vendor ID" << "Product ID");
        layout->addWidget(usbTree);
        usbTree->setColumnWidth(0, 510);
        QTimer *updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &USBWidget::refreshUSBInfo);
        updateTimer->start(20);
        previousDevices = QMap<QString, QTreeWidgetItem*>();
    }

private slots:
    void refreshUSBInfo() {
        QProcess lsusbProcess;
        lsusbProcess.start("lsusb");
        lsusbProcess.waitForFinished();

        QString output = lsusbProcess.readAllStandardOutput();
        QMap<QString, QStringList> currentDevices = parseLsusbOutput(output);
        updateDeviceTree(currentDevices);
    }

private:
    QMap<QString, QStringList> parseLsusbOutput(const QString &output) {
        QMap<QString, QStringList> devices;

        QStringList lines = output.split("\n", QString::SkipEmptyParts);
        QRegularExpression regex(R"(Bus (\d{3}) Device (\d{3}): ID (\w{4}):(\w{4}) (.+))");

        for (const QString &line : lines) {
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                QString bus = match.captured(1);
                QString device = match.captured(2);
                QString vendorId = match.captured(3);
                QString productId = match.captured(4);
                QString deviceName = match.captured(5);

                QString key = bus + "_" + device;

                devices[key] = QStringList() << deviceName << bus << vendorId << productId;
            }
        }
        return devices;
    }

    void updateDeviceTree(const QMap<QString, QStringList>& currentDevices) {
        QList<QString> newDevices;
        QList<QString> removedDevices;
        for (const QString& key : currentDevices.keys()) {
            if (!previousDevices.contains(key)) {
                newDevices.append(key);
            }
        }
        for (const QString& key : previousDevices.keys()) {
            if (!currentDevices.contains(key)) {
                removedDevices.append(key);
            }
        }
        for (const QString& key : removedDevices) {
            previousDevices[key]->setBackground(0, Qt::red);
            previousDevices[key]->setBackground(1, Qt::red);
            previousDevices[key]->setBackground(2, Qt::red);
            previousDevices[key]->setBackground(3, Qt::red);
            QTimer::singleShot(5000, this, [this, key]() {
                delete previousDevices[key];
                previousDevices.remove(key);
            });
        }
        for (const QString& key : newDevices) {
            QStringList deviceInfo = currentDevices[key];
            QTreeWidgetItem *item = new QTreeWidgetItem(usbTree);
            item->setText(0, deviceInfo[0]);
            item->setText(1, deviceInfo[1]);
            item->setText(2, deviceInfo[2]);
            item->setText(3, deviceInfo[3]);
            item->setBackground(0, Qt::green);
            item->setBackground(1, Qt::green);
            item->setBackground(2, Qt::green);
            item->setBackground(3, Qt::green);

            previousDevices[key] = item;
            QTimer::singleShot(5000, this, [item]() {
                item->setBackground(0, Qt::white);
                item->setBackground(1, Qt::white);
                item->setBackground(2, Qt::white);
                item->setBackground(3, Qt::white);
            });
        }
    }

private:
    QTreeWidget *usbTree;
    QMap<QString, QTreeWidgetItem*> previousDevices;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    USBWidget widget;
    widget.setWindowTitle("USB Devices");
    widget.resize(840, 500);
    widget.setFixedSize(widget.size());
    widget.setWindowFlags(widget.windowFlags() & ~Qt::WindowMaximizeButtonHint);

    widget.show();

    return app.exec();
}

