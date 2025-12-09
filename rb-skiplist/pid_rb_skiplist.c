// lib/pid_rb_skiplist.c

#include <linux/slab.h>
#include <linux/pid_rb_skiplist.h>
#include <linux/random.h>
#include <linux/rbtree.h>

static int pid_sl_random_level(void)
{
	int level = 1;
	u32 random = prandom_u32();

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

	if (gfp & __GFP_NOFAIL)
		gfp |= GFP_ATOMIC;

	node = kzalloc(sizeof(*node), gfp);
	if (!node)
		return NULL;

	node->forward = kcalloc(level, sizeof(struct pid_sl_node *), gfp);
	if (!node->forward) {
		kfree(node);
		return NULL;
	}

	node->level = level;
	node->in_rb_tree = false;
	RB_CLEAR_NODE(&node->rb_node);

	return node;
}

void pid_skiplist_init(struct pid_skiplist *sl, gfp_t gfp)
{
	int max = PID_SL_MAX_LEVEL;

	sl->level = 1;
	sl->header = pid_sl_node_alloc(max, gfp | __GFP_NOFAIL);
	BUG_ON(!sl->header);

	sl->header->key = INT_MIN;
	sl->header->pid = NULL;
	memset(sl->header->forward, 0, max * sizeof(struct pid_sl_node *));

	/* ★ RB-tree 초기화 */
	sl->top_rb_root = RB_ROOT;
}

void pid_skiplist_destroy(struct pid_skiplist *sl)
{
	struct pid_sl_node *node, *next;

	synchronize_rcu();

	node = sl->header->forward[0];
	while (node) {
		next = node->forward[0];

		/* RB-tree에서 제거 (이미 비워졌겠지만 안전장치) */
		if (node->in_rb_tree)
			rb_erase(&node->rb_node, &sl->top_rb_root);

		kfree(node->forward);
		kfree(node);
		node = next;
	}

	kfree(sl->header->forward);
	kfree(sl->header);
}

/* ★ RB-tree 헬퍼: 노드 삽입 */
static void rb_insert_node(struct pid_skiplist *sl, struct pid_sl_node *node)
{
	struct rb_node **new = &sl->top_rb_root.rb_node;
	struct rb_node *parent = NULL;

	while (*new) {
		struct pid_sl_node *this = rb_entry(*new, struct pid_sl_node, rb_node);
		parent = *new;

		if (node->key < this->key)
			new = &(*new)->rb_left;
		else if (node->key > this->key)
			new = &(*new)->rb_right;
		else
			return; /* 이미 존재 */
	}

	rb_link_node(&node->rb_node, parent, new);
	rb_insert_color(&node->rb_node, &sl->top_rb_root);
	node->in_rb_tree = true;
}

/* ★ RB-tree 헬퍼: key 이하의 가장 큰 노드 찾기 (lookup 시작점) */
static struct pid_sl_node *rb_find_le(struct pid_skiplist *sl, int key)
{
	struct rb_node *node = sl->top_rb_root.rb_node;
	struct pid_sl_node *result = sl->header;

	while (node) {
		struct pid_sl_node *sl_node = rb_entry(node, struct pid_sl_node, rb_node);

		if (sl_node->key <= key) {
			result = sl_node;
			node = node->rb_right; /* 더 큰 값 탐색 */
		} else {
			node = node->rb_left;
		}
	}

	return result;
}

int pid_skiplist_insert(struct pid_skiplist *sl, int key,
			struct pid *pid, gfp_t gfp)
{
	struct pid_sl_node *update[PID_SL_MAX_LEVEL];
	struct pid_sl_node *x;
	int i, lvl;

	/* ★ 1. 최상위 레벨: RB-tree로 시작점 찾기 */
	if (sl->level == sl->header->level) {
		/* 최상위 레벨이 존재할 때만 RB-tree 사용 */
		x = rb_find_le(sl, key);
	} else {
		x = sl->header;
	}

	/* ★ 2. 나머지 레벨: 일반 skiplist 탐색 */
	for (i = sl->level - 1; i >= 0; i--) {
		while (x->forward[i] && x->forward[i]->key < key)
			x = x->forward[i];
		update[i] = x;
	}

	x = x->forward[0];

	/* ★ 3. 이미 존재하면 pid만 교체 */
	if (x && x->key == key) {
		WRITE_ONCE(x->pid, pid);
		return 0;
	}

	/* ★ 4. 새 노드 생성 */
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

