#include "hw_i2c_config.h"
#include "at32f413.h"

void wk_i2c1_init(void)
{
  /* add user code begin i2c1_init 0 */

  /* add user code end i2c1_init 0 */

  gpio_init_type gpio_init_struct;

  gpio_default_para_init(&gpio_init_struct);

  /* add user code begin i2c1_init 1 */

  /* add user code end i2c1_init 1 */

  /* configure the SCL pin */
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_pins = GPIO_PINS_8;
  gpio_init(GPIOB, &gpio_init_struct);

  /* configure the SDA pin */
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init_struct.gpio_pins = GPIO_PINS_9;
  gpio_init(GPIOB, &gpio_init_struct);

  gpio_pin_remap_config(I2C1_GMUX_0001, TRUE);

  i2c_init(I2C1, I2C_FSMODE_DUTY_2_1, 100000);
  i2c_own_address1_set(I2C1, I2C_ADDRESS_MODE_7BIT, 0x0);
  i2c_ack_enable(I2C1, TRUE);
  i2c_clock_stretch_enable(I2C1, TRUE);
  i2c_general_call_enable(I2C1, FALSE);

  /**
   * Users need to configure I2C1 interrupt functions according to the actual application.
   * 1. Call the below function to enable the corresponding I2C1 interrupt.
   *     --i2c_interrupt_enable(...)
   * 2. Add the user's interrupt handler code into the below function in the at32f413_int.c file.
   *     --void I2C1_EVT_IRQHandler(void)
   *     --void I2C1_ERR_IRQHandler(void)
   */

  /* add user code begin i2c1_init 2 */

  /* add user code end i2c1_init 2 */

  i2c_enable(I2C1, TRUE);

  /* add user code begin i2c1_init 3 */

  /* add user code end i2c1_init 3 */
}


void hw_i2c_config(void)
{
  wk_i2c1_init();
}