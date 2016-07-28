#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/types.h>
#include<linux/device.h>
#include<linux/cdev.h>
#include<linux/mm_types.h>
#include<linux/sched.h>
#include<linux/mount.h>
#include<asm/current.h>

#define PGWLK_NAME "pgwalk"
#define PGWLK_CLASS_NAME "pgwalkc"
#define MAX_DEVNO 1

struct pgwalk_dev{
	dev_t pgw_devno;
	struct class *devclass;
	struct device *device;
	struct cdev pgwalk_cdev;
};

struct pgwalk_dev pgwalk_device;
unsigned long fpage_addr[10];

static struct page* walk_page_table(unsigned long long vaddr)
{
#if 0
	void *page_addr = NULL;
	char hello_str[32] = "Hello World";
	pgd_t *pgd = NULL;
	pud_t *pud = NULL;
	pmd_t *pmd = NULL;
	pte_t *ptep = NULL, pte = {0};
#endif
	struct mm_struct *mm = current->mm;
	pgd_t *pgd = mm->pgd;
	struct page *page = NULL;
	unsigned long pdpe_index = 0;

	pdpe_index = (vaddr & (0x1ffUL << 39)) >> 39;
	printk(KERN_INFO"pdpe_index = 0x%lx\n", pdpe_index);
	pdpe_entry = pgd[pdpe_index];
	printk(KERN_INFO"pgd = 0x%llx, pgd_entry = 0x%llx\n", pgd, __va(pgd[pdpe_index]));
	

#if 0
	pgd = pgd_offset(mm, addr);
	if(pgd_none(*pgd) || pgd_bad(*pgd))
		goto out;
	printk(KERN_INFO"pgd = %lx\n", pgd);

	pgd = pgd_offset(mm, addr);
	if(pgd_none(*pgd) || pgd_bad(*pgd))
		goto out;
	printk(KERN_INFO"pgd = %lx\n", pgd);

	pgd = pgd_offset(mm, addr);
	if(pgd_none(*pgd) || pgd_bad(*pgd))
		goto out;
	printk(KERN_INFO"pgd = %lx\n", pgd);
#endif

	return 0;

out_err:
	return page;
}

int pgwalk_open(struct inode *inode, struct file *fp)
{
	int i = 0;

	printk(KERN_INFO"pgwalk_open\n");
#if 0
	for (i = 0; i < 10; i++){
		fpage_addr[i] = get_zeroed_page(GFP_KERNEL);
		printk(KERN_INFO"start_address :0x%lx, 0x%lx\n", fpage_addr[i], __pa(fpage_addr[i]));
	}
#endif
	return 0;
}

ssize_t pgwalk_write(struct file *fp, const char __user *buf, size_t sz, loff_t *offset)
{
	size_t size = 0;

	printk(KERN_INFO"pgwalk_write offset=%d, size=%d\n", offset, sz);
	return size;
}

ssize_t pgwalk_read(struct file *fp, char __user *buf, size_t sz, loff_t *offset)
{
	size_t size = 0;
	/*get the current process memory. process represented by task_struct
	 * memory map represented by task_struct->mm_struct(mm)
	 * mm_struct have a field vm_area_struct(mmap) which is a linked list of
	 * memory areas*/
	struct mm_struct *cur_mm = current->mm;
	struct vm_area_struct *vmnxt = NULL;
	unsigned long vaddr = 0;

	printk(KERN_INFO"pgwalk_read offset=%d, size=%d, processid=%d\n", *offset, sz, current->pid);

	printk(KERN_INFO"page global directory = 0x%lx\n",cur_mm->pgd);
	
	if (cur_mm){
		vmnxt = cur_mm->mmap;
		/* iterate through different virtual memory areas of the process*/
		while(vmnxt){
			printk(KERN_INFO"0x%lx - 0x%lx, size=%dK, pages=%d\n", vmnxt->vm_start, vmnxt->vm_end,
					(vmnxt->vm_end - vmnxt->vm_start) / 1024, 
					(vmnxt->vm_end - vmnxt->vm_start) / PAGE_SIZE);
			/*file assosicated with the memory area, if not an anonymous mapping
			 * anonymous mapping used for stack,vdso etc anonymous mapping -> if 
			 * a file is not mapped
			 * */
			if (vmnxt->vm_file)
				printk(KERN_INFO"File= %s\n",	vmnxt->vm_file->f_path.dentry->d_iname);
			walk_page_table(vmnxt->vm_start);
			vmnxt = vmnxt->vm_next;
		}
	}

	return size;
}

