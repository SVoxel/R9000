/*
 * Driver for the ARM General Purpose Input/Output (LXX)
 *
 * 2015.12.23 
 */

#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/bitops.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl061.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>

#define r9000_GPIO_MAJOR	240
#define r9000_GPIO_MAX_MINORS	3
#define LED_IOCTL      0x5311

#define r9000_LED_ON		1
#define r9000_LED_OFF		0

#define COLOR_WHITE	1
#define COLOR_OTHER	0

/* single LED*/
#define SINGLE_LED_USB1		0
#define SINGLE_LED_USB2		1
#define SINGLE_LED_WPS		2
#define SINGLE_LED_WIFI		3
#define SINGLE_LED_DAC      5
#define SINGLE_LED_GUEST_WIFI 7

/* combined LED */
#define COMB_LED_PWR		10
#define COMB_LED_WAN		11
/* blinking LED */
#define BLINK_LED_UPG		20
#define BLINK_LED_USB1		21
#define BLINK_LED_USB3		22
#define BLINK_LED_FASTTRACK	23
/* option for all LED */
#define LED_OPTION_BLINK	33
#define LED_OPTION_ON		34
#define LED_OPTION_OFF		35

#define r9000_GPIO_LED_DAC	30
#define r9000_GPIO_LED_INTERNET 	23
#define r9000_GPIO_LED_PWR	22
#define r9000_GPIO_LED_WIFI	29
#define r9000_GPIO_LED_GUEST_WIFI	35
#define r9000_GPIO_LED_USB1	36
#define r9000_GPIO_LED_USB2	37
#define r9000_GPIO_LED_WPS	39

#define r9000_USB_OFF 0
#define r9000_USB_ON 1

#define DATA_BLINK_TIMEVAL	(11 * HZ / 100)

#define RESET2DEF_TIMEVAL	(5 * HZ)
#define RESET_LED_ONTIME	(25 * HZ / 100)
#define RESET_LED_OFFTIME	(75 * HZ / 100)

#define UPG_LED_ONTIME		(25 * HZ / 100)
#define UPG_LED_OFFTIME		(75 * HZ / 100)

#define BOOT_LED_OFFTIME     (100 * HZ / 100)

#define USB_LED_BLINK_INTV	(50 * HZ / 100)
#define FASTTRACK_LED_BLINK_INTV      (50 * HZ / 100)

#define GPIO_STATUS_PRESS 0
#define GPIO_STATUS_RELEASE 1

#define r9000_GPIO_BTN_WIFI_ONOFF	5
#define r9000_GPIO_BTN_LED_ONOFF	35
#define r9000_GPIO_BTN_WPS		32
#define r9000_GPIO_BTN_RESET		31

#define IPQ_USB 2
#define USB0    0
#define USB1	1
#define LED_Antenna 3

struct gpio_button {
	const char *desc;
	unsigned gpio;
	unsigned int irq;
	struct work_struct work;
	work_func_t func;
};

void (*set_lan_led)(int option) = NULL;
EXPORT_SYMBOL(set_lan_led);

struct gpio_led {
	const char *desc;
	unsigned gpio;
	int init_state;
	int stay_state;
	int cur_state;
	u_int8_t data_detected;
	u_int8_t index;
};

#define LED_INDEX_USB1		0 /* must equal to SINGLE_LED_USB1 */
#define LED_INDEX_USB2		1 /* must equal to SINGLE_LED_USB2 */
#define LED_INDEX_WPS		2 /* must equal to SINGLE_LED_WPS */
#define LED_INDEX_WIFI		3 /* must equal to SINGLE_LED_WIFI */
#define LED_INDEX_PWR		4
#define LED_INDEX_DAC		5
#define LED_INDEX_INTERNET	6
#define LED_INDEX_GUEST_WIFI	7

struct gpio_led r9000_gpio_leds[] = {
	{
		.desc		= "USB1",
		.gpio		= r9000_GPIO_LED_USB1,
		.init_state	= GPIOF_OUT_INIT_LOW,
		.index		= LED_INDEX_USB1,
	},
	{
		.desc		= "USB2",
		.gpio		= r9000_GPIO_LED_USB2,
		.init_state	= GPIOF_OUT_INIT_LOW,
		.index		= LED_INDEX_USB2,
	},
	{
		.desc		= "WPS",
		.gpio		= r9000_GPIO_LED_WPS,
		.init_state	= GPIOF_OUT_INIT_LOW,
		.index		= LED_INDEX_WPS,
	},
	{
		.desc		= "WIFI",
		.gpio		= r9000_GPIO_LED_WIFI,
		.init_state	= GPIOF_OUT_INIT_LOW,
		.index		= LED_INDEX_WIFI,
	},
	{
		.desc		= "power",
		.gpio		= r9000_GPIO_LED_PWR,
		.init_state	= GPIOF_OUT_INIT_LOW,
		.index		= LED_INDEX_PWR,
	},
	{
		.desc		= "DAC",
		.gpio		= r9000_GPIO_LED_DAC,
		.init_state	= GPIOF_OUT_INIT_LOW,
		.index		= LED_INDEX_DAC,
	},
	{
		.desc		= "INTERNET",
		.gpio		= r9000_GPIO_LED_INTERNET,
		.init_state	= GPIOF_OUT_INIT_LOW,
		.index		= LED_INDEX_INTERNET,
	},
	{
		.desc		= "DUEST_WIFI",
		.gpio		= r9000_GPIO_LED_GUEST_WIFI,
		.init_state	= GPIOF_OUT_INIT_HIGH,
		.index		= LED_INDEX_GUEST_WIFI,
	},
};

