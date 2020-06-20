#pragma once
#include "hw/sysbus.h"

typedef struct HxGpioState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    qemu_irq irq0;
} HxGpioState;

#define TYPE_HX_GPIO "hx_gpio"
#define HX_GPIO(obj) \
    OBJECT_CHECK(HxGpioState, (obj), TYPE_HX_GPIO)