#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "hw/intc/hx-aic.h"
#include "hw/irq.h"
#include "qemu/log.h"
#include "qemu/module.h"

// Based on https://github.com/corellium/linux-sandcastle/blob/0c2f7dda13d67bb7e06123df516f8bdb1000a79b/drivers/irqchip/irq-hx-aic.c
// also on allwinner-a10-pic.c

#define REG_ID_REVISION                 0x0000
#define REG_ID_CONFIG                   0x0004
#define REG_GLOBAL_CFG                  0x0010
#define REG_TIME_LO                     0x0020
#define REG_TIME_HI                     0x0028
#define REG_ID_CPUID                    0x2000
#define REG_IRQ_ACK                     0x2004
#define  REG_IRQ_ACK_TYPE_MASK          (15 << 16)
#define   REG_IRQ_ACK_TYPE_NONE         (0 << 16)
#define   REG_IRQ_ACK_TYPE_IRQ          (1 << 16)
#define   REG_IRQ_ACK_TYPE_IPI          (4 << 16)
#define    REG_IRQ_ACK_IPI_OTHER        0x40001
#define    REG_IRQ_ACK_IPI_SELF         0x40002
#define  REG_IRQ_ACK_NUM_MASK           (4095)
#define REG_IPI_SET                     0x2008
#define   REG_IPI_FLAG_SELF             (1 << 31)
#define   REG_IPI_FLAG_OTHER            (1 << 0)
#define REG_IPI_CLEAR                   0x200C
#define REG_IPI_DISABLE                 0x2024
#define REG_IPI_ENABLE                  0x2028
#define REG_IPI_DEFER_SET               0x202C
#define REG_IPI_DEFER_CLEAR             0x2030
#define REG_TSTAMP_CTRL                 0x2040
#define REG_TSTAMP_LO                   0x2048
#define REG_TSTAMP_HI                   0x204C
#define REG_IRQ_AFFINITY(i)             (0x3000 + ((i) << 2))
#define REG_IRQ_DISABLE(i)              (0x4100 + (((i) >> 5) << 2))
#define  REG_IRQ_xABLE_MASK(i)          (1 << ((i) & 31))
#define REG_IRQ_ENABLE(i)               (0x4180 + (((i) >> 5) << 2))
#define REG_CPU_REGION                  0x5000
#define  REG_CPU_LOCAL                  0x2000
#define  REG_CPU_SHIFT                  7
#define  REG_PERCPU(r,c)                ((r)+REG_CPU_REGION-REG_CPU_LOCAL+((c)<<REG_CPU_SHIFT))

// TODO(zhuowei)
#define REG_IRQ_DISABLE_BASE 0x4100
#define REG_IRQ_ENABLE_BASE 0x4180

static void hx_aic_update(HxAICState *s) {
    uint8_t i;
    int irq = 0, zeroes;

    s->vector = 0;

    for (i = 0; false && i < HX_AIC_REG_NUM; i++) {
        irq |= s->irq_pending[i] & s->enable[i];
        //fiq |= s->select[i] & s->irq_pending[i] & ~s->mask[i];

        if (!s->vector) {
            zeroes = ctz32(s->irq_pending[i] & s->enable[i]);
            if (zeroes != 32) {
                s->vector = REG_IRQ_ACK_TYPE_IRQ | (i * 32 + zeroes);
            }
        }
    }
    if (s->vector) {
        // fprintf(stderr, "hx_aic_update: irq %x\n", s->vector);
    }

    qemu_set_irq(s->parent_irq, !!irq);
    //qemu_set_irq(s->parent_fiq, !!fiq);
}

static void hx_aic_set_irq(void *opaque, int irq, int level)
{
    // fprintf(stderr, "hx_aic_set_irq: %d %d\n", irq, level);
    HxAICState *s = opaque;

    if (level) {
        set_bit(irq % 32, (void *)&s->irq_pending[irq / 32]);
    } else {
        clear_bit(irq % 32, (void *)&s->irq_pending[irq / 32]);
    }
    hx_aic_update(s);
}

static uint64_t hx_aic_read(void *opaque, hwaddr offset, unsigned size)
{
    HxAICState *s = opaque;
    // TODO(zhuowei)
    switch (offset) {
        case REG_ID_CONFIG:
            return HX_AIC_INT_NR;
        case REG_IRQ_ACK:
            return s->vector;
        default:
            // fprintf(stderr, "hx_aic_read: %llx %x\n", offset, size);
            break;
    }
    return 0;
}

static void hx_aic_write(void *opaque, hwaddr offset, uint64_t value,
                             unsigned size)
{
    HxAICState *s = opaque;
    int index;
    // TODO(zhuowei)
    fprintf(stderr, "hx_aic_write: %llx %llx %x\n", offset, value, size);
    switch (offset) {
        case REG_IRQ_ENABLE_BASE ... REG_IRQ_ENABLE_BASE + (HX_AIC_REG_NUM << 2):
            index = (offset - REG_IRQ_ENABLE_BASE) >> 2;
            // fprintf(stderr, "enable irq set!\n");
            s->enable[index] |= value;
            break;
        case REG_IRQ_DISABLE_BASE ... REG_IRQ_DISABLE_BASE + (HX_AIC_REG_NUM << 2):
            // fprintf(stderr, "disable irq set!\n");
            index = (offset - REG_IRQ_DISABLE_BASE) >> 2;
            s->enable[index] &= ~value;
            break;
    }
    hx_aic_update(s);
}

static const MemoryRegionOps hx_aic_ops = {
    .read = hx_aic_read,
    .write = hx_aic_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void hx_aic_init(Object *obj)
{
    HxAICState *s = HX_AIC(obj);
    SysBusDevice *dev = SYS_BUS_DEVICE(obj);

    qdev_init_gpio_in(DEVICE(dev), hx_aic_set_irq, HX_AIC_INT_NR);
    sysbus_init_irq(dev, &s->parent_irq);
    memory_region_init_io(&s->iomem, OBJECT(s), &hx_aic_ops, s,
                            TYPE_HX_AIC, 0x8000);
    sysbus_init_mmio(dev, &s->iomem);
}

static void hx_aic_reset(DeviceState *d)
{
    HxAICState *s = HX_AIC(d);
    int i;
    for (i = 0; i < HX_AIC_REG_NUM; i++) {
        s->irq_pending[i] = 0;
        s->enable[i] = 0;
    }
}

static void hx_aic_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = hx_aic_reset;
    dc->desc = "Apple Interrupt Controller";
    // TODO(zhuowei) vmstate
    //dc->vmsd = &vmstate_hx_aic;
}

static const TypeInfo hx_aic_info = {
    .name = TYPE_HX_AIC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(HxAICState),
    .instance_init = hx_aic_init,
    .class_init = hx_aic_class_init,
};

static void hx_aic_register_types(void)
{
    type_register_static(&hx_aic_info);
}

type_init(hx_aic_register_types);