struct led_priv_t
{
	int led_num;
	int led_color;
	int led_status;
};

static struct gpio_chip *r9000_gpio_chip = NULL;
static struct kobject *r9000_button_obj = NULL;
static struct kset *r9000_button_kset = NULL;

static struct proc_dir_entry *simple_config_entry = NULL;
static struct proc_dir_entry *tricolor_led_entry  = NULL;
static struct proc_dir_entry *all_led = NULL;
static struct proc_dir_entry *button_test = NULL;
static struct proc_dir_entry *usb_en = NULL;
static struct proc_dir_entry *usb_en0 = NULL;
static struct proc_dir_entry *usb_en1 = NULL;
static struct proc_dir_entry *antenna_led = NULL;
static struct proc_dir_entry *guestwifi_led = NULL;
int led_factory = 0;
int button_factory = 0;
EXPORT_SYMBOL(led_factory);
EXPORT_SYMBOL(button_factory);
static int reset_count=0;
static int wps_count=0;
static int wireless_count=0;
static int ledonoff_count=0;

static unsigned int upg_led_status = 1;
static unsigned int reset_led_status = 1;
static unsigned int usb_led_status[2] = {1, 1};
static unsigned int fasttrack_led_status = 1;
static unsigned int boot_led_status = 1;

static struct timer_list data_blink_led_timer;
static struct timer_list upg_led_timer;
static struct timer_list reset_led_timer;
static struct timer_list usb_led_timer[2];
static struct timer_list fasttrack_led_timer;
static struct timer_list boot_led_timer;

static int usb_led_blink_times[2] = {0, 0};
static int fasttrack_led_blink_times = 0;
static int sata_led_blink_times = 0;

static int led_option = LED_OPTION_ON;

static void upg_led_shot(unsigned long val)
{

	if (upg_led_status) {
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_OFF);
		upg_led_status = 0;
		mod_timer(&upg_led_timer, jiffies + UPG_LED_OFFTIME);
	} else {
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_ON);
		upg_led_status = 1;
		mod_timer(&upg_led_timer, jiffies + UPG_LED_ONTIME);
	}

}

static void boot_led_shot(unsigned long val)
{

	if (boot_led_status) {
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_OFF);
		boot_led_status = 0;
		mod_timer(&boot_led_timer, jiffies + BOOT_LED_OFFTIME);
	} else {
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_ON);
		boot_led_status = 1;
		mod_timer(&boot_led_timer, jiffies + BOOT_LED_OFFTIME);
	}
}

static void reset_led_shot(unsigned long val)
{
	del_timer(&boot_led_timer);
	if (reset_led_status) {
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_OFF);
		printk("Factory Reset Mode\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		reset_led_status = 0;
		mod_timer(&reset_led_timer, jiffies + RESET_LED_OFFTIME);
	} else {
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_ON);
		printk("                  \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		reset_led_status = 1;
		mod_timer(&reset_led_timer, jiffies + RESET_LED_ONTIME);
	}
}

static void usb_led_shot(unsigned long val)
{
	int index = (int)val;

	if (led_option == LED_OPTION_OFF)
		return;

	if (usb_led_status[index]) {
		gpio_set_value(index == 0 ? r9000_GPIO_LED_USB1 : r9000_GPIO_LED_USB2, r9000_LED_OFF);
		usb_led_status[index] = 0;
		mod_timer(&usb_led_timer[index], jiffies + USB_LED_BLINK_INTV);
	} else {
		gpio_set_value(index == 0 ? r9000_GPIO_LED_USB1 : r9000_GPIO_LED_USB2, r9000_LED_ON);
		usb_led_status[index] = 1;
		if (--usb_led_blink_times[index] > 0)
			mod_timer(&usb_led_timer[index], jiffies + USB_LED_BLINK_INTV);
		else {
			del_timer(&usb_led_timer[index]);
			struct gpio_led *led = &r9000_gpio_leds[index == 0 ? LED_INDEX_USB1 : LED_INDEX_USB2];
			led->cur_state = led->stay_state = r9000_LED_ON;
		}
	}

}

