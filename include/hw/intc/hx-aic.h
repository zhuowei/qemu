#pragma once
#include "hw/sysbus.h"
// TODO(zhuowei): fix pragma, fix this whole mess

#define TYPE_HX_AIC  "hx-aic"
#define HX_AIC(obj) OBJECT_CHECK(HxAICState, (obj), TYPE_HX_AIC)

#define HX_AIC_INT_NR 256
#define HX_AIC_REG_NUM      DIV_ROUND_UP(HX_AIC_INT_NR, 32)

typedef struct HxAICState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    MemoryRegion iomem;
    qemu_irq parent_fiq;
    qemu_irq parent_irq;

    uint32_t vector;
    uint32_t irq_pending[HX_AIC_REG_NUM];
    uint32_t enable[HX_AIC_REG_NUM];
} HxAICState;