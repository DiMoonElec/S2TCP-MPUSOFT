#include "w5500_port.h"
#include "at32f413.h"
#include "hw_gpio_config.h"

/******************************************************************************/

#define RESET_ACTIVE() gpio_bits_reset(WIZ_RST_GPIO_PORT, WIZ_RST_PIN)
#define RESET_INACTIVE() gpio_bits_set(WIZ_RST_GPIO_PORT, WIZ_RST_PIN)

#define CS_ACTIVE() gpio_bits_reset(WIZ_CS_GPIO_PORT, WIZ_CS_PIN)
#define CS_INACTIVE() gpio_bits_set(WIZ_CS_GPIO_PORT, WIZ_CS_PIN);

static uint8_t flag_rdy = 1;

static w5500_exchange_context_t *w5500_context;
static w5500_exchange_completed_callback_t callback_context;

/******************************************************************************/

/*
  Инициализация интерфейса обмена
*/
void w5500_port_init(w5500_t *c)
{
  DMA1_CHANNEL1->paddr = (uint32_t)&SPI2->dt; //rx
  DMA1_CHANNEL2->paddr = (uint32_t)&SPI2->dt; //tx

  SPI2->ctrl1_bit.swcsil = 1; //CS в неактивном состоянии
  spi_enable(SPI2, TRUE);

  dma_flag_clear(DMA1_GL1_FLAG);
  dma_interrupt_enable(DMA1_CHANNEL1, DMA_FDT_INT, TRUE);

  RESET_ACTIVE();
  for (int i = 0; i < 1000; i++)
    asm("nop");
  RESET_INACTIVE();

  flag_rdy = 1;
}

/*
  Готовность интерфейса обмена
*/
int8_t w5500_port_isrdy(w5500_t *c)
{
  return flag_rdy;
}

int8_t w5500_port_exchange(w5500_exchange_context_t *context,
                           w5500_exchange_completed_callback_t callback)
{
  if (flag_rdy == 0)
    return -1;

  flag_rdy = 0;

  w5500_context = context;
  callback_context = callback;

  CS_ACTIVE();

  DMA1_CHANNEL1->ctrl_bit.chen = FALSE;
  DMA1_CHANNEL2->ctrl_bit.chen = FALSE;

  DMA1_CHANNEL1->maddr = (uint32_t)(context->buff);
  DMA1_CHANNEL1->dtcnt = context->size;

  DMA1_CHANNEL2->maddr = (uint32_t)(context->buff);
  DMA1_CHANNEL2->dtcnt = context->size;

  DMA1_CHANNEL1->ctrl_bit.chen = TRUE;
  DMA1_CHANNEL2->ctrl_bit.chen = TRUE;

  return 0;
}

/******************************************************************************/

void w5500_port_DMA_IRQHandler(void)
{
  DMA1_CHANNEL1->ctrl_bit.chen = FALSE;
  DMA1_CHANNEL2->ctrl_bit.chen = FALSE;
  dma_flag_clear(DMA1_GL1_FLAG);

  CS_INACTIVE();

  if (callback_context != 0)
    callback_context(w5500_context);

  flag_rdy = 1;
}

/******************************************************************************/