int pgwalk_release(struct inode *inode, struct file *fp)
{
	int i = 0;

	printk(KERN_INFO"pgwalk_release\n");
#if 0
	for (i = 0; i < 10; i++)
		if (fpage_addr[i])
				free_page(fpage_addr[i]);
#endif
		return 0;
}

/*assosciated file operations with the device*/
struct file_operations pgwalk_ops = {
	.owner = THIS_MODULE,
	.open = pgwalk_open,
	.read = pgwalk_read,
	.write = pgwalk_write,
	.release= pgwalk_release,
};

static int setup_cdev(struct cdev *pgw_cdev, struct file_operations *fops)
{
	int ret = 0;

	cdev_init(pgw_cdev, fops);
	pgw_cdev->owner = THIS_MODULE;
	cdev_add(pgw_cdev, pgwalk_device.pgw_devno, MAX_DEVNO);
	if (ret < 0)
		printk(KERN_ERR"adding pgwalk device to kernel failed\n");

	return ret;
}

/* entry point of the module. when 'insmod pgwalk.ko' this function will be executed */
static int pgwalk_init(void)
{
	int ret = 0, i = 0;
	/*first we need to get some device number
	 * [root@compute pgwalk]# ls -al /dev/pgwlk0 
	 * crw-------. 1 root root 247, 0 Jul 19 18:36 /dev/pgwlk0
	 * c=>char, 247-major number, 0- minor number. filesystem
	 * uses this number to identify the fileoperations from vfs.
	 * */
	ret = alloc_chrdev_region(&pgwalk_device.pgw_devno, 0, MAX_DEVNO, PGWLK_NAME);
	if (ret < 0){
		printk("Failed to allocate new device\n");
		goto errout_alloc;
	}

	/*our char device need to be registered to the kernel cdev_init, cdev_add will add
	 * it to the kernels char subsystem*/
	ret = setup_cdev(&pgwalk_device.pgwalk_cdev, &pgwalk_ops);
	if (ret < 0){
		goto errout_cdev;
	}

	/*which class our device belongs to
	 * [root@compute pgwalk]# ls -al /sys/class/pgwalkc/
	 * lrwxrwxrwx.  1 root root 0 Jul 20 09:32 pgwlk0 -> ../../devices/virtual/pgwalkc/pgwlk0
	 * */
	pgwalk_device.devclass = class_create(THIS_MODULE, PGWLK_CLASS_NAME);
	if (IS_ERR(pgwalk_device.devclass)){
		printk(KERN_INFO"Creating device class failed\n");
		ret = -2;
		goto errout_cc;
	}
	/*create the device node in /dev folder using udev.
	 *[root@compute pgwalk]# ls -al /dev/pgwlk0 
	 *crw-------. 1 root root 247, 0 Jul 20 09:32 /dev/pgwlk0
	 * */
	pgwalk_device.device = device_create(pgwalk_device.devclass, NULL, 
			pgwalk_device.pgw_devno, NULL, "pgwlk%d", i);
	if (!pgwalk_device.device){
		printk(KERN_INFO"Creating pgwalk device node failed\n");
		ret = -3;
		goto errout_dc;
	}

	return 0;

/* if failed handle it appropriately*/
errout_dc:
	device_destroy(pgwalk_device.devclass, pgwalk_device.pgw_devno);
	class_destroy(pgwalk_device.devclass);
errout_cc:
	cdev_del(&pgwalk_device.pgwalk_cdev);
errout_cdev:
	unregister_chrdev_region(pgwalk_device.pgw_devno, MAX_DEVNO);
errout_alloc:
	return ret;
}

/* when you rmmod the device*/
static void pgwalk_exit(void)
{
	device_destroy(pgwalk_device.devclass, pgwalk_device.pgw_devno);
	class_destroy(pgwalk_device.devclass);
	cdev_del(&pgwalk_device.pgwalk_cdev);
	unregister_chrdev_region(pgwalk_device.pgw_devno, MAX_DEVNO);
	return;
}

module_init(pgwalk_init);
module_exit(pgwalk_exit);

MODULE_AUTHOR("AJ");
MODULE_LICENSE("GPL");
