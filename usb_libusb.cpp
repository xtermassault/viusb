#include "usb_interface.h"
#include <libusb-1.0/libusb.h>
#include <QTreeWidget>
#include <QDebug>

class USBLibUSB : public USBInterface {
public:
    void populateUSBTree(QTreeWidget *usbTree) override {
        libusb_context *ctx = nullptr;
        libusb_device **devices = nullptr;
        ssize_t count = 0;

        if (libusb_init(&ctx) < 0) {
            qWarning() << "Failed to initialize libusb";
            return;
        }

        count = libusb_get_device_list(ctx, &devices);
        if (count < 0) {
            libusb_exit(ctx);
            qWarning() << "Failed to get USB device list";
            return;
        }

        for (ssize_t i = 0; i < count; ++i) {
            libusb_device *device = devices[i];
            libusb_device_descriptor desc;

            if (libusb_get_device_descriptor(device, &desc) == 0) {
                QString vendorId = QString::number(desc.idVendor, 16).toUpper();
                QString productId = QString::number(desc.idProduct, 16).toUpper();
                QString description = QString("Bus %1, Device %2")
                                          .arg(libusb_get_bus_number(device))
                                          .arg(libusb_get_device_address(device));

                QString manufacturer = "Unknown";
                QString product = "Unknown";

                libusb_device_handle *handle = nullptr;
                if (libusb_open(device, &handle) == 0) {
                    char buffer[256];

                    if (desc.iManufacturer && libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, (unsigned char *)buffer, sizeof(buffer)) > 0) {
                        manufacturer = QString(buffer);
                    }

                    if (desc.iProduct && libusb_get_string_descriptor_ascii(handle, desc.iProduct, (unsigned char *)buffer, sizeof(buffer)) > 0) {
                        product = QString(buffer);
                    }

                    libusb_close(handle);
                }

                QTreeWidgetItem *item = new QTreeWidgetItem(usbTree);
                item->setText(0, vendorId);
                item->setText(1, productId);
                item->setText(2, QString("%1 (%2)").arg(product, manufacturer));
            }
        }

        libusb_free_device_list(devices, 1);
        libusb_exit(ctx);
    }
};

USBInterface* createLibUSB() {
    return new USBLibUSB();
}
