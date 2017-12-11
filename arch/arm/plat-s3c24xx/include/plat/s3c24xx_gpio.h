#ifndef __S3C24XX_GPIO_H
#define __S3C24XX_GPIO_H

struct s3c24xx_gpio_platform_data{
	int base;
	unsigned int ngpio;
	const char *name;
};

#endif
