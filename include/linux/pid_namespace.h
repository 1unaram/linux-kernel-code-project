/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PID_NS_H
#define _LINUX_PID_NS_H

#include <linux/bug.h>
#include <linux/kref.h>
#include <linux/ns_common.h>
#include <linux/threads.h>
#include <linux/workqueue.h>
#include <linux/uidgid.h>
#include <linux/idr.h>
#include <linux/mm.h>

#ifdef CONFIG_PID_SKIPLIST
#include <linux/pid_skiplist.h>
#endif

struct task_struct;
struct user_namespace;
struct ucounts;
struct vfsmount;
struct dentry;
struct fs_pin;

enum {
	HIDEPID_OFF       = 0,
	HIDEPID_NO_ACCESS = 1,
	HIDEPID_INVISIBLE = 2,
};

struct pid_namespace {
	struct kref kref;

#ifndef CONFIG_PID_SKIPLIST
	struct idr idr;
#else
	struct pid_skiplist pid_sl;
	int last_pid;
#endif

	struct rcu_head rcu;
	unsigned int pid_allocated;
	struct task_struct *child_reaper;
	struct kmem_cache *pid_cachep;
	unsigned int level;
	struct pid_namespace *parent;
#ifdef CONFIG_PROC_FS
	struct vfsmount *proc_mnt;
	struct dentry *proc_self;
	struct dentry *proc_thread_self;
#endif
#ifdef CONFIG_BSD_PROCESS_ACCT
	struct fs_pin *bacct;
#endif
	struct user_namespace *user_ns;
	struct ucounts *ucounts;
	struct work_struct proc_work;
	kgid_t pid_gid;
	int hide_pid;
	int reboot;
	struct ns_common ns;
} __randomize_layout;

extern struct pid_namespace init_pid_ns;

#define PIDNS_ADDING (1U << 31)

#ifdef CONFIG_PID_NS
static inline struct pid_namespace *get_pid_ns(struct pid_namespace *ns)
{
	if (ns != &init_pid_ns)
		kref_get(&ns->kref);
	return ns;
}

extern struct pid_namespace *copy_pid_ns(unsigned long flags,
	struct user_namespace *user_ns, struct pid_namespace *ns);
extern void zap_pid_ns_processes(struct pid_namespace *pid_ns);
extern int reboot_pid_ns(struct pid_namespace *pid_ns, int cmd);
extern void put_pid_ns(struct pid_namespace *ns);

#else

#include <linux/err.h>

static inline struct pid_namespace *get_pid_ns(struct pid_namespace *ns)
{
	return ns;
}

static inline struct pid_namespace *copy_pid_ns(unsigned long flags,
	struct user_namespace *user_ns, struct pid_namespace *ns)
{
	if (flags & CLONE_NEWPID)
		ns = ERR_PTR(-EINVAL);
	return ns;
}

static inline void put_pid_ns(struct pid_namespace *ns) {}
static inline void zap_pid_ns_processes(struct pid_namespace *ns) { BUG(); }
static inline int reboot_pid_ns(struct pid_namespace *pid_ns, int cmd) { return 0; }

#endif /* CONFIG_PID_NS */

extern struct pid_namespace *task_active_pid_ns(struct task_struct *tsk);
void pidhash_init(void);
void pid_idr_init(void);

#ifdef CONFIG_PID_SKIPLIST
/* skiplist 버전에서는 last_pid를 cursor로 사용 */
static inline int pid_ns_get_cursor(struct pid_namespace *ns)
{
	return ns->last_pid;
}

static inline void pid_ns_set_cursor(struct pid_namespace *ns, int cursor)
{
	ns->last_pid = cursor;
}
#else
/* IDR 버전 */
static inline int pid_ns_get_cursor(struct pid_namespace *ns)
{
	return idr_get_cursor(&ns->idr);
}

static inline void pid_ns_set_cursor(struct pid_namespace *ns, int cursor)
{
	idr_set_cursor(&ns->idr, cursor);
}
#endif

#endif /* _LINUX_PID_NS_H */

