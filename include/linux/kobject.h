/*
 * kobject.h - generic kernel object infrastructure.
 *
 * Copyright (c) 2002-2003 Patrick Mochel
 * Copyright (c) 2002-2003 Open Source Development Labs
 * Copyright (c) 2006-2008 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (c) 2006-2008 Novell Inc.
 *
 * This file is released under the GPLv2.
 *
 * Please read Documentation/kobject.txt before using the kobject
 * interface, ESPECIALLY the parts about reference counts and object
 * destructors.
 */

#ifndef _KOBJECT_H_
#define _KOBJECT_H_

#include <linux/types.h>
#include <linux/list.h>
#include <linux/sysfs.h>
#include <linux/compiler.h>
#include <linux/spinlock.h>
#include <linux/kref.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <asm/atomic.h>

#define UEVENT_HELPER_PATH_LEN		256
#define UEVENT_NUM_ENVP			32	/* number of env pointers */
#define UEVENT_BUFFER_SIZE		2048	/* buffer for the variables */

/* path to the userspace helper executed on an event */
extern char uevent_helper[];

/* counter to tag the uevent, read only except for the kobject core */
extern u64 uevent_seqnum;

/*
 * The actions here must match the index to the string array
 * in lib/kobject_uevent.c
 *
 * Do not add new actions here without checking with the driver-core
 * maintainers. Action strings are not meant to express subsystem
 * or device specific properties. In most cases you want to send a
 * kobject_uevent_env(kobj, KOBJ_CHANGE, env) with additional event
 * specific variables added to the event environment.
 */
enum kobject_action {
	KOBJ_ADD,
	KOBJ_REMOVE,
	KOBJ_CHANGE,
	KOBJ_MOVE,
	KOBJ_ONLINE,
	KOBJ_OFFLINE,
	KOBJ_MAX
};

/*表示一个内核对象,kobject数据结构最通用的用法时嵌在表示某一对象的数据结构中,
 *比如内核中定义的字符型设备对象cdev中就嵌入了kobject结构*/
struct kobject {
	const char		*name;	/*表示内核对象的名字,如果内核对象加入系统,name
					 *将会出现在sysfs文件系统中*/

	struct list_head	entry;	/*用来将一系列的内核对象构成链表,链接到所在kset
					 *中去的单元*/

	struct kobject		*parent;/*指向该内核对象的上层节点,通过引入该成员构建
					 *内核对象之间的层次关系,父对象的指针*/

	struct kset		*kset;	/*内核对象所属的kset对象的指针,kset对象代表一个
					*subsystem,其中容纳了一系列同类型的kobject对象*/

	/*定义了该内核对象的一组sysfs文件系统相关的操作函数和属性,显然不同类型的内核对象
	会有不同的ktype,用以体现kobject所代表的内核对象的特质,同时内核通过ktype成员
	*将kobject对象的sysfs文件操作与其属性文件关联起来*/
	struct kobj_type	*ktype;

	struct sysfs_dirent	*sd;/*用来表示内核对象在sysfs文件系统中对应的目录项的实例*/

	struct kref		kref;	/*其核心数据是一原子型变量,用来表示内核对象的
					*引用计数,内核通过该成员追踪内核对象的生命周期*/

	unsigned int state_initialized:1;/*表示该kobject所代表的内核对象初始化的状态,
					  *1表示对象已被初始化,0表示未初始化*/

	unsigned int state_in_sysfs:1;	/*表示该kobject所代表的内核对象有没有在sysfs文件
					 *中建立一个入口点*/

	unsigned int state_add_uevent_sent:1;
	unsigned int state_remove_uevent_sent:1;

	/*如果该kobject对象隶属于某一个kset,那么它的状态变化可以导致其所在的kset对象向
	 *用户空间发送event消息,成员uevent_suppress用来表示当该kobject状态发生变化时,
	 *是否让其所在的kset向用户空间发送event消息,值1表示不让kset发送这种event消息*/
	unsigned int uevent_suppress:1;	
};

/*设置kobject中的name*/
extern int kobject_set_name(struct kobject *kobj, const char *name, ...)
			    __attribute__((format(printf, 2, 3)));
extern int kobject_set_name_vargs(struct kobject *kobj, const char *fmt,
				  va_list vargs);

static inline const char *kobject_name(const struct kobject *kobj)
{
	return kobj->name;
}

/*kobject初始化函数,设置 kobject 引用计数为 1*/
extern void kobject_init(struct kobject *kobj, struct kobj_type *ktype);
extern int __must_check kobject_add(struct kobject *kobj,
				    struct kobject *parent,
				    const char *fmt, ...);
/*kobject注册函数，该函数只是kobject_init和kobject_add_varg的简单组合*/
extern int __must_check kobject_init_and_add(struct kobject *kobj,
					     struct kobj_type *ktype,
					     struct kobject *parent,
					     const char *fmt, ...);

/*从linux设备曾测中(hierarchy)中删除kobj对象*/
extern void kobject_del(struct kobject *kobj);

