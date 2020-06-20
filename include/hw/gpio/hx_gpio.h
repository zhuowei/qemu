#pragma once
#include "hw/sysbus.h"

#define HX_GPIO_COUNT 224

typedef struct HxGpioState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    uint32_t status[HX_GPIO_COUNT];
    qemu_irq irq0;
} HxGpioState;

#define TYPE_HX_GPIO "hx_gpio"
#define HX_GPIO(obj) \
    OBJECT_CHECK(HxGpioState, (obj), TYPE_HX_GPIO)