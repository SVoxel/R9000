#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>

#define INPUT_REG  0x00
#define OUTPUT_REG 0x01
#define POLARITY_REG 0x02
#define CONFIG_REG 0x03

#define I2C_ADDR  0x20  //7 bit 
#define I2C_READ_ADDR 0x41 //8 bit
#define I2C_WRITE_ADDR 0x40 //8 bit

#define LED_ONOFF1_HIGH (1<<0)
#define USB0_DISABLE_HIGH (1<<1)
#define USB1_DISABLE_HIGH (1<<2)
#define IRQ_USB_EN_HIGH (1<<3)
#define DSL_RST (1<<4)
#define PE_RST_82A (1<<5)

#define IPQ_USB 2
#define USB0    0
#define USB1	1
#define LED_Antenna 3

static struct i2c_client *g_client;

int pca9554_read(u8 reg)
{
	int ret;
	printk(" client name=[%s]\n ",g_client->name);
	printk(" client addr =[%x]\n",(g_client->addr & (~(1<<7))));
	if(g_client == NULL)
	{
		printk(KERN_ERR "pca9554 i2c g_client dont exit\n");
		return -1;
	}
	ret = i2c_smbus_read_byte_data(g_client,reg);
	if(ret < 0){
		printk(KERN_ERR "pca9554 i2c read fail! reg=%d \n",reg);
	}
	return ret;
}

int pca9554_write(u8 reg, u8 val)
{
	int ret;

	if(g_client == NULL)
	{
		printk(KERN_ERR "pca9554 i2c g_client dont exit(write)\n");
		return -1;
	}

	printk("ready to write value= [%d]\n",val);
	ret = i2c_smbus_write_byte_data(g_client,reg,val);
	
	if(ret < 0){
		printk(KERN_ERR "pca9554 i2c write fail! reg =%d\n",reg);
		//status = -EIO;
	}

	return ret;
}

int pca9554_config(u8 port ,u8 val)
{
		unsigned int output_val=0;
		unsigned int config_val=0;
		unsigned int Port_config=0;

		output_val = pca9554_read(OUTPUT_REG);
		config_val = pca9554_read(CONFIG_REG);
		//printk("Before write:\n");
		//printk("output_val: %x\n",output_val);
		//printk("config_val: %x\n",config_val);
	
		switch(port)
		{
			case IPQ_USB:
				Port_config = IRQ_USB_EN_HIGH;
			  break;
			case USB0:
				Port_config = USB0_DISABLE_HIGH;
			  break;
			case USB1:
				Port_config = USB1_DISABLE_HIGH;
			  break;
			case LED_Antenna: 
				Port_config = LED_ONOFF1_HIGH;
			  break;
		}

		if (!val) { // disable ---0 or negetive
			// reg must be output 0 config also 0
			output_val &= (~Port_config);
			config_val &= (~Port_config);
        } else {
			config_val &= (~Port_config);
			output_val |= Port_config;
        }
		
		pca9554_write(CONFIG_REG,config_val);
		pca9554_write(OUTPUT_REG,output_val);
		
		return 0;
}
EXPORT_SYMBOL_GPL(pca9554_config);

/* sysfs */
static ssize_t show_I2C_pca9554(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int output_val=0;
	unsigned int config_val=0;
	unsigned int input_val=0;
	unsigned int polarity_val=0;
	output_val = pca9554_read(OUTPUT_REG);
	config_val = pca9554_read(CONFIG_REG);
	input_val = pca9554_read(INPUT_REG);
	polarity_val = pca9554_read(POLARITY_REG);
	return  sprintf(buf,"output[%x];input[%x];config[%x],polarity[%x] \n",output_val,input_val,config_val,polarity_val);
}

static ssize_t set_I2C_pca9554(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int buffer[2]={0xc0,0xc0};
//printf("buf = %s\n",buf);
	if(sscanf(buf, "%u %u\n",&buffer[0],&buffer[1]) > 2)
	{
		printk("It seems sscanf not ok !");
		return -EINVAL;
	}
	pca9554_write(CONFIG_REG,buffer[0]);
	pca9554_write(OUTPUT_REG,buffer[1]);
	return count;
}

static DEVICE_ATTR(externoutput, S_IRUGO | S_IWUSR, show_I2C_pca9554, set_I2C_pca9554);

static struct attribute *pca9554_sysfs_entries[] = {
	&dev_attr_externoutput.attr,
	NULL
};

static struct attribute_group pca9554_attr_group = {
	.name = NULL,
	.attrs = pca9554_sysfs_entries,
};

#ifdef CONFIG_PM
static int pca9554_suspend(struct i2c_client *client, pm_message_t state)
{
	printk("pca9554 suspend\n");
	return 0;
}

static int pca9554_resume(struct i2c_client *client)
{
	printk("pca9554 resume\n");
	return 0;
}

#else
#define pca9554_suspend NULL
#define pca9554_resume  NULL
#endif

static int pca9554_shutdown(struct i2c_client *client)
{
        printk("pca9554 shutdown\n");
        return 0;
}

static int pca9554_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret=0;

	printk(KERN_INFO "pca9554 probe\n");
	
	g_client = client;
	
	/* Test workaround: disable USB_EN by init */
	printk(KERN_INFO "pca9554 probe disable USB_EN.\n");
	pca9554_config(IPQ_USB,0);

	ret = sysfs_create_group(&client->dev.kobj, &pca9554_attr_group);
	if(ret) {
		printk("error creating sysfs group\n");
		return -1;
	}
	
	return 0;
}

static int pca9554_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &pca9554_attr_group);
	return 0;
}

static const struct i2c_device_id pca9554_id[] = {
	{"pca9554", 0},
	{}
};

static struct i2c_driver pca9554_driver = {
	.driver = {
	 	.name = "pca9554",
	},
	.probe = pca9554_probe,
	.remove = pca9554_remove,
	.id_table = pca9554_id,
	.suspend = pca9554_suspend,
	.resume = pca9554_resume,
	.shutdown = pca9554_shutdown,
};

static int __init pca9554_init(void)
{
	printk("now enter pca9554_init!");
	return i2c_add_driver(&pca9554_driver);
}

static void __exit pca9554_exit(void)
{
	//remove_proc_entry("simple_pca",&proc_root); proc_root undeclaired?
	//remove_proc_entry("simple_pca",NULL);
	i2c_del_driver(&pca9554_driver);
}

module_init(pca9554_init);
//subsys_initcall(pca9554_init);
module_exit(pca9554_exit);

MODULE_DESCRIPTION("pca9554 driver");
MODULE_LICENSE("GPL");

