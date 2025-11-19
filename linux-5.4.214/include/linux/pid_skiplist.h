// include/linux/pid_skiplist.h
#ifndef _LINUX_PID_SKIPLIST_H
#define _LINUX_PID_SKIPLIST_H

#include <linux/types.h>
#include <linux/rcupdate.h>

struct pid;

#define PID_SL_MAX_LEVEL 16
#define PID_SL_P         4

struct pid_sl_node {
	int key;                     /* PID 번호 */
	struct pid *pid;             /* value: struct pid * */
	struct pid_sl_node **forward;/* [0..level-1] next 포인터 */
	u8 level;
	struct rcu_head rcu;
};

struct pid_skiplist {
	int level;
	struct pid_sl_node *header;
} ____cacheline_aligned;

#ifdef CONFIG_PID_SKIPLIST

void pid_skiplist_init(struct pid_skiplist *sl, gfp_t gfp);
void pid_skiplist_destroy(struct pid_skiplist *sl);

int pid_skiplist_insert(struct pid_skiplist *sl, int key,
			struct pid *pid, gfp_t gfp);
struct pid *pid_skiplist_lookup_rcu(const struct pid_skiplist *sl, int key);
void pid_skiplist_remove(struct pid_skiplist *sl, int key);

#else  /* !CONFIG_PID_SKIPLIST */

/* CONFIG 끈 경우: 빈 스텁 제공 (혹시 include만 하는 코드 대비) */

static inline void pid_skiplist_init(struct pid_skiplist *sl, gfp_t gfp) {}
static inline void pid_skiplist_destroy(struct pid_skiplist *sl) {}

static inline int pid_skiplist_insert(struct pid_skiplist *sl, int key,
				      struct pid *pid, gfp_t gfp)
{
	return -EOPNOTSUPP;
}

static inline struct pid *
pid_skiplist_lookup_rcu(const struct pid_skiplist *sl, int key)
{
	return NULL;
}

static inline void pid_skiplist_remove(struct pid_skiplist *sl, int key) {}

#endif /* CONFIG_PID_SKIPLIST */

#endif /* _LINUX_PID_SKIPLIST_H */