static void fasttrack_led_shot(unsigned long val)
{
	if (led_option == LED_OPTION_OFF)
		return;
	if (fasttrack_led_status) {
		gpio_set_value(r9000_GPIO_LED_WPS, r9000_LED_OFF);
		fasttrack_led_status = 0;
		mod_timer(&fasttrack_led_timer, jiffies + FASTTRACK_LED_BLINK_INTV);
	} else {
		gpio_set_value(r9000_GPIO_LED_WPS, r9000_LED_ON);
		fasttrack_led_status = 1;
		if (--fasttrack_led_blink_times > 0)
			mod_timer(&fasttrack_led_timer, jiffies + FASTTRACK_LED_BLINK_INTV);
		else
			del_timer(&fasttrack_led_timer);
	}
}

void detect_eth_wan_data(void)
{
	if ( led_factory == 1 )
		return;
	r9000_gpio_leds[LED_INDEX_INTERNET].data_detected = 1;
	//else
	//	printk("ERROR: WAN state is off, but detects data\n");
}

void detect_wifi_data(void)
{
	if ( led_factory == 1 )
		return;
	r9000_gpio_leds[LED_INDEX_WIFI].data_detected = 1;
}

void detect_wifi_5g_data(void)
{
	if ( led_factory == 1 )
		return;
}

void detect_guestwifi_data(void)
{
	if ( led_factory == 1 )
		return;
	r9000_gpio_leds[LED_INDEX_GUEST_WIFI].data_detected = 1;
}

void detect_usb1_data(void)
{
	if ( led_factory == 1 )
		return;
	r9000_gpio_leds[LED_INDEX_USB1].data_detected = 1;
}

void detect_usb3_data(void)
{
	if ( led_factory == 1 )
		return;
	r9000_gpio_leds[LED_INDEX_USB2].data_detected = 1;
}

void detect_sfp_data(void)
{
	if ( led_factory == 1 )
		return;
	r9000_gpio_leds[LED_INDEX_DAC].data_detected = 1;
}

EXPORT_SYMBOL(detect_eth_wan_data);
EXPORT_SYMBOL(detect_wifi_data);
EXPORT_SYMBOL(detect_wifi_5g_data);
EXPORT_SYMBOL(detect_usb1_data);
EXPORT_SYMBOL(detect_usb3_data);
EXPORT_SYMBOL(detect_sfp_data);
EXPORT_SYMBOL(detect_guestwifi_data);

static void data_blink_led_shot(unsigned long val)
{
	int i;
	struct gpio_led *led;
	
	if (led_option == LED_OPTION_OFF)
		return;

	for (i = 0; i < ARRAY_SIZE(r9000_gpio_leds); ++i) {
		led = &r9000_gpio_leds[i];
		if (led->gpio == r9000_GPIO_LED_PWR || led->gpio == r9000_GPIO_LED_WPS)
			continue;
		if (led->data_detected) {
			if (led->stay_state != r9000_LED_ON) {
				if(led->index == LED_INDEX_INTERNET)
				{
					continue;
				}//printk("ERROR: %s stay state is off, but we detect data now\n", led->desc);
			}
			if (led->stay_state != r9000_LED_OFF && led->index == LED_INDEX_GUEST_WIFI)
				continue;

			led->cur_state = led->cur_state == r9000_LED_ON ? r9000_LED_OFF : r9000_LED_ON;
			gpio_set_value(led->gpio, led->cur_state);
			led->data_detected = 0;

		}else{
			if (led->cur_state != led->stay_state) {
				gpio_set_value(led->gpio, led->stay_state);
				led->cur_state = led->stay_state;
			}
		}
	}
	mod_timer(&data_blink_led_timer, jiffies + DATA_BLINK_TIMEVAL);
}

static void inline generic_led_action(int index, int status)
{
	if (led_option == LED_OPTION_OFF)
		return;
	struct gpio_led *led = &r9000_gpio_leds[index];
	gpio_set_value(led->gpio, status);
	led->cur_state = led->stay_state = status;
}

static void inline pwr_led_action(int color, int status)
{
	del_timer(&boot_led_timer);
	if (status == r9000_LED_OFF) {
		
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_OFF);
		return;
	}

	gpio_set_value(r9000_GPIO_LED_PWR, color == COLOR_WHITE ? r9000_LED_ON : r9000_LED_OFF);
}

static void inline wan_led_action(int color, int status)
{
	if (led_option == LED_OPTION_OFF)
		return;
	if(color == COLOR_WHITE)
	{
	if(status == r9000_LED_OFF) {
		gpio_set_value(r9000_GPIO_LED_INTERNET, r9000_LED_OFF);
		r9000_gpio_leds[LED_INDEX_INTERNET].cur_state = r9000_gpio_leds[LED_INDEX_INTERNET].stay_state = r9000_LED_OFF ;
		return;
	}
		gpio_set_value(r9000_GPIO_LED_INTERNET, r9000_LED_ON);
		r9000_gpio_leds[LED_INDEX_INTERNET].cur_state = r9000_gpio_leds[LED_INDEX_INTERNET].stay_state = r9000_LED_ON ;
	}
}

