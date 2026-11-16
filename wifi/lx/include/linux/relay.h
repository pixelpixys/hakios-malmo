#ifndef _LX_LINUX_RELAY_H_
#define _LX_LINUX_RELAY_H_
struct rchan;
static inline void relay_close(struct rchan *chan) {}
static inline void relay_flush(struct rchan *chan) {}
#endif

