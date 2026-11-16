#ifndef _LX_LINUX_CLK_H_
#define _LX_LINUX_CLK_H_
struct device;
struct clk;
struct clk_bulk_data {
	const char *id;
	struct clk *clk;
};
static inline int devm_clk_bulk_get_optional(struct device *dev, int num_clks, struct clk_bulk_data *clks) { return 0; }
static inline int clk_bulk_prepare_enable(int num_clks, const struct clk_bulk_data *clks) { return 0; }
static inline void clk_bulk_disable_unprepare(int num_clks, const struct clk_bulk_data *clks) {}
#endif

