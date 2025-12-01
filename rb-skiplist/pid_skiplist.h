/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PID_SKIPLIST_H
#define _LINUX_PID_SKIPLIST_H

#include <linux/types.h>
#include <linux/rbtree.h>
#include <linux/rcupdate.h>

struct pid; /* forward declaration */

#define PID_SL_MAX_LEVEL 16
#define PID_SL_P 4

struct pid_sl_node {
	int key;
	struct pid *pid;
	struct pid_sl_node **forward;
	u8 level;
	struct rcu_head rcu;
	
	/* ★ 최상위 레벨 노드만 RB-tree 링크 사용 */
	struct rb_node rb_node;
	bool in_rb_tree;  /* 이 노드가 RB-tree에 있는지 표시 */
};

struct pid_skiplist {
	int level;
	struct pid_sl_node *header;
	
	/* ★ 최상위 레벨용 RB-tree */
	struct rb_root top_rb_root;
} ____cacheline_aligned;

#ifdef CONFIG_PID_SKIPLIST

void pid_skiplist_init(struct pid_skiplist *sl, gfp_t gfp);
void pid_skiplist_destroy(struct pid_skiplist *sl);
int pid_skiplist_insert(struct pid_skiplist *sl, int key,
			struct pid *pid, gfp_t gfp);
struct pid *pid_skiplist_lookup_rcu(const struct pid_skiplist *sl, int key);
void pid_skiplist_remove(struct pid_skiplist *sl, int key);

/* iter / find_ge 함수 */
struct pid *pid_skiplist_iter_next_rcu(const struct pid_skiplist *sl,
					struct pid_sl_node **cursor,
					int start_key);
struct pid *pid_skiplist_find_ge_rcu(const struct pid_skiplist *sl, int key);

#else /* !CONFIG_PID_SKIPLIST */

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

static inline struct pid *
pid_skiplist_iter_next_rcu(const struct pid_skiplist *sl,
			    struct pid_sl_node **cursor,
			    int start_key)
{
	return NULL;
}

static inline struct pid *
pid_skiplist_find_ge_rcu(const struct pid_skiplist *sl, int key)
{
	return NULL;
}

#endif /* CONFIG_PID_SKIPLIST */

#endif /* _LINUX_PID_SKIPLIST_H */
