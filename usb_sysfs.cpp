#include "usb_interface.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QHeaderView>
#include <QIcon>
#include <QMap>
#include <QPointer>
#include <QSettings>
#include <QSet>
#include <QStyle>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>

class USBSysFS : public QObject, public USBInterface {
    Q_OBJECT

public:
    USBSysFS() {
        loadClassGroupExpandedState();
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &USBSysFS::refreshUSBTree);
        timer->start(1000);
    }

    void populateUSBTree(QTreeWidget *tree, QStatusBar *bar = nullptr) override {
        usbTree = tree;
        statusBar = bar;
        connect(usbTree, &QTreeWidget::itemExpanded, this, &USBSysFS::onItemExpanded, Qt::UniqueConnection);
        connect(usbTree, &QTreeWidget::itemCollapsed, this, &USBSysFS::onItemCollapsed, Qt::UniqueConnection);
        applyHeadersForMode();
        refreshUSBTree();
    }

    void setViewMode(USBViewMode mode) override {
        viewMode = mode;
        applyHeadersForMode();
        renderTreeFromState(QDateTime::currentMSecsSinceEpoch());

        if (usbTree && usbTree->isSortingEnabled()) {
            usbTree->sortItems(usbTree->sortColumn(), usbTree->header()->sortIndicatorOrder());
        }
    }

