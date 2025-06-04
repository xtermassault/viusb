#ifndef USB_INTERFACE_H
#define USB_INTERFACE_H

#include <QTreeWidget>

class USBInterface {
public:
    virtual ~USBInterface() = default;
    virtual void populateUSBTree(QTreeWidget *usbTree) = 0;
};

USBInterface* createSysFS();

#endif // USB_INTERFACE_H
