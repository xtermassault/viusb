#ifndef USB_INTERFACE_H
#define USB_INTERFACE_H

#include <QStatusBar>
#include <QTreeWidget>

enum class USBViewMode {
    Classic,
    Classes
};

class USBInterface {
public:
    virtual ~USBInterface() = default;
    virtual void populateUSBTree(QTreeWidget *usbTree, QStatusBar *statusBar = nullptr) = 0;
    virtual void setViewMode(USBViewMode viewMode) = 0;
};

USBInterface *createSysFS();

#endif // USB_INTERFACE_H
