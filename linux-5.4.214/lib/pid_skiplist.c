// lib/pid_skiplist.c
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/pid.h>
#include <linux/pid_skiplist.h>

static int pid_sl_random_level(void)
{
	int level = 1;

	while (level < PID_SL_MAX_LEVEL) {
		u8 r;
		get_random_bytes(&r, 1);
		if (r % PID_SL_P != 0)
			break;
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
	sl->header->key = INT_MIN;
	sl->header->pid = NULL;
}

void pid_skiplist_destroy(struct pid_skiplist *sl)
{
	struct pid_sl_node *node, *next;

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
		x->pid = pid;
		return 0;
	}

	lvl = pid_sl_random_level();
	if (lvl > sl->level) {
		for (i = sl->level; i < lvl; i++)
			update[i] = sl->header;
		sl->level = lvl;
	}

	x = pid_sl_node_alloc(lvl, gfp);
	if (!x)
		return -ENOMEM;

	x->key = key;
	x->pid = pid;

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
		for (;;) {
			next = READ_ONCE(x->forward[i]);
			if (!next || next->key > key)
				break;
			if (next->key == key)
				return READ_ONCE(next->pid);
			x = next;
		}
	}
	return NULL;
}

void pid_skiplist_remove(struct pid_skiplist *sl, int key)
{
	struct pid_sl_node *update[PID_SL_MAX_LEVEL];
	struct pid_sl_node *x = sl->header;
	int i;

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

	while (sl->level > 1 && !sl->header->forward[sl->level - 1])
		sl->level--;

	/* RCU로 free하고 싶다면 call_rcu 사용해도 됨 */
	kfree(x->forward);
	kfree(x);
}
