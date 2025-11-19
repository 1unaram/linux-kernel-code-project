// lib/pid_skiplist.c
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/pid.h>
#include <linux/pid_skiplist.h>

static int pid_sl_random_level(void)
{
    int level = 1;
    u32 random = prandom_u32();  // 빠른 pseudo-random

    while (level < PID_SL_MAX_LEVEL &&
           (random & ((1 << PID_SL_P) - 1)) == 0) {
        random >>= PID_SL_P;
        level++;
    }
    return level;
}

static struct pid_sl_node *pid_sl_node_alloc(int level, gfp_t gfp)
{
	struct pid_sl_node *node;

	node = kzalloc(sizeof(*node), gfp);
	if (!node)
		return NULL;

	node->forward = kcalloc(level, sizeof(struct pid_sl_node *), gfp);
	if (!node->forward) {
		kfree(node);
		return NULL;
	}

	node->level = level;
	return node;
}

// idr_init 대체 함수
void pid_skiplist_init(struct pid_skiplist *sl, gfp_t gfp)
{
	int max = PID_SL_MAX_LEVEL;

	sl->level = 1;
	sl->header = pid_sl_node_alloc(max, gfp | __GFP_NOFAIL);
	if (WARN_ON(!sl->header))  // 커널 로그에 경고
        return;
	sl->header->key = INT_MIN;
	sl->header->pid = NULL;
}

void pid_skiplist_destroy(struct pid_skiplist *sl)
{
	struct pid_sl_node *node, *next;

	// RCU grace period 대기
	synchronize_rcu();

	node = sl->header->forward[0];
	while (node) {
		next = node->forward[0];
		kfree(node->forward);
		kfree(node);
		node = next;
	}
	kfree(sl->header->forward);
	kfree(sl->header);
}

/* 쓰기 측: 락(예: pidmap_lock) 보호 하에서만 호출한다고 가정 */
int pid_skiplist_insert(struct pid_skiplist *sl, int key,
			struct pid *pid, gfp_t gfp)
{
	struct pid_sl_node *update[PID_SL_MAX_LEVEL];
	struct pid_sl_node *x = sl->header;
	int i, lvl;

	for (i = sl->level - 1; i >= 0; i--) {
		while (x->forward[i] && x->forward[i]->key < key)
			x = x->forward[i];
		update[i] = x;
	}

	x = x->forward[0];
	if (x && x->key == key) {
		/* 이미 있다면 struct pid*만 교체 */
		WRITE_ONCE(x->pid, pid);
		return 0;
	}

	lvl = pid_sl_random_level();
	if (lvl > sl->level) {
		for (i = sl->level; i < lvl; i++)
			update[i] = sl->header;
		WRITE_ONCE(sl->level, lvl);
	}

	x = pid_sl_node_alloc(lvl, gfp);
	if (!x)
		return -ENOMEM;

	x->key = key;
	WRITE_ONCE(x->pid, pid);

	for (i = 0; i < lvl; i++) {
		x->forward[i] = update[i]->forward[i];
		/* WRITE_ONCE 사용 권장 */
		WRITE_ONCE(update[i]->forward[i], x);
	}

	return 0;
}

/* RCU 읽기: 호출자는 rcu_read_lock() / rcu_read_unlock() 감싸야 함 */
struct pid *pid_skiplist_lookup_rcu(const struct pid_skiplist *sl, int key)
{
    const struct pid_sl_node *x = sl->header;
    int i;

    for (i = sl->level - 1; i >= 0; i--) {
        const struct pid_sl_node *next;
        while ((next = READ_ONCE(x->forward[i])) != NULL && next->key < key)
            x = next;

        // 정확히 일치하는 키 발견
        if (next && next->key == key)
            return READ_ONCE(next->pid);
    }
    return NULL;
}

static void pid_sl_node_rcu_free(struct rcu_head *rcu)
{
    struct pid_sl_node *x = container_of(rcu, struct pid_sl_node, rcu);

    kfree(x->forward);
    kfree(x);
}

void pid_skiplist_remove(struct pid_skiplist *sl, int key)
{
	struct pid_sl_node *update[PID_SL_MAX_LEVEL];
	struct pid_sl_node *x = sl->header;
	int i;
	int new_level = sl->level;

	for (i = sl->level - 1; i >= 0; i--) {
		while (x->forward[i] && x->forward[i]->key < key)
			x = x->forward[i];
		update[i] = x;
	}

	x = x->forward[0];
	if (!x || x->key != key)
		return;

	for (i = 0; i < sl->level; i++) {
		if (update[i]->forward[i] != x)
			break;
		WRITE_ONCE(update[i]->forward[i], x->forward[i]);
	}

	while (new_level > 1 && !READ_ONCE(sl->header->forward[new_level - 1]))
		new_level--;

	if (new_level != sl->level)
		WRITE_ONCE(sl->level, new_level);

	/* RCU로 free하고 싶다면 call_rcu 사용해도 됨 */
	call_rcu(&x->rcu, pid_sl_node_rcu_free);
}

/* RCU 안전한 순회: 시작 key 이후의 모든 항목 순회 */
struct pid *pid_skiplist_iter_next_rcu(const struct pid_skiplist *sl,
                                        struct pid_sl_node **cursor,
                                        int start_key)
{
    const struct pid_sl_node *node;

    if (!*cursor) {
        /* 첫 호출: start_key 이상의 첫 노드 찾기 (SkipList 활용) */
        node = sl->header;
        int i;

        /* 상위 레벨부터 탐색하여 빠르게 접근 */
        for (i = sl->level - 1; i >= 0; i--) {
            while (1) {
                const struct pid_sl_node *next = READ_ONCE(node->forward[i]);
                if (!next || next->key >= start_key)
                    break;
                node = next;
            }
        }

        /* 레벨 0에서 start_key 이상의 첫 노드 */
        node = READ_ONCE(node->forward[0]);
        if (!node || node->key < start_key)
            return NULL;

        *cursor = (struct pid_sl_node *)node;
        return READ_ONCE(node->pid);
    }

    /* 다음 노드로 이동 (레벨 0 순회) */
    node = READ_ONCE((*cursor)->forward[0]);
    if (!node)
        return NULL;

    *cursor = (struct pid_sl_node *)node;
    return READ_ONCE(node->pid);
}

/* nr 이상의 첫 번째 노드 찾기 (SkipList 최적화) */
struct pid *pid_skiplist_find_ge_rcu(const struct pid_skiplist *sl, int key)
{
    const struct pid_sl_node *node = sl->header;
    int i;

    /* 상위 레벨부터 탐색 */
    for (i = sl->level - 1; i >= 0; i--) {
        while (1) {
            const struct pid_sl_node *next = READ_ONCE(node->forward[i]);
            if (!next || next->key >= key)
                break;
            node = next;
        }
    }

    /* 레벨 0에서 key 이상의 첫 노드 */
    node = READ_ONCE(node->forward[0]);
    if (!node)
        return NULL;

    return READ_ONCE(node->pid);
}