static void inline upg_blink_led_action(int status)
{
	if (status == r9000_LED_ON) {
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_ON);
		upg_led_status = 1;
		mod_timer(&upg_led_timer, jiffies + HZ);
	} else {
		del_timer(&upg_led_timer);
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_ON);
	}
}

static void inline boot_blink_led_action(int status)
{
	if (status == r9000_LED_ON) {
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_ON);
		boot_led_status = 1;
		mod_timer(&boot_led_timer, jiffies + HZ);
	} else {
		del_timer(&boot_led_timer);
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_ON);
	}
}

static void inline usb_blink_led_action(int index, int status)
{
	if (led_option == LED_OPTION_OFF)
		return;
	if (status == r9000_LED_ON) {
		gpio_set_value(index == 0 ? r9000_GPIO_LED_USB1 : r9000_GPIO_LED_USB2, r9000_LED_ON);
		usb_led_status[index] = 1;
		usb_led_blink_times[index] = 5;
		mod_timer(&usb_led_timer[index], jiffies + USB_LED_BLINK_INTV);
	} else {
		del_timer(&usb_led_timer[index]);
		gpio_set_value(index == 0 ? r9000_GPIO_LED_USB1 : r9000_GPIO_LED_USB2, r9000_LED_ON);
	}
}

static void inline fasttrack_blink_led_action(int status)
{
	if (led_option == LED_OPTION_OFF)
		return;
	if (status == r9000_LED_ON) {
		gpio_set_value(r9000_GPIO_LED_WPS, r9000_LED_ON);
		fasttrack_led_status = 1;
		fasttrack_led_blink_times = 20;
		mod_timer(&fasttrack_led_timer, jiffies + FASTTRACK_LED_BLINK_INTV);
	} else {
		del_timer(&fasttrack_led_timer);
		gpio_set_value(r9000_GPIO_LED_WPS, r9000_LED_ON);
	}
}

static void inline led_option_action(int option)
{
	struct gpio_led *led;
	int i;

	if (led_option == option)
		return;

	if (set_lan_led)
		set_lan_led(option);

	if (option == LED_OPTION_BLINK)
		mod_timer(&data_blink_led_timer, jiffies + DATA_BLINK_TIMEVAL);

	if (led_option == LED_OPTION_BLINK) 
		del_timer(&data_blink_led_timer);

	led_option = option;
	for (i = 0; i < ARRAY_SIZE(r9000_gpio_leds); ++i) {
		led = &r9000_gpio_leds[i];
		if (led->gpio == r9000_GPIO_LED_PWR)
			continue;
		if (led->gpio == r9000_GPIO_LED_GUEST_WIFI)
			gpio_set_value(led->gpio, option == LED_OPTION_OFF ? r9000_LED_ON : led->stay_state);
		else
			gpio_set_value(led->gpio, option == LED_OPTION_OFF ? r9000_LED_OFF : led->stay_state);
	}
}

static long r9000_gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct led_priv_t led_conf;

	if (cmd != LED_IOCTL) {
		printk("The LED command is NOT matched!!!\n");
		return -EFAULT;
	}

	if (copy_from_user(&led_conf, (void *)arg, sizeof(struct led_priv_t)))
		return -EFAULT;

	switch (led_conf.led_num) {
		case SINGLE_LED_USB1:
		case SINGLE_LED_USB2:
		case SINGLE_LED_WPS:
		case SINGLE_LED_WIFI:
		case SINGLE_LED_DAC:
		case SINGLE_LED_GUEST_WIFI:
			generic_led_action(led_conf.led_num, led_conf.led_status);
			break;
		case COMB_LED_PWR:
			pwr_led_action(led_conf.led_color, led_conf.led_status);
			break;
		case COMB_LED_WAN:
			wan_led_action(led_conf.led_color, led_conf.led_status);
			break;
		case BLINK_LED_UPG:
			if(led_conf.led_color == COLOR_WHITE)
				boot_blink_led_action(led_conf.led_status);
			else
				upg_blink_led_action(led_conf.led_status);
			break;
		case BLINK_LED_USB1:
		case BLINK_LED_USB3:
			usb_blink_led_action(led_conf.led_num - BLINK_LED_USB1, led_conf.led_status);
			break;
		case BLINK_LED_FASTTRACK:
			fasttrack_blink_led_action(led_conf.led_status);
			break;
		case LED_OPTION_BLINK:
		case LED_OPTION_ON:
		case LED_OPTION_OFF:
			led_option_action(led_conf.led_num);
			break;
	}

	return 0;
}

