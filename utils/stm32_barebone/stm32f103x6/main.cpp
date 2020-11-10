#include "stm32f1xx.h"
#include "src/serial.cpp"

#include "ow/ow.h"
#include "ow/devices/ow_device_ds18x20.h"
#include "common/onewire_uart/src/system/ow_ll_stm32.c"
#include "scan_devices.h"

extern const ow_ll_drv_t ow_ll_drv_stm32;
ow_t ow;
ow_rom_t rom_ids[20];
size_t rom_found;

int main() {
    serial_setup(19200);
    serial_print("INFO: Starting program!\r\n");

    ow_init(&ow, &ow_ll_drv_stm32, NULL);
    serial_print("INFO: ow_init done\r\n");

    owr_t res = ow_search_devices(&ow, rom_ids, OW_ARRAYSIZE(rom_ids), &rom_found);
    serial_print("INFO: ow_search_devices done\r\n");

    // if (scan_onewire_devices(&ow, rom_ids, OW_ARRAYSIZE(rom_ids), &rom_found) == owOK) {
    //         serial_print("Devices scanned, found devices!\r\n");
    //     } else {
    //         serial_print("Device scan error\r\n");
    //     }

    while(true) {
        asm("");
    }
}
