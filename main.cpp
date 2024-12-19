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
    explicit USBWidget(QWidget *parent = nullptr) : QWidget(parent), firstRun(true) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        usbTree = new QTreeWidget(this);
        usbTree->setHeaderLabels(QStringList() << "Device" << "Bus" << "Vendor ID" << "Product ID" << "Serial Device");
        layout->addWidget(usbTree);
        usbTree->setColumnWidth(0, 400);

        QPalette palette = usbTree->palette();
        palette.setColor(QPalette::Base, QColor(53, 53, 53));
        palette.setColor(QPalette::Text, Qt::white);
        usbTree->setPalette(palette);

        QTimer* updateTimer;
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &USBWidget::refreshUSBInfo);
        updateTimer->start(200);

        previousDevices = QMap<QString, QTreeWidgetItem*>();
    }

private slots:
    void refreshUSBInfo() {
        QProcess lsusbProcess;
        lsusbProcess.start("lsusb");
        lsusbProcess.waitForFinished();
        QString output = lsusbProcess.readAllStandardOutput();

        QProcess serialProcess;
        serialProcess.start("bash", {"-c", "ls /dev/ttyACM* 2>/dev/null && ls /dev/ttyUSB* 2>/dev/null "});
        serialProcess.waitForFinished();
        QString serialOutput = serialProcess.readAllStandardOutput();
        QMap<QString, QString> serialDevices = parseSerialDevices(serialOutput);

        QMap<QString, QStringList> currentDevices = parseLsusbOutput(output);

        updateDeviceTree(currentDevices, serialDevices);
    }

private:
    static QMap<QString, QStringList> parseLsusbOutput(const QString &output) {
        QMap<QString, QStringList> devices;

        QStringList lines;
        lines = output.split("\n", QString::SkipEmptyParts);
        QRegularExpression regex(R"(Bus (\d{3}) Device (\d{3}): ID (\w{4}):(\w{4}) (.+))");

        for (const QString &line : lines) {
            QRegularExpressionMatch match;
            match = regex.match(line);
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

    static QMap<QString, QString> parseSerialDevices(const QString &output) {
        QMap<QString, QString> serialMap;
        QStringList devices = output.split("\n", QString::SkipEmptyParts);
        for (const QString &device : devices) {
            QProcess udevProcess;
            udevProcess.start("udevadm", {"info", "-q", "property", "-n", device});
            udevProcess.waitForFinished();

            QString udevOutput = udevProcess.readAllStandardOutput();
            QRegularExpression regex(R"(BUSNUM=(\d+)\nDEVNUM=(\d+))");
            QRegularExpressionMatch match = regex.match(udevOutput);

            if (match.hasMatch()) {
                QString bus = match.captured(1).rightJustified(3, '0');
                QString dev = match.captured(2).rightJustified(3, '0');
                QString key = bus + "_" + dev;
                serialMap[key] = device;
            }
        }
        return serialMap;
    }

    void updateDeviceTree(const QMap<QString, QStringList>& currentDevices, const QMap<QString, QString>& serialDevices) {
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
            previousDevices[key]->setBackground(4, Qt::red);
            QTimer::singleShot(5000, this, [this, key]() {
                delete previousDevices[key];
                previousDevices.remove(key);
            });
        }

        for (const QString& key : newDevices) {
            QStringList deviceInfo = currentDevices[key];
            QString serialDevice = serialDevices.value(key, "N/A");

            auto *item = new QTreeWidgetItem(usbTree);
            item->setText(0, deviceInfo[0]);
            item->setText(1, deviceInfo[1]);
            item->setText(2, deviceInfo[2]);
            item->setText(3, deviceInfo[3]);
            item->setText(4, serialDevice);

            if (serialDevice == "N/A") {
                item->setForeground(4, Qt::red);
            }


            if (!firstRun) {
                item->setBackground(0, Qt::green);
                item->setBackground(1, Qt::green);
                item->setBackground(2, Qt::green);
                item->setBackground(3, Qt::green);
                item->setBackground(4, Qt::green);
                QTimer::singleShot(5000, [item]() {
                    item->setBackground(0, QColor(53, 53, 53));
                    item->setBackground(1, QColor(53, 53, 53));
                    item->setBackground(2, QColor(53, 53, 53));
                    item->setBackground(3, QColor(53, 53, 53));
                    item->setBackground(4, QColor(53, 53, 53));
                    item->setForeground(0, Qt::white);
                    item->setForeground(1, Qt::white);
                    item->setForeground(2, Qt::white);
                    item->setForeground(3, Qt::white);
                    item->setForeground(4, Qt::white);
                });
            }

            item->setForeground(0, Qt::white);
            item->setForeground(1, Qt::white);
            item->setForeground(2, Qt::white);
            item->setForeground(3, Qt::white);
            item->setForeground(4, Qt::white);

            previousDevices[key] = item;
        }

        firstRun = false;
    }

private:
    QTreeWidget *usbTree;
    QMap<QString, QTreeWidgetItem*> previousDevices;
    bool firstRun;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    USBWidget widget;
    widget.setWindowTitle("viusb");
    widget.resize(960, 500);
    widget.setFixedSize(widget.size());
    widget.setWindowFlags(widget.windowFlags() & ~Qt::WindowMaximizeButtonHint);

    widget.show();

    return app.exec();
}
