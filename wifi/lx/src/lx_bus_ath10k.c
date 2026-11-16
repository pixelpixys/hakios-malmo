/*
 * Bus & LinuxKPI glue for Linux ath10k (WCN3990 SNOC on Blair / Moto G85).
 * Replaces lx_bus.c (which is for ath11k) when building ath10k.o.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/interrupt.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>

#include "core.h"
#include "ce.h"
#include "snoc.h"
#include "debug.h"
#include "qmi.h"

#include <lx_os.h>
#include "lx.h"

extern struct platform_driver *lx_get_platform_driver(void);
extern void *lx_os_map_registers(unsigned long long physical, unsigned long size);

static struct {
	struct ath10k *ar;
	volatile bool looking;
	volatile bool looks;
	volatile bool napi_enabled;
} bus;

/* ---- Readable logging (replaces debug.c's %pV printk) ---- */

static void
ath10k_say(const char *kind, const char *fmt, va_list ap)
{
	char line[320];
	size_t len;

	vsnprintf(line, sizeof(line), fmt, ap);
	len = strlen(line);
	lx_tell("ath10k%s: %s%s", kind, line, len != 0 && line[len - 1] == '\n' ? "" : "\n");
}

void
ath10k_info(struct ath10k *ar, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	ath10k_say("", fmt, ap);
	va_end(ap);
}

void
ath10k_warn(struct ath10k *ar, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	ath10k_say(" warn", fmt, ap);
	va_end(ap);
}

void
ath10k_err(struct ath10k *ar, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	ath10k_say(" err", fmt, ap);
	va_end(ap);
}

void
__ath10k_dbg(struct ath10k *ar, enum ath10k_debug_mask mask, const char *fmt, ...)
{
	va_list ap;
	if (!(ath10k_debug_mask & mask))
		return;
	va_start(ap, fmt);
	ath10k_say(" dbg", fmt, ap);
	va_end(ap);
}

void
ath10k_dbg_dump(struct ath10k *ar, enum ath10k_debug_mask mask, const char *msg,
    const char *prefix, const void *buf, size_t len)
{
}

/* ---- QMI bridge: WLFW QMI exchange already ran in hakios.wlfw.c up to MISSION MODE ---- */

int
ath10k_qmi_init(struct ath10k *ar, u32 msa_size)
{
	return (0);
}

int
ath10k_qmi_deinit(struct ath10k *ar)
{
	return (0);
}

extern int lx_os_wlfw_wlan_enable(void);
extern int lx_os_wlfw_wlan_disable(void);

int
ath10k_qmi_wlan_enable(struct ath10k *ar, struct ath10k_qmi_wlan_enable_cfg *config,
    enum wlfw_driver_mode_enum_v01 mode, const char *version)
{
	return (lx_os_wlfw_wlan_enable());
}

int
ath10k_qmi_wlan_disable(struct ath10k *ar)
{
	return (lx_os_wlfw_wlan_disable());
}

int
ath10k_qmi_set_fw_log_mode(struct ath10k *ar, u8 fw_log_mode)
{
	return (0);
}

/* ---- FreeBSD / LinuxKPI helpers missing from lx_base.c / lx_80211.c ---- */

