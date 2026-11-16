#ifndef _LX_LINUX_PWRSEQ_CONSUMER_H_
#define _LX_LINUX_PWRSEQ_CONSUMER_H_
#include <linux/err.h>
struct device;
struct pwrseq_desc;
static inline struct pwrseq_desc *devm_pwrseq_get(struct device *dev, const char *target) { return ERR_PTR(-EPROBE_DEFER); }
static inline int pwrseq_power_on(struct pwrseq_desc *desc) { return 0; }
static inline int pwrseq_power_off(struct pwrseq_desc *desc) { return 0; }
#endif

