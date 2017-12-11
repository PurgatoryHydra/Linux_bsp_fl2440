#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <asm/gpio.h>
#include <plat/map.h>
#include <plat/s3c24xx_gpio.h>

MODULE_AUTHOR("Song Qiang");
MODULE_DESCRIPTION("GPIO driver for s3c24xx SOCs");
MODULE_LICENSE("MIT");

/* #define S3C24XX_GPIO_DEBUG */

static unsigned int gpio_s3c24xx_usage_count = 0;

#define S3C24XX_CLKCON 	S3C24XX_VA_CLKPWR + 0x0c
#define S3C24XX_GPXCON  getBase(chip)
#define S3C24XX_GPXDAT  (getBase(chip) + 4)
#define S3C24XX_GPXUP 	(getBase(chip) + 8)

static inline unsigned long getBase(struct gpio_chip *chip)
{
	if(chip->base < 128)
	{
		return (unsigned long)(S3C24XX_VA_GPIO + (chip->base / 16) * 0x10);
	}
	else
		return (unsigned long)(S3C24XX_VA_GPIO + 0xd0);
}

static int gpio_s3c24xx_request(struct gpio_chip *chip, unsigned offset)
{
	if(!gpio_s3c24xx_usage_count)
	{
		/* The power is on by default, so actually nothing to do. */
		/*S3C24XX_CLKCON |= 0x1 << 13;*/
	}

	gpio_s3c24xx_usage_count |= 0x1 << offset;

	return 0;
#ifdef S3C24XX_GPIO_DEBUG
	printk(KERN_INFO "s3c24xx-gpio: request, offset: %d", offset);
#endif
}

static void gpio_s3c24xx_free(struct gpio_chip *chip, unsigned offset)
{
	gpio_s3c24xx_usage_count &=~ (0x1 << offset);

	if(!gpio_s3c24xx_usage_count)
	{
		/* shutdown the power. */
	}

#ifdef S3C24XX_GPIO_DEBUG
	printk(KERN_INFO "s3c24xx-gpio: free, offset: %d", offset);
#endif
}

static int gpio_s3c24xx_direction_input(struct gpio_chip *chip, unsigned offset)
{
	unsigned long temp;
	temp = __raw_readl(S3C24XX_GPXCON);
	temp &=~ (0x3 << (offset << 1));
	__raw_writel(temp, S3C24XX_GPXCON);

#ifdef S3C24XX_GPIO_DEBUG
	printk(KERN_INFO "s3c24xx-gpio: direction_input, offset: %d", offset);
#endif
	return 0;
}

static int gpio_s3c24xx_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	unsigned long temp;
	temp = __raw_readl(S3C24XX_GPXCON);
	temp &=~ (0x2 << (offset << 1));
	temp |=  (0x1 << (offset << 1));
	__raw_writel(temp, S3C24XX_GPXCON);

	temp = __raw_readl(S3C24XX_GPXDAT);
	if(value)
		temp |= 0x1 << offset;
	else
		temp &=~ (0x1 << offset);
	__raw_writel(temp, S3C24XX_GPXDAT);

#ifdef S3C24XX_GPIO_DEBUG
	printk(KERN_INFO "s3c24xx-gpio: direction_output offset: %d, value: %d",offset, value);
#endif
	return 0;
}

static void gpio_s3c24xx_set(struct gpio_chip *chip, unsigned offset, int value)
{
	unsigned long temp;
	if(value)
	{
		temp = __raw_readl(S3C24XX_GPXDAT);
		temp |=  (0x1 << offset);
		__raw_writel(temp, S3C24XX_GPXDAT);
	}
	else
	{
		temp = __raw_readl(S3C24XX_GPXDAT);
		temp &=~ (0x1 << offset);
		__raw_writel(temp, S3C24XX_GPXDAT);
	}
#ifdef S3C24XX_GPIO_DEBUG
	printk(KERN_INFO "s3c24xx-gpio: set, offset: %d, value: %d", offset, value);
#endif
}

static int gpio_s3c24xx_get(struct gpio_chip *chip, unsigned offset)
{
	return (__raw_readl(S3C24XX_GPXDAT) & (0x1 << offset));
}

/* gpio_chips for GPA...GPJ. */
static struct gpio_chip *s3c24xx_gpiochip[9];

static int gpio_s3c24xx_remove(struct platform_device *pdev)
{
	struct s3c24xx_gpio_platform_data *pdata = pdev->dev.platform_data;
	int ret;
	ret = gpiochip_remove(s3c24xx_gpiochip[pdata->base/16]);
	if(ret < 0)
	{
		printk(KERN_ERR "s3c24xx gpio platform device %s unbind failed. ", pdata->name);
		return ret;
	}

	printk(KERN_INFO "s3c24xx gpio platform device %s unbind succeeded.", pdata->name);
	return ret;
}

static int __devinit gpio_s3c24xx_probe(struct platform_device *pdev)
{
	struct s3c24xx_gpio_platform_data *pdata = pdev->dev.platform_data;
	int ret;
	s3c24xx_gpiochip[pdata->base/16] = kzalloc(sizeof(struct gpio_chip), GFP_KERNEL);

	s3c24xx_gpiochip[pdata->base/16]->label 			= pdata->name;
	s3c24xx_gpiochip[pdata->base/16]->base 				= pdata->base;
	s3c24xx_gpiochip[pdata->base/16]->ngpio 			= pdata->ngpio;

	s3c24xx_gpiochip[pdata->base/16]->owner 			= THIS_MODULE;
	s3c24xx_gpiochip[pdata->base/16]->request 			= gpio_s3c24xx_request;
	s3c24xx_gpiochip[pdata->base/16]->free 				= gpio_s3c24xx_free;
	s3c24xx_gpiochip[pdata->base/16]->direction_input 	= gpio_s3c24xx_direction_input;
	s3c24xx_gpiochip[pdata->base/16]->direction_output 	= gpio_s3c24xx_direction_output;
	s3c24xx_gpiochip[pdata->base/16]->get 				= gpio_s3c24xx_get;
	s3c24xx_gpiochip[pdata->base/16]->set 				= gpio_s3c24xx_set;
	s3c24xx_gpiochip[pdata->base/16]->can_sleep 		= 0;

	ret = gpiochip_add(s3c24xx_gpiochip[pdata->base/16]);
	if(ret < 0)
	{
		printk(KERN_ERR "s3c24xx gpio platform device %s bind failed. ", pdata->name);
		gpio_s3c24xx_remove(pdev);
		return ret;
	}

	printk(KERN_INFO "s3c24xx gpio platform device %s bind succeeded.", pdata->name);

	return ret;
}


static struct platform_driver gpio_s3c24xx_pdriver = {
	.probe 	= gpio_s3c24xx_probe,
	.remove = gpio_s3c24xx_remove,
	.driver = {
		.name 	= "s3c24xx-gpio",
		.owner 	= THIS_MODULE,
	},
};

static int __init gpio_s3c24xx_init(void)
{
	return platform_driver_register(&gpio_s3c24xx_pdriver);
}

static void __exit gpio_s3c24xx_exit(void)
{
	platform_driver_unregister(&gpio_s3c24xx_pdriver);
}

module_init(gpio_s3c24xx_init);
module_exit(gpio_s3c24xx_exit);