const u8 rfc1042_header[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

const uint32_t crc32_tab[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

void
DELAY(int usec)
{
	if (usec >= 1000) {
		lx_pause_ms((unsigned)(usec / 1000));
	} else if (usec > 0) {
		uint64_t end = lx_os_nanoseconds() + (uint64_t)usec * 1000ull;
		while (lx_os_nanoseconds() < end) {
			__asm__ volatile("" ::: "memory");
		}
	}
}

void
nanouptime(struct timespec *tsp)
{
	uint64_t ns = lx_os_nanoseconds();
	tsp->tv_sec = (time_t)(ns / 1000000000ull);
	tsp->tv_nsec = (long)(ns % 1000000000ull);
}

int
device_printf(device_t dev, const char *fmt, ...)
{
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	printf("%s", buf);
	return (0);
}

int
eventratecheck(struct timeval *lasttime, int *curpps, int maxpps)
{
	return (1);
}

static struct reg_dmn_pair_mapping default_regpair = {
	.reg_domain = 0x60,
	.reg_5ghz_ctl = 0x10,
	.reg_2ghz_ctl = 0x10,
};

bool ath_is_world_regd(struct ath_regulatory *reg) { return (true); }
int ath_regd_init(struct ath_regulatory *reg, struct wiphy *wiphy,
    void (*reg_notifier)(struct wiphy *wiphy, struct regulatory_request *request))
{
	if (reg != NULL) {
		if (reg->regpair == NULL)
			reg->regpair = &default_regpair;
		if (reg->alpha2[0] == '\0') {
			reg->alpha2[0] = '0';
			reg->alpha2[1] = '0';
		}
	}
	return (0);
}
void ath_reg_notifier_apply(struct wiphy *wiphy, struct regulatory_request *request,
    struct ath_regulatory *reg) {}

const struct property *
of_find_property(const struct device_node *np, const char *name, int *lenp)
{
	if (np == NULL || np->properties == NULL || name == NULL)
		return (NULL);
	for (int i = 0; i < np->nproperties; i++) {
		if (strcmp(np->properties[i].name, name) == 0) {
			if (lenp != NULL)
				*lenp = np->properties[i].length;
			return (&np->properties[i]);
		}
	}
	return (NULL);
}

const void *
of_get_property(const struct device_node *np, const char *name, int *lenp)
{
	const struct property *p = of_find_property(np, name, lenp);
	return (p != NULL ? p->value : NULL);
}

int
of_property_read_string(const struct device_node *np, const char *name, const char **out)
{
	const struct property *p = of_find_property(np, name, NULL);
	if (p == NULL || p->value == NULL)
		return (-EINVAL);
	*out = p->value;
	return (0);
}

bool
of_device_is_compatible(const struct device_node *np, const char *compatible)
{
	const char *val = of_get_property(np, "compatible", NULL);
	return (val != NULL && compatible != NULL && strcmp(val, compatible) == 0);
}

const struct of_device_id *
of_match_node(const struct of_device_id *matches, const struct device_node *np)
{
	if (matches == NULL || np == NULL)
		return (NULL);
	for (; matches->compatible[0] != '\0'; matches++) {
		if (of_device_is_compatible(np, matches->compatible))
			return (matches);
	}
	return (NULL);
}

static const void *matched_drv_data;

const void *
device_get_match_data(struct device *dev)
{
	return (matched_drv_data);
}

void __iomem *
devm_ioremap_resource(struct device *dev, const struct resource *res)
{
	if (res == NULL)
		return (ERR_PTR(-EINVAL));
	return ((void __iomem *)lx_os_map_registers(res->start, resource_size(res)));
}

int linux_dma_tag_init(struct device *dev, u64 dma_mask) { return (0); }
int linux_dma_tag_init_coherent(struct device *dev, u64 dma_mask) { return (0); }

void lkpi_devm_kmalloc_release(struct device *dev, void *res) {}
void *lkpi_devres_alloc(void (*release)(struct device *, void *), size_t size, gfp_t gfp)
{
	return (lx_os_alloc(size));
}
void lkpi_devres_add(struct device *dev, void *res) {}

void lkpi_disable_irq(unsigned int irq) {}
int lkpi_enable_irq(unsigned int irq) { return (0); }
void lkpi_free_irq(unsigned int irq, void *dev) {}

int lkpi_hex_dump_to_buffer(const void *buf, size_t len, int rowsize, int groupsize,
    char *linebuf, size_t linebuflen, bool ascii)
{
	if (linebuflen > 0)
		linebuf[0] = '\0';
	return (0);
}

struct net_device *
linuxkpi_alloc_netdev(size_t sizeof_priv, const char *name, uint32_t name_assign_type,
    void (*setup)(struct net_device *))
{
	struct net_device *ndev = lx_os_alloc(sizeof(*ndev) + sizeof_priv);
	if (ndev != NULL && setup != NULL)
		setup(ndev);
	return (ndev);
}

void linuxkpi_free_netdev(struct net_device *ndev)
{
	if (ndev != NULL)
		lx_os_free(ndev, sizeof(*ndev));
}

void linuxkpi_napi_disable(struct napi_struct *n) { bus.napi_enabled = false; }
void linuxkpi_napi_enable(struct napi_struct *n) { bus.napi_enabled = true; }
void linuxkpi_napi_synchronize(struct napi_struct *n) {}
void linuxkpi_netif_napi_del(struct napi_struct *n) {}

void *
linuxkpi_dmam_alloc_coherent(struct device *dev, size_t size,
    dma_addr_t *dma_handle, gfp_t gfp)
{
	return dma_alloc_coherent(dev, size, dma_handle, gfp);
}

void
linuxkpi_netif_napi_add(struct net_device *ndev, struct napi_struct *napi,
    int (*poll)(struct napi_struct *, int))
{
	napi->dev = ndev;
	napi->poll = poll;
}

bool linuxkpi_napi_schedule(struct napi_struct *n) { return true; }

bool
linuxkpi_napi_complete(struct napi_struct *n)
{
	return true;
}

int
lkpi_request_irq(struct device *dev, unsigned int irq, irq_handler_t handler,
    irq_handler_t thread_handler, unsigned long flags, const char *name,
    void *arg)
{
	return 0;
}

struct device_node *
of_get_child_by_name(const struct device_node *np, const char *name)
{
	return NULL;
}

int
of_reserved_mem_region_to_resource(const struct device_node *np, int index,
    struct resource *res)
{
	return -ENOENT;
}

struct sk_buff *
linuxkpi_alloc_skb(size_t size, gfp_t gfp)
{
	return (alloc_skb(size, gfp));
}

struct sk_buff *
linuxkpi_skb_copy(const struct sk_buff *skb, gfp_t gfp)
{
	struct sk_buff *n = alloc_skb(skb->len, gfp);
	if (n != NULL) {
		memcpy(skb_put(n, skb->len), skb->data, skb->len);
	}
	return (n);
}

int linuxkpi_device_get_mac_address(struct device *dev, char *addr) { return (-ENOENT); }
u32 linuxkpi_cfg80211_calculate_bitrate(struct rate_info *rate) { return (0); }
struct linuxkpi_ieee80211_regdomain *lkpi_get_linuxkpi_ieee80211_regdomain(size_t n) {
	return (kzalloc(sizeof(struct linuxkpi_ieee80211_regdomain) + n * sizeof(struct ieee80211_reg_rule), GFP_KERNEL));
}

uint8_t linuxkpi_ieee80211_get_tid(struct ieee80211_hdr *hdr, bool is_qos) { return (0); }
struct ieee80211_txq *linuxkpi_ieee80211_next_txq(struct ieee80211_hw *hw, u8 ac) { return (NULL); }
void linuxkpi_ieee80211_schedule_txq(struct ieee80211_hw *hw, struct ieee80211_txq *txq, bool force) {}
void linuxkpi_ieee80211_stop_queue(struct ieee80211_hw *hw, int queue) {}
struct sk_buff *linuxkpi_ieee80211_tx_dequeue(struct ieee80211_hw *hw, struct ieee80211_txq *txq) { return (NULL); }
void linuxkpi_ieee80211_txq_get_depth(struct ieee80211_txq *txq, unsigned long *frame_cnt, unsigned long *byte_cnt)
{
	if (frame_cnt) *frame_cnt = 0;
	if (byte_cnt) *byte_cnt = 0;
}
void linuxkpi_ieee80211_txq_schedule_start(struct ieee80211_hw *hw, u8 ac) {}
void linuxkpi_ieee80211_wake_queue(struct ieee80211_hw *hw, int queue) {}

/* ---- CE & NAPI polling thread (no GIC IRQ wiring needed initially) ---- */

static void
ce_poll_thread(void *argument)
{
	struct ath10k *ar = argument;
	struct ath10k_ce *ce = ath10k_ce_priv(ar);

	bus.looks = true;
	while (bus.looking) {
		if (bus.napi_enabled) {
			for (int i = 0; i < CE_COUNT; i++) {
				if (ce != NULL && ce->ce_states[i].ar != NULL) {
					ath10k_ce_per_engine_service(ar, i);
				}
			}
			if (ar->napi.poll != NULL) {
				ar->napi.poll(&ar->napi, 64);
			}
		}
		lx_pause_ms(1);
	}
	bus.looks = false;
}

/* ---- Entry points from hakios.lx.c ---- */

int
lx_prepare(void)
{
	return (lx_base_init() != 0 || lx_pool_init() != 0 ? ENOMEM : 0);
}

struct platform_device *
platform_device_register_full(const struct platform_device_info *pdevinfo)
{
	static struct platform_device fw_pdev;
	fw_pdev.name = pdevinfo ? pdevinfo->name : "ath10k-fw";
	fw_pdev.id = pdevinfo ? pdevinfo->id : -1;
	fw_pdev.dev.dma_priv = (void *)1;
	return (&fw_pdev);
}

int
lx_start(void)
{
	static struct resource res[1] = {
		{
			.start = 0x0c800000ull,
			.end = 0x0cffffffull,
			.flags = IORESOURCE_MEM,
		},
	};
	static const struct property props[] = {
		{ .name = "compatible", .length = sizeof("qcom,wcn3990-wifi"), .value = "qcom,wcn3990-wifi" },
	};
	static struct device_node node = {
		.name = "wifi",
		.full_name = "wifi@c800000",
		.properties = props,
		.nproperties = 1,
	};
	static struct platform_device pdev = {
		.name = "ath10k_snoc",
		.id = -1,
		.num_resources = 1,
		.resource = res,
	};
	extern const struct ath10k_snoc_drv_priv wcn3990_snoc_drv_priv;
	struct platform_driver *drv;
	struct ath10k *ar;
	int ret;

	if (lx_prepare() != 0) {
		lx_tell("lx: no memory or thread for ath10k\n");
		return (ENOMEM);
	}
	ath10k_debug_mask = ATH10K_DBG_BOOT;

	pdev.dev.of_node = &node;
	pdev.dev.kobj.name = "wifi@c800000";
	pdev.dev.dma_priv = (void *)1;
	drv = lx_get_platform_driver();
	matched_drv_data = of_match_node(drv->driver.of_match_table, &node)->data;
	if (matched_drv_data == NULL) {
		lx_tell("lx: ath10k_snoc_dt_match did not match qcom,wcn3990-wifi\n");
		return (ENODEV);
	}

	lx_tell("lx: 1. probing ath10k_snoc...\n");
	ret = drv->probe(&pdev);
	if (ret != 0) {
		lx_tell("lx: ath10k_snoc_probe failed: %d\n", ret);
		return (-ret);
	}
	ar = platform_get_drvdata(&pdev);
	bus.ar = ar;

	lx_tell("lx: 2. ath10k_snoc probed (ar=%p); triggering FW_READY_IND\n", ar);
	ret = ath10k_snoc_fw_indication(ar, ATH10K_QMI_EVENT_FW_READY_IND);
	if (ret != 0) {
		lx_tell("lx: ath10k_snoc_fw_indication failed: %d\n", ret);
		return (-ret);
	}

	lx_tell("lx: 3. FW_READY_IND done; starting ce_poll_thread\n");
	bus.looking = true;
	if (lx_os_thread("ath10k-ce", ce_poll_thread, ar) != 0) {
		lx_tell("lx: failed to start ath10k CE poll thread\n");
		return (ENOMEM);
	}

	/* ath10k_core_register runs probe_fw asynchronously on ar->register_work */
	for (unsigned ms = 0; ms < 15000; ms += 20) {
		if (test_bit(ATH10K_FLAG_CORE_REGISTERED, &ar->dev_flags))
			break;
		lx_pause_ms(20);
	}
	lx_tell("lx: ath10k core state=%d dev_flags=%#lx\n", ar->state, ar->dev_flags);

	if (lx_os_option("wifi_station") || lx_os_option("wifi")) {
		ret = lx_station_offer();
		if (ret == 0)
			return (0);
	} else if (lx_os_option("wifi_scan")) {
		ret = lx_station_scan();
		lx_stop();
		return (ret);
	} else if (lx_os_option("wifi_join")) {
		ret = lx_station_join();
		if (ret == 0)
			return (0);
	}
	return (0);
}

void
lx_stop(void)
{
	bus.looking = false;
	if (bus.ar != NULL) {
		lx_station_withdraw();
		lx_station_leave();
		ath10k_core_unregister(bus.ar);
		bus.ar = NULL;
	}
}

int
lx_transmit(const unsigned char *frame, unsigned long length)
{
	return (bus.ar != NULL ? lx_station_send(frame, length) : ENETDOWN);
}