	/* ★ 5. skiplist 링크 연결 */
	for (i = 0; i < lvl; i++) {
		x->forward[i] = update[i]->forward[i];
		WRITE_ONCE(update[i]->forward[i], x);
	}

	/* ★ 6. 최상위 레벨이면 RB-tree에도 추가 */
	if (lvl == sl->level && lvl == sl->header->level) {
		rb_insert_node(sl, x);
	}

	return 0;
}

struct pid *pid_skiplist_lookup_rcu(const struct pid_skiplist *sl, int key)
{
	const struct pid_sl_node *x;
	const struct pid_sl_node *next;
	int i;

	if (!sl || !sl->header)
		return NULL;

	/* ★ 1. 최상위 레벨: RB-tree로 시작점 찾기 */
	x = rb_find_le((struct pid_skiplist *)sl, key);

	/* ★ 2. 나머지 레벨: skiplist 탐색 */
	for (i = sl->level - 1; i >= 0; i--) {
		while ((next = READ_ONCE(x->forward[i])) != NULL && next->key < key)
			x = next;
	}

	/* ★ 3. 레벨 0에서 확인 */
	next = READ_ONCE(x->forward[0]);
	if (next && next->key == key)
		return READ_ONCE(next->pid);

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
	struct pid_sl_node *x;
	int i;
	int new_level;

	if (!sl || !sl->header)
		return;

	/* ★ 1. 최상위 레벨: RB-tree로 시작점 찾기 */
	x = rb_find_le(sl, key);

	/* ★ 2. skiplist 탐색 */
	for (i = sl->level - 1; i >= 0; i--) {
		while (x->forward[i] && x->forward[i]->key < key)
			x = x->forward[i];
		update[i] = x;
	}

	x = x->forward[0];

	if (!x || x->key != key)
		return;

	/* ★ 3. RB-tree에서 제거 (최상위 레벨이면) */
	if (x->in_rb_tree) {
		rb_erase(&x->rb_node, &sl->top_rb_root);
		x->in_rb_tree = false;
	}

	/* ★ 4. skiplist 링크 제거 */
	for (i = 0; i < sl->level; i++) {
		if (update[i]->forward[i] != x)
			break;
		WRITE_ONCE(update[i]->forward[i], x->forward[i]);
	}

	/* ★ 5. 레벨 조정 */
	new_level = sl->level;
	while (new_level > 1 && !READ_ONCE(sl->header->forward[new_level - 1]))
		new_level--;
	if (new_level != sl->level)
		WRITE_ONCE(sl->level, new_level);

	/* ★ 6. RCU 해제 */
	call_rcu(&x->rcu, pid_sl_node_rcu_free);
}

struct pid *pid_skiplist_iter_next_rcu(const struct pid_skiplist *sl,
					struct pid_sl_node **cursor,
					int start_key)
{
	const struct pid_sl_node *node;

	if (!sl || !sl->header)
		return NULL;

	if (!*cursor) {
		/* ★ 첫 호출: RB-tree + skiplist로 start_key 이상 찾기 */
		node = rb_find_le((struct pid_skiplist *)sl, start_key);

		int i;
		for (i = sl->level - 1; i >= 0; i--) {
			while (1) {
				const struct pid_sl_node *next = READ_ONCE(node->forward[i]);
				if (!next || next->key >= start_key)
					break;
				node = next;
			}
		}

		node = READ_ONCE(node->forward[0]);
		if (!node || node->key < start_key)
			return NULL;

		*cursor = (struct pid_sl_node *)node;
		return READ_ONCE(node->pid);
	}

	/* 다음 노드로 이동 */
	node = READ_ONCE((*cursor)->forward[0]);
	if (!node)
		return NULL;

	*cursor = (struct pid_sl_node *)node;
	return READ_ONCE(node->pid);
}

struct pid *pid_skiplist_find_ge_rcu(const struct pid_skiplist *sl, int key)
{
	const struct pid_sl_node *node;
	int i;

	if (!sl || !sl->header)
		return NULL;

	/* ★ RB-tree + skiplist로 key 이상의 첫 노드 찾기 */
	node = rb_find_le((struct pid_skiplist *)sl, key);

	for (i = sl->level - 1; i >= 0; i--) {
		while (1) {
			const struct pid_sl_node *next = READ_ONCE(node->forward[i]);
			if (!next || next->key >= key)
				break;
			node = next;
		}
	}

	node = READ_ONCE(node->forward[0]);
	if (!node)
		return NULL;

	return READ_ONCE(node->pid);
}
