#pragma once
#include "hw/sysbus.h"
// TODO(zhuowei): fix pragma, fix this whole mess

#define TYPE_HX_AIC  "hx-aic"
#define HX_AIC(obj) OBJECT_CHECK(HxAICState, (obj), TYPE_HX_AIC)

typedef struct HxAICState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    MemoryRegion iomem;
    qemu_irq parent_fiq;
    qemu_irq parent_irq;
} HxAICState;