static int r9000_gpio_open(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t r9000_gpio_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t r9000_gpio_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return count;
}

static int r9000_gpio_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations r9000_gpio_fops = {
	.unlocked_ioctl	= r9000_gpio_ioctl,
	.open		= r9000_gpio_open,
	.read		= r9000_gpio_read,
	.write		= r9000_gpio_write,
	.release	= r9000_gpio_release,
};

static struct cdev r9000_gpio_cdev = {
	.kobj	= {.name = "r9000_gpio", },
	.owner	= THIS_MODULE,
};

static int init_r9000_led(void)
{
	dev_t dev;
	int i, error;
	struct gpio_led *led;

	printk("init led now!!");
#if 0
	error=gpio_request_array(r9000_gpio_leds, ARRAY_SIZE(r9000_gpio_leds));
	if (error < 0) {
		printk("Failed to request GPIO Array");
		goto error;
	}
#endif
	for (i = 0; i < ARRAY_SIZE(r9000_gpio_leds); ++i) {
		led = &r9000_gpio_leds[i];
		if (i != led->index) {
			printk("ERROR: index for %s is error\n", led->desc);
			return -1;
		}
	error = gpio_request_one(led->gpio,led->init_state,led->desc);
	if (error < 0) {
		printk("Failed to request GPIO LED[%d]\n",i);
		goto error;
	}
#if 0
		error = gpio_direction_output(led->gpio, led->init_state);
		if (error < 0) {
			printk("Failed to configure direction for GPIO %s, error %d\n",
				led->desc, error);
			continue;
		}
#endif
		led->data_detected = 0;
		led->stay_state = led->cur_state = led->init_state;
	}

	dev = MKDEV(r9000_GPIO_MAJOR, 0);

	if (register_chrdev_region(dev, r9000_GPIO_MAX_MINORS, "r9000_gpio"))
		goto error;

	cdev_init(&r9000_gpio_cdev, &r9000_gpio_fops);
	if (cdev_add(&r9000_gpio_cdev, dev, r9000_GPIO_MAX_MINORS)) {
		unregister_chrdev_region(dev, r9000_GPIO_MAX_MINORS);
		goto error;
	}

	init_timer(&data_blink_led_timer);
	data_blink_led_timer.data = 0;
	data_blink_led_timer.function = data_blink_led_shot;

	init_timer(&upg_led_timer);
	upg_led_timer.data = 0;
	upg_led_timer.function = upg_led_shot;

	init_timer(&usb_led_timer[0]);
	usb_led_timer[0].data = 0;
	usb_led_timer[0].function = usb_led_shot;

	init_timer(&usb_led_timer[1]);
	usb_led_timer[1].data = 1;
	usb_led_timer[1].function = usb_led_shot;

	init_timer(&fasttrack_led_timer);
	fasttrack_led_timer.data = 0;
	fasttrack_led_timer.function = fasttrack_led_shot;

	init_timer(&boot_led_timer);
	boot_led_timer.data = 0;
	boot_led_timer.function = boot_led_shot;

	return 0;

error:
	printk(KERN_ERR "error register r9000_gpio device\n");
	//gpio_free_array(r9000_gpio_leds, ARRAY_SIZE(r9000_gpio_leds));
	return 1;
}

static int button_test_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{	
    char buffer[50];
    int len;
    len= snprintf(buffer,sizeof(buffer),"reset=%d; wps=%d; wifi=%d; ledonoff_count=%d;\n", reset_count, wps_count, wireless_count, ledonoff_count);
    return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static int button_test_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	const char  buffer[10];
	const size_t maxlen = sizeof(buffer) - 1;
	
	memset(buffer, 0, sizeof(buffer));
	
	if (copy_from_user(buffer, buf, count > maxlen ? maxlen : count))
		return -EFAULT;
    if (strncmp(buffer, "reset", 5) == 0) {
			button_factory = 1;
			reset_count = 0;
			wps_count = 0;
            wireless_count = 0;
			ledonoff_count=0;
	}
	return count;
}

static int all_led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char buffer[3];
    int len;
	len= snprintf(buffer,sizeof(buffer),"%d\n", led_factory);
    return simple_read_from_buffer(buf, count, ppos, buffer, len);
}

