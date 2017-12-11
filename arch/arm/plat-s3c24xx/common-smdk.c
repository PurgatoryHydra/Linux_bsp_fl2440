/* linux/arch/arm/plat-s3c24xx/common-smdk.c
 *
 * Copyright (c) 2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Common code for SMDK2410 and SMDK2440 boards
 *
 * http://www.fluff.org/ben/smdk2440/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/sysdev.h>
#include <linux/dm9000.h>
#include <linux/platform_device.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <asm/irq.h>

#include <mach/regs-gpio.h>
#include <mach/leds-gpio.h>

#include <plat/nand.h>

#include <plat/common-smdk.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/pm.h>

#include <plat/s3c24xx_gpio.h>

/* LED devices */

static struct gpio_led smdk_led[] = {
	{
		.name = "led0",
		.default_trigger = "heartbeat",
		.gpio 			= 21, 
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
		.active_low = 1,
	},
	{
		.name = "led1",
		.default_trigger = "nand-disk",
		.gpio 			= 22,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
		.active_low = 1,
	},
	{
		.name = "led2",
		.default_trigger = "none",
		.gpio 			= 24,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
		.active_low = 1,
	},
	{
		.name = "led3",
		.default_trigger = "none",
		.gpio 			= 26, 
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
		.active_low = 1,
	},
};

static struct gpio_led_platform_data smdk_pdata_led = {
	.num_leds = 4,
	.leds = smdk_led,
};

static struct platform_device smdk_pdev_led = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev		= {
		.platform_data = &smdk_pdata_led,
	},
};

/* NAND parititon from 2.4.18-swl5 */

static struct mtd_partition smdk_default_nand_part[] = {
	[0] = {
		.name	= "u-boot",
		.size	= SZ_4M,
		.offset	= 0,
	},
	[1] = {
		.name	= "partion jffs2",
		.offset = MTDPART_OFS_APPEND,
		.size	= SZ_8M,
	},
	[2] = {
		.name	= "partion yaffs",
		.offset = MTDPART_OFS_APPEND,
		.size	= MTDPART_SIZ_FULL,
	}
};

static struct s3c2410_nand_set smdk_nand_sets[] = {
	[0] = {
		.name		= "NAND",
		.nr_chips	= 1,
		.nr_partitions	= ARRAY_SIZE(smdk_default_nand_part),
		.partitions	= smdk_default_nand_part,
	},
};

/* DM9000 */

static struct resource s3c_dm9k_resource[] = {
	[0] = {
		.start	= S3C2410_CS4,
		.end	= S3C2410_CS4 + 3,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= S3C2410_CS4 + 4,
		.end	= S3C2410_CS4 + 4 + 3,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= IRQ_EINT7,
		.end	= IRQ_EINT7,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct dm9000_plat_data s3c_dm9k_platdata = {
	.flags = DM9000_PLATF_16BITONLY,
};

static struct platform_device s3c_device_dm9k = {
	.name = "dm9000",
	.id = 0,
	.num_resources = ARRAY_SIZE(s3c_dm9k_resource),
	.resource = s3c_dm9k_resource,
	.dev = {
		.platform_data = &s3c_dm9k_platdata,
	}
};

/* choose a set of timings which should suit most 512Mbit
 * chips and beyond.
*/

static struct s3c2410_platform_nand smdk_nand_info = {
	.tacls		= 20,
	.twrph0		= 60,
	.twrph1		= 20,
	.nr_sets	= ARRAY_SIZE(smdk_nand_sets),
	.sets		= smdk_nand_sets,
};

/* gpio devices. */

static struct s3c24xx_gpio_platform_data *s3c_pdata_gpio;
static struct platform_device *s3c_device_gpio;

static char *gpio_port_name[] = {
	"GPA",
	"GPB",
	"GPC",
	"GPD",
	"GPE",
	"GPF",
	"GPG",
	"GPH",
	"GPJ"
};

static void s3c_gpio_set_platdata(struct s3c24xx_gpio_platform_data *pdata, struct platform_device *pdev)
{
	uint8_t i = 0;
	if(pdata == NULL || pdev == NULL)
	{
		printk(KERN_ERR "s3c_gpio: pdev pdata memory alloc failed. %d %d", pdata, pdev);
	}
	for(i = 0; i < 9; i++)
	{
		pdata[i].name = gpio_port_name[i];
		pdata[i].base = i * 16;
		pdata[i].ngpio = 16;

//		pdata += sizeof(struct s3c24xx_gpio_platform_data);

		pdev[i].name = "s3c24xx-gpio";
		pdev[i].id = i;
		pdev[i].dev.platform_data = pdata + i;

/*		pdev += sizeof(struct platform_device);*/
	}
}

/* devices we initialise */

static struct platform_device __initdata *smdk_devs[12] = {
	&s3c_device_nand,
	&smdk_pdev_led,
	&s3c_device_dm9k,
};

void __init smdk_machine_init(void)
{
	uint8_t i;
	if (machine_is_smdk2443())
		smdk_nand_info.twrph0 = 50;

	s3c_nand_set_platdata(&smdk_nand_info);

	s3c_pdata_gpio = (struct s3c24xx_gpio_platform_data *)kzalloc(9*sizeof(struct s3c24xx_gpio_platform_data), GFP_KERNEL);
	s3c_device_gpio = (struct platform_device *)kzalloc(9*sizeof(struct platform_device), GFP_KERNEL);

	s3c_gpio_set_platdata(s3c_pdata_gpio, s3c_device_gpio);
	for(i = 0; i < 9; i++)
		smdk_devs[3+i] = s3c_device_gpio + i;

	platform_add_devices(smdk_devs, ARRAY_SIZE(smdk_devs));

	s3c_pm_init();
}
