#ifndef _LX_LINUX_REGULATOR_CONSUMER_H_
#define _LX_LINUX_REGULATOR_CONSUMER_H_
struct device;
struct regulator;
struct regulator_bulk_data {
	const char *supply;
	struct regulator *consumer;
};
static inline int devm_regulator_bulk_get(struct device *dev, int num, struct regulator_bulk_data *consumers) { return 0; }
static inline int regulator_bulk_enable(int num, struct regulator_bulk_data *consumers) { return 0; }
static inline int regulator_bulk_disable(int num, struct regulator_bulk_data *consumers) { return 0; }
#endif