static int all_led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	const char  buffer[10];
	const size_t maxlen = sizeof(buffer) - 1;
	
	memset(buffer, 0, sizeof(buffer));
	
	if (copy_from_user(buffer, buf, count > maxlen ? maxlen : count))
		return -EFAULT;

	if (0 != strncmp(buffer,"whiteon",7) && 0 != strncmp(buffer,"whiteoff",8) && 0 != strncmp(buffer,"amberon",7) && 0 != strncmp(buffer,"amberoff",8) && 0 != strncmp(buffer,"off",3))
	{
		printk("cmd error!!");
		return count;
	}

    if (strncmp(buffer,"whiteon",7) == 0) {
		led_factory = 1;
		led_option_action(LED_OPTION_ON);
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_ON);
		gpio_set_value(r9000_GPIO_LED_USB1, r9000_LED_ON);
		gpio_set_value(r9000_GPIO_LED_USB2, r9000_LED_ON);
		gpio_set_value(r9000_GPIO_LED_WPS, r9000_LED_ON);
		gpio_set_value(r9000_GPIO_LED_WIFI, r9000_LED_ON);
		gpio_set_value(r9000_GPIO_LED_GUEST_WIFI, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_DAC, r9000_LED_ON);
		gpio_set_value(r9000_GPIO_LED_INTERNET, r9000_LED_ON);
    } else if (strncmp(buffer,"whiteoff",8) == 0) {
		led_factory = 1;
		led_option_action(LED_OPTION_OFF);
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_USB1, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_USB2, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_WPS, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_DAC, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_WIFI, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_GUEST_WIFI, r9000_LED_ON);
		gpio_set_value(r9000_GPIO_LED_INTERNET, r9000_LED_OFF);
    } else if (strncmp(buffer,"amberon",7) == 0) {
		led_factory = 1;
		led_option_action(LED_OPTION_ON);
		gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_USB1, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_USB2, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_DAC, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_WPS, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_WIFI, r9000_LED_OFF);
		gpio_set_value(r9000_GPIO_LED_GUEST_WIFI, r9000_LED_ON);
		gpio_set_value(r9000_GPIO_LED_INTERNET, r9000_LED_OFF);
    } else if (strncmp(buffer,"amberoff",8) == 0) {
		led_factory = 1;
		led_option_action(LED_OPTION_OFF);
    } else 
		led_factory = 0;
    return count;
}


static int gpio_tricolor_led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static int gpio_tricolor_led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	u_int32_t val;

	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;

	gpio_set_value(r9000_GPIO_LED_WPS, val == 0 ? r9000_LED_OFF : r9000_LED_ON);

	return count;
}

static int usb_enable_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
        return 0;
}

static int usb_enable_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
        u_int32_t val;
        if (sscanf(buf, "%d", &val) != 1)
                return -EINVAL;
		printk("pca9554 config: val=[%d] usb_ipq\n",val);
		pca9554_config(IPQ_USB,val);
        return count;
}

static int usb_0_enable_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
        return 0;
}

static int usb_0_enable_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
        u_int32_t val;

        if (sscanf(buf, "%d", &val) != 1)
                return -EINVAL;
		printk("pca9554 config: val=[%d] usb0\n",val);
		pca9554_config(USB0,val);
		pca9554_config(LED_Antenna,val);
        return count;
}

static int usb_1_enable_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
        return 0;
}

static int usb_1_enable_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
        u_int32_t val;
        if (sscanf(buf, "%d", &val) != 1)
                return -EINVAL;
		printk("pca9554 config: val=[%d] usb1\n",val);
		pca9554_config(USB1,val);
        return count;
}

static int antenna_led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
        return 0;
}

static int antenna_led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
        u_int32_t val;

        if (sscanf(buf, "%d", &val) != 1)
                return -EINVAL;
		pca9554_config(LED_Antenna,val);
        return count;
}

static int guestwifi_led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
        return 0;
}

static int guestwifi_led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
        u_int32_t val;

        if (sscanf(buf, "%d", &val) != 1)
                return -EINVAL;
		
		if (val == 1){
			detect_guestwifi_data();
		}
        return count;
}

static struct file_operations all_led_operation = {
     .read = all_led_read,
	 .write = all_led_write,
};

static struct file_operations button_test_operation = {
     .read = button_test_read,
	 .write = button_test_write,
};

static struct file_operations gpio_tricolor_led_operation = {
     .read = gpio_tricolor_led_read,
	 .write = gpio_tricolor_led_write,
};

static struct file_operations usb_enable_operation = {
     .read = usb_enable_read,
	 .write = usb_enable_write,
};

static struct file_operations usb_0_enable_operation = {
     .read = usb_0_enable_read,
	 .write = usb_0_enable_write,
};

static struct file_operations usb_1_enable_operation = {
     .read = usb_1_enable_read,
	 .write = usb_1_enable_write,
};

static struct file_operations antenna_led_operation = {
     .read = antenna_led_read,
	 .write = antenna_led_write,
};

static struct file_operations guestwifi_led_operation = {
     .read = guestwifi_led_read,
	 .write = guestwifi_led_write,
};

