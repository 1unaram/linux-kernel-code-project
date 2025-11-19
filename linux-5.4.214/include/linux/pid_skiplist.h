// include/linux/pid_skiplist.h#ifndef _LINUX_PID_SKIPLIST_H
#define _LINUX_PID_SKIPLIST_H

#include <linux/types.h>
#include <linux/rcupdate.h>

#define PID_SL_MAX_LEVEL 16
#define PID_SL_P 2  // 1/4 확률

struct pid;

struct pid_sl_node {
    int key;
    struct pid *pid;
    int level;
    struct pid_sl_node **forward;
    struct rcu_head rcu;
};

struct pid_skiplist {
    int level;
    struct pid_sl_node *header;
} ____cacheline_aligned;  // 캐시 라인 정렬 최적화

/* 기본 함수 */
void pid_skiplist_init(struct pid_skiplist *sl, gfp_t gfp);
void pid_skiplist_destroy(struct pid_skiplist *sl);
int pid_skiplist_insert(struct pid_skiplist *sl, int key, struct pid *pid, gfp_t gfp);
void pid_skiplist_remove(struct pid_skiplist *sl, int key);

/* RCU 안전한 조회 함수 */
struct pid *pid_skiplist_lookup_rcu(const struct pid_skiplist *sl, int key);

/* 순회/검색 함수 - 새로 추가된 함수들 */
struct pid *pid_skiplist_iter_next_rcu(const struct pid_skiplist *sl,
                                        struct pid_sl_node **cursor,
                                        int start_key);

struct pid *pid_skiplist_find_ge_rcu(const struct pid_skiplist *sl, int key);

#endif /* _LINUX_PID_SKIPLIST_H */
