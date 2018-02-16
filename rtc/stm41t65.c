#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>

#include "stm41t65.h"

#define WDT_TIMEOUT_DEFAULT 1
#define WDT_RESOLUTION_DEFAULT 0x02

int wdog_timeout = WDT_TIMEOUT_DEFAULT;
module_param(wdog_timeout, int, S_IRUGO);
MODULE_PARM_DESC(wdog_timeout, "Watchdog timeout period, default 1");

int wdog_resolution = WDT_RESOLUTION_DEFAULT;
module_param(wdog_resolution, int, S_IRUGO);
MODULE_PARM_DESC(wdog_resolution, "Watchdog timeout resolution, default 1 s");

/*this is needed for watchdog*/
struct i2c_client *gclient = NULL;

/*
 * This is my dts for i2c controller
	i2c@116500 { **adapter
		#address-cells = <0x1>;
		#size-cells = <0x0>;
		cell-index = <0x0>;
		compatible = "fsl-i2c";
		reg = <0x116500 0x100>;
		interrupts = <0x26 0x2 0x0 0x0>;
		dfsrr;

		vpd@57 { **client 1
			compatible = "jedec,jc42";
			reg = <0x1f>;
		};

		/my rtc is at i2c slave address, 0x68
		during creating the client nodes it will ignore st from the string
		/
		rtc_wdt@68 { **client 2
			compatible = "st,m41t65";
			reg = <0x68>;
		};

		user_eeprom@55 {
			compatible = "atmel,24c512";
			reg = <0x55>;
		};

		accel@1d {
			compatible = "fsl,mma8451";
			reg = <0x1d>;
		};
	};

 * this id table recoganizes the device. Once the driver 
 * registers, immediately probe will get invoked if it is
 * matching with the dts entry
 * */