private:
    static constexpr qint64 HighlightDurationMs = 5000;
    static constexpr auto kConfigFile = ".config/viusb/viusb.conf";
    static constexpr auto kClassesExpandedPrefix = "ui/classes_expanded";

    struct DeviceEntry {
        bool present = true;
        qint64 newUntilMs = 0;
        qint64 removedUntilMs = 0;

        QString busNumber;
        QString deviceNumber;
        QString description;
        QString vendorId;
        QString productId;
        QString deviceClass;
        QString deviceSubClass;
        QString deviceProtocol;
        QStringList serialPorts;
    };

    QPointer<QTreeWidget> usbTree;
    QPointer<QStatusBar> statusBar;
    QTimer *timer = nullptr;
    QMap<QString, DeviceEntry> currentDevices;
    QMap<QString, bool> classGroupExpandedState;
    bool initialScanDone = false;
    qint64 lastRefreshMs = 0;
    USBViewMode viewMode = USBViewMode::Classic;

    QString classStateKey(const QString &groupLabel) const {
        const QByteArray encoded =
            groupLabel.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        return QString("%1/%2").arg(kClassesExpandedPrefix, QString::fromLatin1(encoded));
    }

    void persistClassGroupExpandedState(const QString &groupLabel, bool expanded) {
        QSettings settings(QDir::home().filePath(kConfigFile), QSettings::IniFormat);
        settings.setValue(classStateKey(groupLabel), expanded);
        settings.sync();
    }

    void loadClassGroupExpandedState() {
        QSettings settings(QDir::home().filePath(kConfigFile), QSettings::IniFormat);
        settings.beginGroup(kClassesExpandedPrefix);
        const QStringList encodedKeys = settings.childKeys();
        for (const QString &encoded : encodedKeys) {
            const QByteArray decoded = QByteArray::fromBase64(encoded.toLatin1(), QByteArray::Base64UrlEncoding);
            if (decoded.isEmpty()) {
                continue;
            }
            const QString groupLabel = QString::fromUtf8(decoded);
            classGroupExpandedState[groupLabel] = settings.value(encoded, true).toBool();
        }
        settings.endGroup();
    }

    bool isClassGroupItem(const QTreeWidgetItem *item) const {
        return viewMode == USBViewMode::Classes && item && item->parent() == nullptr;
    }

    void onItemExpanded(QTreeWidgetItem *item) {
        if (!isClassGroupItem(item)) {
            return;
        }
        const QString label = item->text(0);
        classGroupExpandedState[label] = true;
        persistClassGroupExpandedState(label, true);
    }

    void onItemCollapsed(QTreeWidgetItem *item) {
        if (!isClassGroupItem(item)) {
            return;
        }
        const QString label = item->text(0);
        classGroupExpandedState[label] = false;
        persistClassGroupExpandedState(label, false);
    }

    void setStatusMessage(const QString &message) {
        if (statusBar) {
            statusBar->showMessage(message);
        }
    }

    void clearTreeAndState() {
        currentDevices.clear();
        if (usbTree) {
            usbTree->clear();
        }
    }

    void applyHeadersForMode() {
        if (!usbTree) {
            return;
        }

        usbTree->setHeaderLabels({"Bus", "Device", "Description", "Vendor ID", "Product ID"});
        usbTree->setRootIsDecorated(true);
        usbTree->setSortingEnabled(viewMode == USBViewMode::Classic);

        auto *header = usbTree->header();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(2, QHeaderView::Stretch);
        header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    }

    QString normalizeHexByte(const QString &value) {
        bool ok = false;
        const int parsed = value.toInt(&ok, 16);
        if (!ok) {
            return "";
        }
        return QString("%1").arg(parsed, 2, 16, QChar('0')).toUpper();
    }

    QString classCodeName(const QString &classCode) {
        bool ok = false;
        const int code = classCode.toInt(&ok, 16);
        if (!ok) {
            return "Unknown";
        }

        switch (code) {
            case 0x00: return "Per Interface";
            case 0x01: return "Audio";
            case 0x02: return "Communications";
            case 0x03: return "Human Interface Devices";
            case 0x05: return "Physical";
            case 0x06: return "Imaging Devices";
            case 0x07: return "Printers";
            case 0x08: return "Mass Storage";
            case 0x09: return "USB Hubs";
            case 0x0A: return "CDC-Data";
            case 0x0B: return "Smart Card";
            case 0x0D: return "Content Security";
            case 0x0E: return "Video Devices";
            case 0x0F: return "Personal Healthcare";
            case 0x10: return "Audio/Video";
            case 0x11: return "Billboard";
            case 0x12: return "USB-C Bridge";
            case 0xDC: return "Diagnostic";
            case 0xE0: return "Wireless Controller";
            case 0xEF: return "Miscellaneous";
            case 0xFE: return "Application Specific";
            case 0xFF: return "Vendor Specific";
            default: return "Unknown";
        }
    }

    QString classGroupName(const DeviceEntry &entry) {
        if (entry.deviceClass.isEmpty()) {
            return "Unknown Class";
        }
        return QString("%1 (%2)").arg(classCodeName(entry.deviceClass), entry.deviceClass);
    }

    QString classIconThemeName(const QString &classCode) const {
        bool ok = false;
        const int code = classCode.toInt(&ok, 16);
        if (!ok) {
            return "dialog-question";
        }

        switch (code) {
            case 0x01: return "audio-card";
            case 0x03: return "input-keyboard";
            case 0x06: return "camera-photo";
            case 0x07: return "printer";
            case 0x08: return "drive-removable-media-usb";
            case 0x09: return "network-wired";
            case 0x0E: return "camera-video";
            case 0xE0: return "network-wireless";
            case 0xFF: return "applications-system";
            default: return "usb-creator-gtk";
        }
    }

    QIcon classGroupIcon(const DeviceEntry &entry) const {
        QIcon icon = QIcon::fromTheme(classIconThemeName(entry.deviceClass));
        if (!icon.isNull()) {
            return icon;
        }

        if (!qApp) {
            return QIcon();
        }

        QStyle *style = qApp->style();
        if (!style) {
            return QIcon();
        }
        return style->standardIcon(QStyle::SP_DirIcon);
    }

    void fillDeviceColumns(QTreeWidgetItem *item, const DeviceEntry &entry) {
        item->setText(0, entry.busNumber);
        item->setText(1, entry.deviceNumber);
        item->setText(2, entry.description);
        item->setText(3, entry.vendorId);
        item->setText(4, entry.productId);
    }

    bool isSerialPortName(const QString &name) const {
        return name.startsWith("ttyACM") || name.startsWith("ttyUSB");
    }

    QStringList detectSerialPorts(const QString &devicePath) const {
        QSet<QString> uniquePorts;
        QDirIterator it(devicePath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QString name = it.fileInfo().fileName();
            if (!isSerialPortName(name)) {
                continue;
            }
            uniquePorts.insert("/dev/" + name);
        }

        QStringList ports = uniquePorts.values();
        std::sort(ports.begin(), ports.end());
        return ports;
    }

    void syncSerialPortChildren(QTreeWidgetItem *deviceItem, const QStringList &ports) {
        if (!deviceItem) {
            return;
        }

        QMap<QString, QTreeWidgetItem *> existing;
        const int childCount = deviceItem->childCount();
        for (int i = 0; i < childCount; ++i) {
            QTreeWidgetItem *child = deviceItem->child(i);
            if (!child) {
                continue;
            }
            const QString port = child->data(0, Qt::UserRole + 1).toString();
            if (!port.isEmpty()) {
                existing[port] = child;
            }
        }

        for (const QString &port : ports) {
            QTreeWidgetItem *child = existing.value(port, nullptr);
            if (!child) {
                child = new QTreeWidgetItem(deviceItem);
                child->setData(0, Qt::UserRole + 1, port);
            }
            child->setText(0, "");
            child->setText(1, "");
            child->setText(2, port);
            child->setText(3, "");
            child->setText(4, "");
        }

        for (auto it = existing.begin(); it != existing.end(); ++it) {
            if (ports.contains(it.key())) {
                continue;
            }
            const int index = deviceItem->indexOfChild(it.value());
            if (index >= 0) {
                delete deviceItem->takeChild(index);
            }
        }

        deviceItem->setChildIndicatorPolicy(
            ports.isEmpty() ? QTreeWidgetItem::DontShowIndicator : QTreeWidgetItem::ShowIndicator);
    }

    QMap<QString, QTreeWidgetItem *> collectClassGroups() const {
        QMap<QString, QTreeWidgetItem *> groups;
        if (!usbTree || viewMode != USBViewMode::Classes) {
            return groups;
        }

        const int count = usbTree->topLevelItemCount();
        for (int i = 0; i < count; ++i) {
            QTreeWidgetItem *group = usbTree->topLevelItem(i);
            if (!group) {
                continue;
            }
            groups[group->text(0)] = group;
        }
        return groups;
    }

    QMap<QString, QTreeWidgetItem *> collectRenderedDeviceItems() const {
        QMap<QString, QTreeWidgetItem *> items;
        if (!usbTree) {
            return items;
        }

        const int topCount = usbTree->topLevelItemCount();
        for (int i = 0; i < topCount; ++i) {
            QTreeWidgetItem *top = usbTree->topLevelItem(i);
            if (!top) {
                continue;
            }

            if (viewMode == USBViewMode::Classic) {
                const QString key = top->data(0, Qt::UserRole).toString();
                if (!key.isEmpty()) {
                    items[key] = top;
                }
                continue;
            }

            const int childCount = top->childCount();
            for (int j = 0; j < childCount; ++j) {
                QTreeWidgetItem *child = top->child(j);
                if (!child) {
                    continue;
                }
                const QString key = child->data(0, Qt::UserRole).toString();
                if (!key.isEmpty()) {
                    items[key] = child;
                }
            }
        }
        return items;
    }

    void ensureGroupStyle(QTreeWidgetItem *groupItem) const {
        if (!groupItem) {
            return;
        }

        groupItem->setFirstColumnSpanned(true);
        QFont font = groupItem->font(0);
        font.setBold(true);
        groupItem->setFont(0, font);
    }

    void pruneEmptyGroups(const QMap<QString, QTreeWidgetItem *> &groups) {
        for (auto it = groups.begin(); it != groups.end(); ++it) {
            QTreeWidgetItem *groupItem = it.value();
            if (!groupItem || groupItem->childCount() > 0 || !usbTree) {
                continue;
            }

            const int index = usbTree->indexOfTopLevelItem(groupItem);
            if (index >= 0) {
                delete usbTree->takeTopLevelItem(index);
            }
        }
    }

    void setRowStyle(QTreeWidgetItem *item, const QBrush &background, const QBrush &foreground) {
        if (!usbTree || !item) {
            return;
        }

        const int columnCount = usbTree->columnCount();
        for (int i = 0; i < columnCount; ++i) {
            item->setBackground(i, background);
            item->setForeground(i, foreground);
        }
    }

    enum class HighlightState {
        None,
        New,
        Removed
    };

    HighlightState highlightStateAt(const DeviceEntry &entry, qint64 atMs) const {
        if (!entry.present) {
            return HighlightState::Removed;
        }
        if (entry.newUntilMs > atMs) {
            return HighlightState::New;
        }
        return HighlightState::None;
    }

    void captureClassesExpansionState() {
        if (!usbTree || viewMode != USBViewMode::Classes) {
            return;
        }

        const int count = usbTree->topLevelItemCount();
        for (int i = 0; i < count; ++i) {
            QTreeWidgetItem *group = usbTree->topLevelItem(i);
            if (!group) {
                continue;
            }
            classGroupExpandedState[group->text(0)] = group->isExpanded();
        }
    }

    void applyHighlight(QTreeWidgetItem *item, const DeviceEntry &entry, qint64 nowMs) {
        if (!item) {
            return;
        }

        if (!entry.present) {
            setRowStyle(item, QBrush(QColor(255, 210, 210)), QBrush(Qt::black));
            return;
        }

        if (entry.newUntilMs > nowMs) {
            setRowStyle(item, QBrush(QColor(210, 255, 210)), QBrush(Qt::black));
            return;
        }

        setRowStyle(item, QBrush(), QBrush());
    }

    void renderTreeFromState(qint64 nowMs) {
        if (!usbTree) {
            return;
        }

        captureClassesExpansionState();
        usbTree->clear();

        if (viewMode == USBViewMode::Classic) {
            for (auto it = currentDevices.begin(); it != currentDevices.end(); ++it) {
                auto *item = new QTreeWidgetItem(usbTree);
                fillDeviceColumns(item, *it);
                item->setData(0, Qt::UserRole, it.key());
                syncSerialPortChildren(item, it->serialPorts);
                applyHighlight(item, *it, nowMs);
            }
            return;
        }

        QMap<QString, QTreeWidgetItem *> groups;
        for (auto it = currentDevices.begin(); it != currentDevices.end(); ++it) {
            const QString groupLabel = classGroupName(*it);

            QTreeWidgetItem *groupItem = groups.value(groupLabel, nullptr);
            if (!groupItem) {
                groupItem = new QTreeWidgetItem(usbTree);
                groupItem->setText(0, groupLabel);
                groupItem->setIcon(0, classGroupIcon(*it));
                groupItem->setFirstColumnSpanned(true);

                QFont font = groupItem->font(0);
                font.setBold(true);
                groupItem->setFont(0, font);

                groups.insert(groupLabel, groupItem);
            }

            auto *item = new QTreeWidgetItem(groupItem);
            fillDeviceColumns(item, *it);
            item->setData(0, Qt::UserRole, it.key());
            syncSerialPortChildren(item, it->serialPorts);
            applyHighlight(item, *it, nowMs);
        }

        for (auto it = groups.begin(); it != groups.end(); ++it) {
            const bool expanded = classGroupExpandedState.value(it.key(), true);
            it.value()->setExpanded(expanded);
        }
    }

    void renderClassesIncremental(qint64 nowMs) {
        if (!usbTree || viewMode != USBViewMode::Classes) {
            return;
        }

        captureClassesExpansionState();
        auto groups = collectClassGroups();
        auto renderedItems = collectRenderedDeviceItems();

        for (auto it = currentDevices.begin(); it != currentDevices.end(); ++it) {
            const QString key = it.key();
            const QString groupLabel = classGroupName(*it);

            QTreeWidgetItem *groupItem = groups.value(groupLabel, nullptr);
            if (!groupItem) {
                groupItem = new QTreeWidgetItem(usbTree);
                groupItem->setText(0, groupLabel);
                groupItem->setIcon(0, classGroupIcon(*it));
                ensureGroupStyle(groupItem);
                groupItem->setExpanded(classGroupExpandedState.value(groupLabel, true));
                groups.insert(groupLabel, groupItem);
            }

            QTreeWidgetItem *item = renderedItems.value(key, nullptr);
            if (!item) {
                item = new QTreeWidgetItem(groupItem);
                item->setData(0, Qt::UserRole, key);
                renderedItems.insert(key, item);
            } else if (item->parent() != groupItem) {
                QTreeWidgetItem *oldParent = item->parent();
                if (oldParent) {
                    oldParent->removeChild(item);
                } else if (usbTree) {
                    const int oldIndex = usbTree->indexOfTopLevelItem(item);
                    if (oldIndex >= 0) {
                        usbTree->takeTopLevelItem(oldIndex);
                    }
                }
                groupItem->addChild(item);
            }

            fillDeviceColumns(item, *it);
            syncSerialPortChildren(item, it->serialPorts);
            applyHighlight(item, *it, nowMs);
        }

        for (auto it = renderedItems.begin(); it != renderedItems.end(); ++it) {
            if (currentDevices.contains(it.key())) {
                continue;
            }

            QTreeWidgetItem *item = it.value();
            if (!item) {
                continue;
            }

            QTreeWidgetItem *parent = item->parent();
            if (parent) {
                delete parent->takeChild(parent->indexOfChild(item));
            } else if (usbTree) {
                const int index = usbTree->indexOfTopLevelItem(item);
                if (index >= 0) {
                    delete usbTree->takeTopLevelItem(index);
                }
            }
        }

        pruneEmptyGroups(groups);
    }

    void refreshUSBTree() {
        if (!usbTree) {
            return;
        }

        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        QDir dir("/sys/bus/usb/devices");
        const QFileInfo sysFsInfo(dir.absolutePath());
        if (!dir.exists() || !sysFsInfo.isReadable()) {
            clearTreeAndState();
            setStatusMessage("USB sysfs is unavailable or not readable: /sys/bus/usb/devices");
            return;
        }

        const QStringList devices = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QSet<QString> detectedKeys;
        int missingAddressCount = 0;
        bool stateChanged = false;

        for (const QString &device : devices) {
            const QString devicePath = dir.absoluteFilePath(device);

            const QString vendorId = readUsbInfo(devicePath, "idVendor");
            const QString productId = readUsbInfo(devicePath, "idProduct");
            const QString manufacturer = readUsbInfo(devicePath, "manufacturer");
            const QString product = readUsbInfo(devicePath, "product");

            bool busOk = false;
            bool devOk = false;
            const int busNum = readUsbInfo(devicePath, "busnum").toInt(&busOk);
            const int devNum = readUsbInfo(devicePath, "devnum").toInt(&devOk);

            if (vendorId.isEmpty() || productId.isEmpty()) {
                continue;
            }

            QString busNumber;
            QString deviceNumber;
            QString key;

            if (busOk && devOk) {
                busNumber = QString("%1").arg(busNum, 3, 10, QChar('0'));
                deviceNumber = QString("%1").arg(devNum, 3, 10, QChar('0'));
                key = QString("%1-%2").arg(busNumber, deviceNumber);
            } else {
                ++missingAddressCount;
                busNumber = "---";
                deviceNumber = "---";
                key = device;
            }

            const QString description = (!product.isEmpty() && !manufacturer.isEmpty())
                                            ? QString("%1 %2").arg(manufacturer, product)
                                            : (!manufacturer.isEmpty() ? manufacturer
                                                                       : (!product.isEmpty() ? product : "Unknown"));

            detectedKeys.insert(key);

            auto it = currentDevices.find(key);
            if (it == currentDevices.end()) {
                DeviceEntry entry;
                entry.present = true;
                entry.newUntilMs = initialScanDone ? (nowMs + HighlightDurationMs) : 0;
                entry.removedUntilMs = 0;
                it = currentDevices.insert(key, entry);
                stateChanged = true;
            } else if (!it->present) {
                it->present = true;
                it->newUntilMs = nowMs + HighlightDurationMs;
                it->removedUntilMs = 0;
                stateChanged = true;
            }

            const QString nextClass = normalizeHexByte(readUsbInfo(devicePath, "bDeviceClass"));
            const QString nextSubClass = normalizeHexByte(readUsbInfo(devicePath, "bDeviceSubClass"));
            const QString nextProtocol = normalizeHexByte(readUsbInfo(devicePath, "bDeviceProtocol"));
            const QStringList nextSerialPorts = detectSerialPorts(devicePath);

            if (it->busNumber != busNumber || it->deviceNumber != deviceNumber || it->description != description ||
                it->vendorId != vendorId || it->productId != productId || it->deviceClass != nextClass ||
                it->deviceSubClass != nextSubClass || it->deviceProtocol != nextProtocol ||
                it->serialPorts != nextSerialPorts) {
                stateChanged = true;
            }

            it->busNumber = busNumber;
            it->deviceNumber = deviceNumber;
            it->description = description;
            it->vendorId = vendorId;
            it->productId = productId;
            it->deviceClass = nextClass;
            it->deviceSubClass = nextSubClass;
            it->deviceProtocol = nextProtocol;
            it->serialPorts = nextSerialPorts;
        }

        for (auto it = currentDevices.begin(); it != currentDevices.end(); ++it) {
            if (!detectedKeys.contains(it.key()) && it->present) {
                it->present = false;
                it->newUntilMs = 0;
                it->removedUntilMs = nowMs + HighlightDurationMs;
                stateChanged = true;
            }
        }

        int activeDevices = 0;
        int fadingOutDevices = 0;

        for (auto it = currentDevices.begin(); it != currentDevices.end();) {
            if (!it->present && it->removedUntilMs <= nowMs) {
                it = currentDevices.erase(it);
                stateChanged = true;
                continue;
            }

            if (it->present) {
                ++activeDevices;
            } else {
                ++fadingOutDevices;
            }
            ++it;
        }

        bool highlightStateChanged = false;
        for (auto it = currentDevices.begin(); it != currentDevices.end(); ++it) {
            if (highlightStateAt(*it, lastRefreshMs) != highlightStateAt(*it, nowMs)) {
                highlightStateChanged = true;
                break;
            }
        }

        if (stateChanged || highlightStateChanged || !initialScanDone) {
            if (viewMode == USBViewMode::Classes && initialScanDone) {
                renderClassesIncremental(nowMs);
            } else {
                renderTreeFromState(nowMs);
            }

            if (viewMode == USBViewMode::Classic && usbTree->isSortingEnabled()) {
                usbTree->sortItems(usbTree->sortColumn(), usbTree->header()->sortIndicatorOrder());
            }
        }

        const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
        QString message = QString("%1 USB device(s), refreshed %2").arg(activeDevices).arg(timestamp);
        if (fadingOutDevices > 0) {
            message += QString(", %1 recently disconnected").arg(fadingOutDevices);
        }
        if (missingAddressCount > 0) {
            message += QString(" (%1 with missing bus/device number)").arg(missingAddressCount);
        }
        setStatusMessage(message);
        lastRefreshMs = nowMs;
        initialScanDone = true;
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

USBInterface *createSysFS() {
    return new USBSysFS();
}

#include "usb_sysfs.moc"
