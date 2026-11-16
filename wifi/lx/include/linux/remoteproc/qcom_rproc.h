#ifndef _LX_LINUX_REMOTEPROC_QCOM_RPROC_H_
#define _LX_LINUX_REMOTEPROC_QCOM_RPROC_H_
#include <linux/types.h>
enum {
	QCOM_SSR_BEFORE_POWERUP,
	QCOM_SSR_AFTER_POWERUP,
	QCOM_SSR_BEFORE_SHUTDOWN,
	QCOM_SSR_AFTER_SHUTDOWN,
};
struct qcom_ssr_notify_data {
	const char *name;
	bool crashed;
};
struct notifier_block;
static inline void *qcom_register_ssr_notifier(const char *name, struct notifier_block *nb) { return (void *)0; }
static inline int qcom_unregister_ssr_notifier(void *notify, struct notifier_block *nb) { return 0; }
#endif
