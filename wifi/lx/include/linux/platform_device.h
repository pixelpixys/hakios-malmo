#ifndef _LX_LINUX_PLATFORM_DEVICE_H_
#define _LX_LINUX_PLATFORM_DEVICE_H_

#include <linux/device.h>
#include <linux/ioport.h>

struct platform_device {
	const char *name;
	int id;
	struct device dev;
	u32 num_resources;
	struct resource *resource;
};

struct platform_driver {
	int (*probe)(struct platform_device *);
	void (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	struct device_driver driver;
};

static inline struct resource *
platform_get_resource(struct platform_device *dev, unsigned int type, unsigned int num)
{
	u32 i;
	for (i = 0; i < dev->num_resources; i++) {
		struct resource *r = &dev->resource[i];
		if ((r->flags & type) == type) {
			if (num-- == 0)
				return r;
		}
	}
	return NULL;
}

static inline struct resource *
platform_get_resource_byname(struct platform_device *dev, unsigned int type, const char *name)
{
	return platform_get_resource(dev, type, 0);
}

void __iomem *devm_ioremap_resource(struct device *dev, const struct resource *res);

static inline void __iomem *
devm_platform_ioremap_resource(struct platform_device *pdev, unsigned int index)
{
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, index);
	return devm_ioremap_resource(&pdev->dev, res);
}

#define to_platform_device(x) container_of((x), struct platform_device, dev)
static inline void platform_device_unregister(struct platform_device *pdev) {}

#define module_platform_driver(drv) \
	struct platform_driver *lx_get_platform_driver(void); \
	struct platform_driver *lx_get_platform_driver(void) { return &(drv); }

#endif
