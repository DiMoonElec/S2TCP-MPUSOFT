#ifndef __HW_GPIO_CONFIG_H__
#define __HW_GPIO_CONFIG_H__

#define WIZ_RST_PIN    GPIO_PINS_12
#define WIZ_RST_GPIO_PORT    GPIOB
#define WIZ_CS_PIN    GPIO_PINS_9
#define WIZ_CS_GPIO_PORT    GPIOA


void hw_gpio_config(void);

#endif