static int create_simple_config_led_proc_entry(void)
{
	simple_config_entry = proc_mkdir("simple_config", NULL);
	if (!simple_config_entry)
		return -ENOENT;

	all_led = proc_create ("all_led", 0644, simple_config_entry,&all_led_operation);
	if (!all_led)
		return -ENOENT;

	button_test = proc_create ("button_test", 0644, simple_config_entry,&button_test_operation);
	if (!button_test)
		return -ENOENT;

	tricolor_led_entry = proc_create ("tricolor_led", 0644,simple_config_entry,&gpio_tricolor_led_operation);
	if (!tricolor_led_entry)
		return -ENOENT;

	usb_en = proc_create ("usb", 0644, simple_config_entry,&usb_enable_operation);
	if (!usb_en)
			return -ENOENT;

	usb_en0 = proc_create ("usb_0", 0644, simple_config_entry,&usb_0_enable_operation);
	if (!usb_en0)
			return -ENOENT;

	usb_en1 = proc_create ("usb_1", 0644, simple_config_entry ,&usb_1_enable_operation);
	if (!usb_en1)
			return -ENOENT;

	antenna_led = proc_create ("antenna_led", 0644, simple_config_entry ,&antenna_led_operation);
	if (!antenna_led)
			return -ENOENT;

	guestwifi_led = proc_create ("guestwifi_led_blink", 0644, simple_config_entry ,&guestwifi_led_operation);
	if (!guestwifi_led)
			return -ENOENT;

	return 0;
}

static irqreturn_t r9000_gpio_button_isr(int irq, void *dev_id)
{
	struct gpio_button *bdata = dev_id;

	BUG_ON(irq != bdata->irq);

	schedule_work(&bdata->work);

	return IRQ_HANDLED;

}

static void release_r9000_button(struct kobject *kobj)
{
	kfree(kobj);
}

static struct kobj_type r9000_button_ktype = {
	.release = release_r9000_button,
};

static int init_r9000_button_obj(void)
{
	int retval;

	r9000_button_kset = kset_create_and_add("button", NULL, kernel_kobj);
	if (!r9000_button_kset)
		return -ENOMEM;

	r9000_button_obj = kzalloc(sizeof(*r9000_button_obj), GFP_KERNEL);
	if (!r9000_button_obj)
		return -ENOMEM;

	r9000_button_obj->kset = r9000_button_kset;
	retval = kobject_init_and_add(r9000_button_obj, &r9000_button_ktype, NULL, "%s", "r9000_button");
	if (retval)
		return retval;

	kobject_uevent(r9000_button_obj, KOBJ_ADD);
	return 0;
}

static void wifi_onoff_gpio_work_func(struct work_struct *work)
{
	//printk("wifi_onoff_gpio_work_func start!!!");
	struct gpio_button *bdata = container_of(work, struct gpio_button, work);
	int state = gpio_get_value(bdata->gpio);
	static unsigned long time_when_press;
	static int wlan_push = 0;
	char *envp[] = {
		"BUTTON=wlan_toggle",
		"BUTTONACTION=pressed",
		NULL
	};

	if (state == GPIO_STATUS_PRESS) {
		if (button_factory == 1)
			wlan_push = 1;
		else {
			time_when_press = jiffies;
			return;
		}
	} else {
		if (button_factory == 1){
			if(wlan_push){
				wireless_count++;
				wlan_push = 0;
			}
		}
	}
	
	if (time_before(jiffies, (time_when_press + HZ))) {
		/* don't hold over 1 second, ignore*/
		return;
	}
	if (button_factory == 0)
		kobject_uevent_env(r9000_button_obj, KOBJ_CHANGE, envp);
}

static void wps_gpio_work_func(struct work_struct *work)
{
	//printk("wps_gpio_work_func start!!!");
	struct gpio_button *bdata = container_of(work, struct gpio_button, work);
	int state = gpio_get_value(bdata->gpio);
	static int wps_push = 0;

	char *envp_press[] = {
		"BUTTON=wps_pbc",
		"BUTTONACTION=pressed",
		NULL
	};
	char *envp_release[] = {
		"BUTTON=wps_pbc",
		"BUTTONACTION=released",
		NULL
	};
	if (state == GPIO_STATUS_PRESS) {
		if (button_factory == 1)
			wps_push = 1;
		else
			kobject_uevent_env(r9000_button_obj, KOBJ_CHANGE, envp_press);
	} else {
		if (button_factory == 1) {
			if(wps_push){
				wps_count++;
				wps_push = 0;
			}
		} else
			kobject_uevent_env(r9000_button_obj, KOBJ_CHANGE, envp_release);
	}
}