static const struct i2c_device_id m41t65_idtable[] = {
	{ "m41t65", 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, m41t65_idtable);

static DEFINE_MUTEX(m41t65_rtc_mutex);
static volatile unsigned long m41t65_is_open;

static int m41t65_get_datetime(struct i2c_client *client,
		struct rtc_time *tm)
{
	u8 buf[M41T65_DATETIME_REG_SIZE], dt_addr[1] = { M41T65_REG_SEC };
	/*for read we need two i2c transfers 
	 * start->7/10 bit slave_address+r/w=0->ack->wordAddress->ack
	 * now the pointer inside the rtc chip is set to wordAddress
	 * start->7/10 bit slave_address+r/w=1->ack->Data->ack
	 * flags = 0 write
	 * 		 = 1 read
	 */
	struct i2c_msg msgs[] = {
		{
			.addr   = client->addr,
			.flags  = 0,
			.len    = 1,
			.buf    = dt_addr,
		},
		{
			.addr   = client->addr,
			.flags  = I2C_M_RD,
			.len    = M41T65_DATETIME_REG_SIZE - M41T65_REG_SEC,
			.buf    = buf + M41T65_REG_SEC,
		},
	};
	printk(KERN_INFO"Anil calling rtc get\n");

	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		dev_err(&client->dev, "read error\n");
		return -EIO;
	}

	tm->tm_sec = bcd2bin(buf[M41T65_REG_SEC] & 0x7f);
	tm->tm_min = bcd2bin(buf[M41T65_REG_MIN] & 0x7f);
	tm->tm_hour = bcd2bin(buf[M41T65_REG_HOUR] & 0x3f);
	tm->tm_mday = bcd2bin(buf[M41T65_REG_DAY] & 0x3f);
	tm->tm_wday = buf[M41T65_REG_WDAY] & 0x07;
	tm->tm_mon = bcd2bin(buf[M41T65_REG_MON] & 0x1f) - 1;

	tm->tm_year = bcd2bin(buf[M41T65_REG_YEAR]) + 100;

	printk(KERN_INFO"%d/%d/%d, %d:%d:%d\n", 
			1900+tm->tm_year, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	return rtc_valid_tm(tm);
}

static int m41t65_set_datetime(struct i2c_client *client,
		struct rtc_time *tm)
{
	u8 wbuf[M41T65_DATETIME_REG_SIZE], dt_addr[1] = { M41T65_REG_SEC };
	u8 *buf = &wbuf[1];
	struct i2c_msg msg_rd[] = {
		{
			.addr   = client->addr,
			.flags  = 0,
			.len    = 1,
			.buf    = dt_addr,
		},
		{
			.addr   = client->addr,
			.flags  = I2C_M_RD,
			.len    = M41T65_DATETIME_REG_SIZE - M41T65_REG_SEC,
			.buf    = buf + M41T65_REG_SEC,
		},
	};
	struct i2c_msg msg_wr[]={
		{
			.addr = client->addr,
			.flags = 0,
			.len = M41T65_DATETIME_REG_SIZE,
			.buf = wbuf,
		},
	};
	printk(KERN_INFO"Anil calling rtc set\n");

	//read the device first. Only one nibble is time information. 
	//Rest can be controlling bit
	if (i2c_transfer(client->adapter, msg_rd, 2) < 0) {
		dev_err(&client->dev, "read error\n");
		return -EIO;
	}

	//create the buffer to write.
	wbuf[0] = 0; /* offset into rtc's regs */
	buf[M41T65_REG_SSEC] = 0;
	buf[M41T65_REG_SEC] =
		bin2bcd(tm->tm_sec) | (buf[M41T65_REG_SEC] & ~0x7f);
	buf[M41T65_REG_MIN] =
		bin2bcd(tm->tm_min) | (buf[M41T65_REG_MIN] & ~0x7f);
	buf[M41T65_REG_HOUR] =
		bin2bcd(tm->tm_hour) | (buf[M41T65_REG_HOUR] & ~0x3f);
	buf[M41T65_REG_WDAY] =
		(tm->tm_wday & 0x07) | (buf[M41T65_REG_WDAY] & ~0x07);
	buf[M41T65_REG_DAY] =
		bin2bcd(tm->tm_mday) | (buf[M41T65_REG_DAY] & ~0x3f);
	buf[M41T65_REG_MON] =
		bin2bcd(tm->tm_mon + 1) | (buf[M41T65_REG_MON] & ~0x1f);
	buf[M41T65_REG_YEAR] = bin2bcd(tm->tm_year % 100);

	if (i2c_transfer(client->adapter, msg_wr, 1) != 1) {
		dev_err(&client->dev, "write error\n");
		return -EIO;
	}
	
	return 0;
}

static int m41t65_rtc_read_time(struct device *rtcdev, struct rtc_time *tm)
{
	int ret = 0;

	if (!tm){
		dev_err(rtcdev, "Invalid parameter for rtc get time\n");
		return -1;
	}
	
	ret = m41t65_get_datetime(to_i2c_client(rtcdev), tm);

	return ret;
}

static int m41t65_rtc_set_time(struct device *rtcdev, struct rtc_time *tm)
{
	int ret = 0;

	if (!tm){
		dev_err(rtcdev, "Invalid parameter for rtc set time");
		return -1;
	}
	
	ret = m41t65_set_datetime(to_i2c_client(rtcdev), tm);

	return ret;
}

static struct rtc_class_ops m41t65_rtc_ops = {
	.read_time = m41t65_rtc_read_time,
	.set_time = m41t65_rtc_set_time,
	//.proc = m41t65_rtc_proc,
};

/*lets make this one time opening and seek disabled*/
int wdt_open (struct inode *inode, struct file *file)
{
	printk(KERN_INFO"<Anil> in %s\n", __func__);
	if (MINOR(inode->i_rdev) == WATCHDOG_MINOR){
		mutex_lock(&m41t65_rtc_mutex);
		if(test_and_set_bit(0, &m41t65_is_open)){
			printk(KERN_INFO"Device busy\n");
			mutex_unlock(&m41t65_rtc_mutex);
			return -EBUSY;
		}

		m41t65_is_open = 1;
		mutex_unlock(&m41t65_rtc_mutex);
		return nonseekable_open(inode, file);
	}

	return -ENODEV;
}

ssize_t wdt_read (struct file *file, char __user *rd_buf, size_t sz, loff_t *offset)
{
	printk(KERN_INFO"<Anil> in %s\n", __func__);

	return 0;
}

static int wdt_ping(void)
{
	u8 msg_buf[2] = {0};
	struct i2c_msg msg_ping_rd[2] = {
		{
			.addr = gclient->addr,
			.flags = 0, 
			.len = 1,
			.buf = msg_buf,
		},
		{
			.addr = gclient->addr,
			.flags = I2C_M_RD, 
			.len = 1,
			.buf = msg_buf,
		}
	};

	struct i2c_msg msg_ping_wr[1] = {
		{
			.addr = gclient->addr,
			.flags = 0, 
			.len = 2,
			.buf = msg_buf,
		}
	};

	msg_buf[0] = M41T65_REG_WDOG;
	if (i2c_transfer(gclient->adapter, msg_ping_rd, 2) < 0){
		dev_err(&gclient->dev, "read error\n");
		return -EIO;
	}

	printk(KERN_INFO"Current watchdog value is 0x%02x\n", msg_buf[1]);
	msg_buf[0] = M41T65_REG_WDOG;
	msg_buf[1] |= (((wdog_resolution << 4)| wdog_resolution) & M41T65_WDOG_MUL_MASK);
	msg_buf[1] |= ((wdog_timeout << 1) & M41T65_WDOG_RES_MASK);

	printk(KERN_INFO"wdog reg = 0x%02x\n", msg_buf[0]);
	if (i2c_transfer(gclient->adapter, msg_ping_wr, 1) < 0){
		dev_err(&gclient->dev, "read error\n");
		return -EIO;
	}

	return 0;
}

ssize_t wdt_write (struct file *file, const char __user *wr_buf, size_t sz, loff_t *offset)
{
	u8 *buf = (u8*)kzalloc(sz, GFP_KERNEL);

	if (!buf){
		return -ENOMEM;
	}
	copy_from_user(buf, wr_buf, sz);
	printk(KERN_INFO"<Anil> in %s, userdata = %s\n", __func__, buf);

	if (sz){
		wdt_ping();
		return sz;
	}
	kfree(buf);

	//interesting!!!
	//if you return 0, echo 1 > /dev/m41t65wdt will keep on writing as echo will try to write till it 
	//is finished with data echo 1234 will loop 4 times if it is returning 1, EPERM is an error, so 
	//echo will stop trying after 1 try
	//return 0;
	
	return -EPERM; 
}

long wdt_ioctl (struct file *file, unsigned int ioctl_req, unsigned long arg)
{
	printk(KERN_INFO"<Anil> in %s\n", __func__);

	return 0;
}

int wdt_mmap (struct file *file, struct vm_area_struct *vm_area)
{
	printk(KERN_INFO"<Anil> in %s\n", __func__);

	return 0;
}

int wdt_release (struct inode *inode, struct file *file)
{
	printk(KERN_INFO"<Anil> in %s\n", __func__);

	if (MINOR(inode->i_rdev) == WATCHDOG_MINOR)
		clear_bit(0, &m41t65_is_open);

	return 0;
}

static const struct file_operations m41t65_fops = {
	.owner = THIS_MODULE,
	.open = wdt_open,
	.read = wdt_read,
	.write = wdt_write,
	.mmap = wdt_mmap,
	.unlocked_ioctl = wdt_ioctl,
	.release = wdt_release,
};

static struct miscdevice m41t65wdt = {
	.minor = WATCHDOG_MINOR,
	.name = "m41t65wdt",
	.fops = &m41t65_fops,
};

static int m41t65_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct rtc_time cur_time = {0};
	struct rtc_device *rtc = NULL;
	int rc = 0;

	printk(KERN_INFO"probing the i2c device\n");
	if (client)
		printk(KERN_INFO"name = %s, addr = 0x%x\n", client->name, client->addr);

	//this will create an entry in /dev as /dev/rtc0 linked to /dev/rtc. What if more number
	//of device are there??
	rtc = devm_rtc_device_register(&client->dev, client->name, &m41t65_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		rc = PTR_ERR(rtc);
		rtc = NULL;
		goto exit;
	}
	//i want this rtc to deregister from remove as an argument to unregister function
	//if I have something more, make this into a structure and add more info.
	i2c_set_clientdata(client, rtc);

	//this will create a node with '/dev/<m41t65wdt.name> and can be used to do the file
	//operations
	misc_register(&m41t65wdt);

	gclient = client;	

	printk(KERN_INFO"watchdog timeout %s to %d\n", (wdog_timeout == WDT_TIMEOUT_DEFAULT) ? "set":"modified", wdog_timeout);
	m41t65_get_datetime(client, &cur_time);

	return 0;

exit:
	return -1;
}

static int m41t65_remove(struct i2c_client *client)
{
	struct rtc_device *rtc = (struct rtc_device *)i2c_get_clientdata(client);

	printk(KERN_INFO"Removing the i2c device\n");
	misc_deregister(&m41t65wdt);
	devm_rtc_device_unregister(&client->dev, rtc);

	return 0;
}

static struct i2c_driver m41t65 = {
	.driver = {
		.name = M41T65_DEVICE,
	},
	.id_table = m41t65_idtable,
	.probe = m41t65_probe,
	.remove = m41t65_remove,
};

#if 0
__init static int m41t65_init(void)
{
	printk(KERN_INFO"Initializing STM M41T65\n");
	return 0;
}

__exit static void m41t65_exit(void)
{
	printk(KERN_INFO"Exiting STM M41T65\n");
	return;
}

module_init(m41t65_init);
module_exit(m41t65_exit);
#else
module_i2c_driver(m41t65);
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnilJ");
