/*
 * What ath11k uses of Linux and LinuxKPI's headers (FreeBSD 15.1) do not declare. Small things
 * each; where one needs a function, it is in src/.
 */
#ifndef _LX_MORE_H_
#define _LX_MORE_H_

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/cleanup.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>

DEFINE_LOCK_GUARD_1(spinlock_bh, spinlock_t, spin_lock_bh(_T->lock), spin_unlock_bh(_T->lock))

struct nvmem_cell;
static inline struct nvmem_cell *devm_nvmem_cell_get(struct device *dev, const char *id) { return ERR_PTR(-ENOENT); }
static inline void *nvmem_cell_read(struct nvmem_cell *cell, size_t *len) { return ERR_PTR(-ENOENT); }

/* --- interrupts: how the line is triggered is the kernel's knowledge, not the driver's */
#define IRQF_TRIGGER_RISING	0x0001
#define IRQF_NO_AUTOEN		0
#define IRQ_NOAUTOEN		(1 << 12)	/* request_irq() leaves the interrupt disabled */
#define IRQ_DISABLE_UNLAZY	(1 << 19)	/* disable_irq() masks at once: it always does here */
int enable_irq_wake(unsigned int irq);
int disable_irq_wake(unsigned int irq);
struct net_device;
static inline void netif_threaded_enable(struct net_device *dev) {}
struct platform_device;
struct resource;
static inline int platform_get_irq(struct platform_device *dev, unsigned int num) { return (int)num; }
static inline int device_property_read_u32(struct device *dev, const char *propname, u32 *val) { return -EINVAL; }
#define MEMREMAP_WC	(1 << 1)
static inline void *devm_memremap(struct device *dev, resource_size_t offset, size_t size, unsigned long flags) { return NULL; }
struct platform_device_info {
	struct device *parent;
	struct fwnode_handle *fwnode;
	const char *name;
	int id;
	const struct resource *res;
	unsigned int num_res;
	const void *data;
	size_t size_data;
	u64 dma_mask;
};
struct platform_device *platform_device_register_full(const struct platform_device_info *pdevinfo);
static inline int of_dma_configure(struct device *dev, struct device_node *np, bool force_dma) { return 0; }
struct iommu_domain;
static inline struct iommu_domain *iommu_paging_domain_alloc(struct device *dev) { static int d; return (struct iommu_domain *)&d; }
static inline void iommu_domain_free(struct iommu_domain *domain) {}
static inline int iommu_attach_device(struct iommu_domain *domain, struct device *dev) { return 0; }
static inline void iommu_detach_device(struct iommu_domain *domain, struct device *dev) {}
static inline int iommu_map(struct iommu_domain *domain, unsigned long iova, phys_addr_t paddr, size_t size, int prot, gfp_t gfp) { return 0; }
static inline size_t iommu_unmap(struct iommu_domain *domain, unsigned long iova, size_t size) { return size; }
#define IOMMU_READ	1
#define IOMMU_WRITE	2
const void *device_get_match_data(struct device *dev);
#define platform_set_drvdata(pdev, data) dev_set_drvdata(&(pdev)->dev, (data))
#define platform_get_drvdata(pdev) dev_get_drvdata(&(pdev)->dev)

/* the phone is not woken by its radio: nothing here suspends */
static inline bool device_may_wakeup(struct device *dev) { return (false); }

/* --- DMA: memory for the device that need not be coherent, and the barriers around ring indices */
void *dma_alloc_noncoherent(struct device *dev, size_t size, dma_addr_t *handle,
    enum dma_data_direction dir, gfp_t gfp);
void dma_free_noncoherent(struct device *dev, size_t size, void *vaddr, dma_addr_t handle,
    enum dma_data_direction dir);
#define dma_rmb()	rmb()
#define dma_wmb()	wmb()
#define dma_mb()	mb()

/* --- a table of device ids is found through the driver (of_match_table), not through a section */
#undef MODULE_DEVICE_TABLE
#define MODULE_DEVICE_TABLE(bus, table)

/* --- 802.11: radio measurement action frames (IEEE 802.11-2020, 9.6.6) */
#define WLAN_CATEGORY_RADIO_MEASUREMENT			5
#define WLAN_RM_ACTION_LINK_MEASUREMENT_REQUEST		2
#define WLAN_RM_ACTION_LINK_MEASUREMENT_REPORT		3
/* the driver wants to hear of every change of regulatory domain it brought about itself */
#define WIPHY_FLAG_NOTIFY_REGDOM_BY_DRIVER		BIT(24)

#endif