static void reset_gpio_work_func(struct work_struct *work)
{
	
	//printk("reset_gpio_work_func start!!!");
	struct gpio_button *bdata = container_of(work, struct gpio_button, work);
	int state = gpio_get_value(bdata->gpio);
	static unsigned long time_when_press;
	static int reset_push = 0;
	char *envp_reboot[] = {
		"BUTTON=reset",
		"BUTTONACTION=reboot",
		NULL
	};
	char *envp_default[] = {
		"BUTTON=reset",
		"BUTTONACTION=default",
		NULL
	};

	if (state == GPIO_STATUS_PRESS) {
			if (button_factory == 1){
				reset_push = 1;
				time_when_press = jiffies;
			} else {
				time_when_press = jiffies;
				boot_blink_led_action(r9000_LED_ON);
				reset_led_status = 1;
				mod_timer(&reset_led_timer, jiffies + RESET2DEF_TIMEVAL);
			}
	} else {
		if (button_factory == 1){
			if(reset_push){
				reset_count++;
				reset_push = 0;
			}
			if (!time_before(jiffies, (time_when_press + RESET2DEF_TIMEVAL)))
				kobject_uevent_env(r9000_button_obj, KOBJ_CHANGE, envp_default);
		} else {
			del_timer(&reset_led_timer);
			del_timer(&boot_led_timer);
			gpio_set_value(r9000_GPIO_LED_PWR, r9000_LED_OFF);

			kobject_uevent_env(r9000_button_obj, KOBJ_CHANGE,
					time_before(jiffies, (time_when_press + RESET2DEF_TIMEVAL)) ? envp_reboot : envp_default);
		}

	}
}

static void led_onoff_work_func(struct work_struct *work)
{
	//printk("led_onoff_work_func start!!!");
	struct gpio_button *bdata = container_of(work, struct gpio_button, work);
	int state = gpio_get_value(bdata->gpio);
	static unsigned long time_when_press;
	static int ledonoff_push = 0;
	char *envp_press[] = {
		"BUTTON=ledonoff",
		"BUTTONACTION=pressed",
		NULL
	};
	if (state == GPIO_STATUS_PRESS) {
		if (button_factory == 1)
			ledonoff_push = 1;
		else {
			time_when_press = jiffies;
			return;
		}
	} else {
		if (button_factory == 1) {
			if(ledonoff_push){
				ledonoff_count++;
				ledonoff_push = 0;
			}
		}
	}

	if (time_before(jiffies, (time_when_press + HZ))) {
		/* don't hold over 1 second, ignore*/
		return;
	}
	if (button_factory == 0)
		kobject_uevent_env(r9000_button_obj, KOBJ_CHANGE, envp_press);
}

static struct gpio_button r9000_gpio_buttons[] = {
	{
		.desc	= "wifi_onoff",
		.gpio	= r9000_GPIO_BTN_WIFI_ONOFF,
		.func	= wifi_onoff_gpio_work_func,
	},
	{
		.desc	= "WPS",
		.gpio	= r9000_GPIO_BTN_WPS,
		.func	= wps_gpio_work_func,
	},
	{
		.desc	= "reset",
		.gpio	= r9000_GPIO_BTN_RESET,
		.func	= reset_gpio_work_func,
	},
};

void init_r9000_button(void)
{
	int i, error;
	struct gpio_button *button;

	printk("init button now!!\n");
#if 0
	error=gpio_request_array(r9000_gpio_buttons, ARRAY_SIZE(r9000_gpio_buttons));
	if (error < 0) {
		printk("Failed to request GPIO Button Array");
		goto free_array;
	}
#endif
	for (i = 0; i < ARRAY_SIZE(r9000_gpio_buttons); ++i) {
	button = &r9000_gpio_buttons[i];
	error = gpio_request_one(button->gpio,GPIOF_IN,button->desc);
	if (error < 0) {
		printk("Failed to request GPIO Button[%d]\n",i);
	//	goto free_array;
	}
#if 0
		error = gpio_direction_input(button->gpio);
		if (error < 0) {
			printk("Failed to configure direction for GPIO %s, error %d\n",
				button->desc, error);
			continue;
		}
#endif
		button->irq = gpio_to_irq(button->gpio);
		if (button->irq < 0) {
			printk("Unable to get irq number for GPIO %s, error %d\n",
				button->desc, button->irq);
			continue;
		}

		INIT_WORK(&button->work, button->func);

		error = request_any_context_irq(button->irq, r9000_gpio_button_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, button->desc, button);
		if (error < 0) {
			printk("Unable to claim irq %d; error %d\n",
				button->irq, error);
		}
	}

	init_timer(&reset_led_timer);
	reset_led_timer.data = 0;
	reset_led_timer.function = reset_led_shot;
    
	return;

free_array:
	//gpio_free_array(r9000_gpio_buttons, ARRAY_SIZE(r9000_gpio_buttons));
	return;
}

static int __init r9000_gpio_init(void)
{
	init_r9000_led();
	create_simple_config_led_proc_entry();
	init_r9000_button_obj();
	init_r9000_button();
}
module_init(r9000_gpio_init);

MODULE_AUTHOR("Baruch Siach <baruch@tkos.co.il>");
MODULE_DESCRIPTION("lxx GPIO driver");

