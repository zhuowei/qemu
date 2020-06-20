#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "qapi/error.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "hw/gpio/hx_gpio.h"
#include "hw/irq.h"

static uint64_t hx_gpio_read(void *opaque, hwaddr offset, unsigned size)
{
    fprintf(stderr, "hx_gpio_read: %llx %x\n", offset, size);
}

static void hx_gpio_write(void *opaque, hwaddr offset,
        uint64_t value, unsigned size)
{
    fprintf(stderr, "hx_gpio_write: %llx %llx %x\n", offset, value, size);
}

static const MemoryRegionOps hx_gpio_ops = {
    .read = hx_gpio_read,
    .write = hx_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void hx_gpio_init(Object *obj)
{
    HxGpioState *s = HX_GPIO(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj,
            &hx_gpio_ops, s, "hx_gpio", 0x100000);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void hx_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    // TODO(zhuowei)
    //dc->vmsd = &vmstate_hx_gpio;
    //dc->realize = &hx_gpio_realize;
    //dc->reset = &hx_gpio_reset;
}

static const TypeInfo hx_gpio_info = {
    .name          = TYPE_HX_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(HxGpioState),
    .instance_init = hx_gpio_init,
    .class_init    = hx_gpio_class_init,
};

static void hx_gpio_register_types(void)
{
    type_register_static(&hx_gpio_info);
}

type_init(hx_gpio_register_types)
