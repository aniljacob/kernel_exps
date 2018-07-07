#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>

static int pre_hndlr(struct kprobe *kp, struct pt_regs *regs)
{
	pr_info("do_fork pre handler\n");
	return 0;
}

static void post_hndlr(struct kprobe *kp, struct pt_regs *regs, unsigned long flags)
{
	pr_info("do_fork post handler\n");
	return;
}

static int fault_hndlr(struct kprobe *kp, struct pt_regs *regs, int trap_num)
{
	pr_info("do_fork fault handler\n");
	return 0;
}

struct kprobe kp = {
	.pre_handler = pre_hndlr,
	.post_handler = post_hndlr,
	.fault_handler = fault_hndlr
};

static int kprobe_init(void)
{
	kp.addr = (kprobe_opcode_t *)0xffffffffabc8b060;
	
	register_kprobe(&kp);
	return 0;
}

static void kprobe_exit(void)
{
	unregister_kprobe(&kp);
	return;
}

module_init(kprobe_init);
module_exit(kprobe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnilJaco");