extern struct kobject * __must_check kobject_create(void);
extern struct kobject * __must_check kobject_create_and_add(const char *name,
						struct kobject *parent);

extern int __must_check kobject_rename(struct kobject *, const char *new_name);
extern int __must_check kobject_move(struct kobject *, struct kobject *);

/*增加或减少kobject的引用计数*/
extern struct kobject *kobject_get(struct kobject *kobj);
extern void kobject_put(struct kobject *kobj);

extern char *kobject_get_path(struct kobject *kobj, gfp_t flag);

/*show 相当于 read,store 相当于 write。
struct attribute **default_attrs;是属性数组。在 sysfs 中,kobject 对应目录,
kobject 的属性对应这个目录下的文件。调用 show 和 store 函数来读写文件,
就可以得到属性中的内容*/
struct kobj_type {
	void (*release)(struct kobject *kobj);	/*释放kobject使用release函数*/
	struct sysfs_ops *sysfs_ops;		/*sysfs_ops 是指向如何读写的函数的指针*/
	struct attribute **default_attrs;	/*属性数组*/
};

struct kobj_uevent_env {
	char *envp[UEVENT_NUM_ENVP];
	int envp_idx;
	char buf[UEVENT_BUFFER_SIZE];
	int buflen;
};

/*对热插拔事件的控制*/
struct kset_uevent_ops {
	int (*filter)(struct kset *kset, struct kobject *kobj);
	const char *(*name)(struct kset *kset, struct kobject *kobj);
	int (*uevent)(struct kset *kset, struct kobject *kobj,
		      struct kobj_uevent_env *env);
};

struct kobj_attribute {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf);
	ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count);
};

extern struct sysfs_ops kobj_sysfs_ops;

/**
 * struct kset - a set of kobjects of a specific type, belonging to a specific subsystem.
 *
 * A kset defines a group of kobjects.  They can be individually
 * different "types" but overall these kobjects all want to be grouped
 * together and operated on in the same manner.  ksets are used to
 * define the attribute callbacks and other common events that happen to
 * a kobject.
 *
 * @list: the list of all kobjects for this kset
 * @list_lock: a lock for iterating over the kobjects
 * @kobj: the embedded kobject for this kset (recursion, isn't it fun...)
 * @uevent_ops: the set of uevent operations for this kset.  These are
 * called whenever a kobject has something happen to it so that the kset
 * can add new environment variables, or filter out the uevents if so
 * desired.
 */
struct kset {
	struct list_head list;	/*用于连接该kset中所有kobject的链表头*/
	spinlock_t list_lock;	/*用于互斥访问*/
	struct kobject kobj;	/*嵌入的kobject*/
	struct kset_uevent_ops *uevent_ops;/*对热插拔事件的控制*/
};

extern void kset_init(struct kset *kset);
extern int __must_check kset_register(struct kset *kset);
extern void kset_unregister(struct kset *kset);
extern struct kset * __must_check kset_create_and_add(const char *name,
						struct kset_uevent_ops *u,
						struct kobject *parent_kobj);

static inline struct kset *to_kset(struct kobject *kobj)
{
	return kobj ? container_of(kobj, struct kset, kobj) : NULL;
}

static inline struct kset *kset_get(struct kset *k)
{
	return k ? to_kset(kobject_get(&k->kobj)) : NULL;
}

static inline void kset_put(struct kset *k)
{
	kobject_put(&k->kobj);
}

static inline struct kobj_type *get_ktype(struct kobject *kobj)
{
	return kobj->ktype;
}

extern struct kobject *kset_find_obj(struct kset *, const char *);

/* The global /sys/kernel/ kobject for people to chain off of */
extern struct kobject *kernel_kobj;
/* The global /sys/kernel/mm/ kobject for people to chain off of */
extern struct kobject *mm_kobj;
/* The global /sys/hypervisor/ kobject for people to chain off of */
extern struct kobject *hypervisor_kobj;
/* The global /sys/power/ kobject for people to chain off of */
extern struct kobject *power_kobj;
/* The global /sys/firmware/ kobject for people to chain off of */
extern struct kobject *firmware_kobj;

#if defined(CONFIG_HOTPLUG)
int kobject_uevent(struct kobject *kobj, enum kobject_action action);
int kobject_uevent_env(struct kobject *kobj, enum kobject_action action,
			char *envp[]);

int add_uevent_var(struct kobj_uevent_env *env, const char *format, ...)
	__attribute__((format (printf, 2, 3)));

int kobject_action_type(const char *buf, size_t count,
			enum kobject_action *type);
#else
static inline int kobject_uevent(struct kobject *kobj,
				 enum kobject_action action)
{ return 0; }
static inline int kobject_uevent_env(struct kobject *kobj,
				      enum kobject_action action,
				      char *envp[])
{ return 0; }

static inline int add_uevent_var(struct kobj_uevent_env *env,
				 const char *format, ...)
{ return 0; }

static inline int kobject_action_type(const char *buf, size_t count,
				      enum kobject_action *type)
{ return -EINVAL; }
#endif

#endif /* _KOBJECT_H_ */
