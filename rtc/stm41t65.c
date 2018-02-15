#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include "stm41t65.h"

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
 * matching with the dts
 * */

static const struct i2c_device_id m41t65_idtable[] = {
	{ "m41t65", 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, m41t65_idtable);

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

	m41t65_get_datetime(client, &cur_time);

	return 0;

exit:
	return -1;
}

static int m41t65_remove(struct i2c_client *client)
{
	struct rtc_device *rtc = (struct rtc_device *)i2c_get_clientdata(client);

	printk(KERN_INFO"Removing the i2c device\n");
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
