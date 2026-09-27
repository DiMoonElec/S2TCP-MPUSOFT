#include "at32f413.h"
#include "hw_i2c_config.h"
#include "i2c_hw.h"

/*
  I2C1, SCL/SDA настроены и сконфигурированы сгенерированной
  wk_i2c1_init() (GPIO, remap, i2c_init(), own address, ACK,
  clock stretch, general call, i2c_enable). Эта функция вызывается
  один раз извне (в board-init коде) ДО hw_i2c_init().

  wk_i2c1_init() объявлена в сгенерированном файле - просто
  форвард-декларация, чтобы можно было вызвать её повторно из
  hw_i2c_Abort() после программного сброса блока.
*/
extern void wk_i2c1_init(void);

/******************************************************************************/

/*
  phase - состояние конечного автомата транзакции.
  Управляется только из прерываний (кроме момента запуска транзакции
  и финального опроса STOP в hw_i2c_IsRdy()).
*/
enum
{
  PH_IDLE = 0,

  PH_WAIT_SB_W,    // ждём STARTF после Start, затем отправим адрес+W
  PH_WAIT_ADDR_W,  // ждём подтверждения адреса+W (ADDR7F)
  PH_TX,           // передача tx_buf, ожидание TDBE/TDC (EV8_2)

  PH_WAIT_SB_R,    // ждём STARTF после Repeated Start (или первого Start для чистого чтения)
  PH_WAIT_ADDR_R,  // ждём подтверждения адреса+R (ADDR7F)

  PH_RX_SINGLE,    // приём единственного байта
  PH_RX_2BYTE,     // приём ровно двух байт через mackctrl=NEXT (аналог POS=1)
  PH_RX_BULK,      // приём байт 1..N-3 через RDBF (ACK=1, без гонок по времени)
  PH_RX_TAIL1,     // приём байта N-2 через TDC, взвод ACK=0 для байта N
  PH_RX_TAIL2,     // приём байт N-1 и N через TDC + STOP

  PH_WAIT_STOP_CLEAR // финальная фаза: ждём аппаратного сброса бита genstop
};

/******************************************************************************/

static volatile uint8_t phase;
static volatile hw_i2c_status_t status;

static uint8_t slave_addr7;

static const uint8_t *tx_buf;
static volatile uint16_t tx_len;
static volatile uint16_t tx_idx;

static uint8_t *rx_buf;
static volatile uint16_t rx_len;
static volatile uint16_t rx_idx;

/******************************************************************************/

void hw_i2c_init(void)
{
  phase = PH_IDLE;
  status = HW_I2C_OK;

  // GPIO и сам блок I2C1 (частота/адрес/ACK/растяжка такта) уже
  // настроены сгенерированной wk_i2c1_init(), вызванной раньше.
  // Здесь донастраиваем то, чего нет в генераторе - прерывания.
  i2c_interrupt_enable(I2C1, I2C_EVT_INT | I2C_DATA_INT | I2C_ERR_INT, TRUE);

  NVIC_SetPriority(I2C1_EVT_IRQn, 5);
  NVIC_SetPriority(I2C1_ERR_IRQn, 5);
  NVIC_EnableIRQ(I2C1_EVT_IRQn);
  NVIC_EnableIRQ(I2C1_ERR_IRQn);
}

/******************************************************************************/

void hw_i2c_Abort(void)
{
  // На время сброса отключаем прерывания I2C1, чтобы ISR не сработал
  // на "полусброшенных" регистрах и не испортил состояние автомата
  NVIC_DisableIRQ(I2C1_EVT_IRQn);
  NVIC_DisableIRQ(I2C1_ERR_IRQn);

  // Программный сброс блока I2C1 (аналог SWRST на STM32, RM0038
  // 26.6.1) - официальный способ выйти из залоченного состояния.
  // Отпускает SCL/SDA со стороны нашего контроллера и приводит
  // периферию к исходному виду независимо от того, в какой фазе
  // она зависла.
  //
  // ВНИМАНИЕ: если линия прижата к земле физически внешним устройством
  // (а не просто наш I2C1 "запутался" из-за помехи), эта функция не
  // заставит слейва отпустить линию - busyf тут же выставится снова
  // при следующей попытке Start(). Для такого случая нужна отдельная
  // процедура ручного "выталкивания" зависшего слейва тактовыми
  // импульсами SCL (bit-bang recovery) - в данной реализации не
  // предусмотрена, но может быть добавлена отдельной функцией при
  // необходимости.
  i2c_software_reset(I2C1, TRUE);
  i2c_software_reset(I2C1, FALSE);

  // После сброса вся конфигурация блока (частота, duty, адрес, ACK,
  // растяжка такта) потеряна - просто повторно вызываем
  // сгенерированную wk_i2c1_init() вместо дублирования тех же
  // магических констант здесь второй раз (переконфигурация GPIO
  // повторно безобидна).
  wk_i2c1_init();
  i2c_interrupt_enable(I2C1, I2C_EVT_INT | I2C_DATA_INT | I2C_ERR_INT, TRUE);

  NVIC_ClearPendingIRQ(I2C1_EVT_IRQn);
  NVIC_ClearPendingIRQ(I2C1_ERR_IRQn);
  NVIC_EnableIRQ(I2C1_EVT_IRQn);
  NVIC_EnableIRQ(I2C1_ERR_IRQn);

  status = HW_I2C_ERR_ABORTED;
  phase = PH_IDLE;
}

uint8_t hw_i2c_IsRdy(void)
{
  // Для мастера в этой периферии нет отдельного события "STOP отправлен",
  // поэтому завершение STOP проверяется однократным чтением бита genstop
  // прямо здесь (без блокирующего ожидания - вызывающий код сам
  // вызывает IsRdy() в цикле).
  if (phase == PH_WAIT_STOP_CLEAR)
  {
    if (I2C1->ctrl1_bit.genstop == 0)
      phase = PH_IDLE;
  }

  return (phase == PH_IDLE);
}

hw_i2c_status_t hw_i2c_GetStatus(void)
{
  return status;
}

/******************************************************************************/

static void start_transaction(uint8_t addr7,
                               const uint8_t *tx, uint16_t txlen,
                               uint8_t *rx, uint16_t rxlen)
{
  if (!hw_i2c_IsRdy())
    return;

  slave_addr7 = addr7;

  tx_buf = tx;
  tx_len = txlen;
  tx_idx = 0;

  rx_buf = rx;
  rx_len = rxlen;
  rx_idx = 0;

  status = HW_I2C_BUSY;

  if ((txlen == 0) && (rxlen == 0))
  {
    // Пустая транзакция - обмена нет, сразу считаем успешной
    status = HW_I2C_OK;
    phase = PH_IDLE;
    return;
  }

  i2c_master_receive_ack_set(I2C1, I2C_MASTER_ACK_CURRENT); // аналог POS=0
  i2c_ack_enable(I2C1, TRUE);

  phase = (txlen > 0) ? PH_WAIT_SB_W : PH_WAIT_SB_R;

  i2c_start_generate(I2C1);
}

void hw_i2c_Write(uint8_t addr7, const uint8_t *buf, uint16_t len)
{
  start_transaction(addr7, buf, len, 0, 0);
}

void hw_i2c_Read(uint8_t addr7, uint8_t *buf, uint16_t len)
{
  start_transaction(addr7, 0, 0, buf, len);
}

void hw_i2c_WriteRead(uint8_t addr7,
                       const uint8_t *tx_data, uint16_t txlen,
                       uint8_t *rx_data, uint16_t rxlen)
{
  start_transaction(addr7, tx_data, txlen, rx_data, rxlen);
}

/******************************************************************************/

static void abort_transaction(hw_i2c_status_t err)
{
  status = err;

  i2c_master_receive_ack_set(I2C1, I2C_MASTER_ACK_CURRENT);
  i2c_ack_enable(I2C1, TRUE);
  i2c_stop_generate(I2C1);

  phase = PH_WAIT_STOP_CLEAR;
}

/******************************************************************************/

void I2C1_EVT_IRQHandler(void)
{
  uint32_t sts1 = I2C1->sts1;

  switch (phase)
  {
  /////////////////////////////////////////////////////////////////////
  case PH_WAIT_SB_W:
    if (sts1 & I2C_STARTF_FLAG)
    {
      i2c_7bit_address_send(I2C1, slave_addr7, I2C_DIRECTION_TRANSMIT);
      phase = PH_WAIT_ADDR_W;
    }
    break;

  /////////////////////////////////////////////////////////////////////
  case PH_WAIT_ADDR_W:
    if (sts1 & I2C_ADDR7F_FLAG)
    {
      (void)I2C1->sts2; // очистка addr7f (sts1 уже прочитан выше)
      i2c_data_send(I2C1, tx_buf[tx_idx++]);
      phase = PH_TX;
    }
    break;

  /////////////////////////////////////////////////////////////////////
  case PH_TX:
    if (sts1 & I2C_TDBE_FLAG)
    {
      if (tx_idx < tx_len)
      {
        i2c_data_send(I2C1, tx_buf[tx_idx++]);
      }
      else if (sts1 & I2C_TDC_FLAG)
      {
        // EV8_2: последний байт передан и подтверждён
        if (rx_len > 0)
        {
          phase = PH_WAIT_SB_R;
          i2c_start_generate(I2C1); // Repeated Start
        }
        else
        {
          status = HW_I2C_OK;
          i2c_stop_generate(I2C1);
          phase = PH_WAIT_STOP_CLEAR;
        }
      }
      // TDBE=1, но TDC ещё не установлен - ждём следующего прерывания,
      // оно придёт само, когда TDC взведётся (тот же вектор)
    }
    break;

  /////////////////////////////////////////////////////////////////////
  case PH_WAIT_SB_R:
    if (sts1 & I2C_STARTF_FLAG)
    {
      i2c_7bit_address_send(I2C1, slave_addr7, I2C_DIRECTION_RECEIVE);
      phase = PH_WAIT_ADDR_R;
    }
    break;

  /////////////////////////////////////////////////////////////////////
  case PH_WAIT_ADDR_R:
    if (sts1 & I2C_ADDR7F_FLAG)
    {
      if (rx_len == 1)
      {
        // Аналог RM0038 26.3.3: ACK снимается ДО очистки ADDR7F,
        // STOP - после
        i2c_ack_enable(I2C1, FALSE);
        (void)I2C1->sts2;
        i2c_stop_generate(I2C1);
        phase = PH_RX_SINGLE;
      }
      else if (rx_len == 2)
      {
        i2c_ack_enable(I2C1, FALSE);
        i2c_master_receive_ack_set(I2C1, I2C_MASTER_ACK_NEXT); // аналог POS=1
        (void)I2C1->sts2;
        phase = PH_RX_2BYTE;
      }
      else // rx_len >= 3
      {
        i2c_ack_enable(I2C1, TRUE);
        (void)I2C1->sts2;
        phase = PH_RX_BULK;
      }
    }
    break;

  /////////////////////////////////////////////////////////////////////
  case PH_RX_SINGLE:
    if (sts1 & I2C_RDBF_FLAG)
    {
      rx_buf[rx_idx++] = i2c_data_receive(I2C1);
      status = HW_I2C_OK;
      phase = PH_WAIT_STOP_CLEAR;
    }
    break;

  /////////////////////////////////////////////////////////////////////
  case PH_RX_2BYTE:
    if (sts1 & I2C_TDC_FLAG)
    {
      // data1 в DT, data2 уже целиком в сдвиговом регистре
      i2c_stop_generate(I2C1);
      rx_buf[rx_idx++] = i2c_data_receive(I2C1); // data1
      rx_buf[rx_idx++] = i2c_data_receive(I2C1); // data2 (переносится автоматически)
      i2c_master_receive_ack_set(I2C1, I2C_MASTER_ACK_CURRENT);
      status = HW_I2C_OK;
      phase = PH_WAIT_STOP_CLEAR;
    }
    break;

  /////////////////////////////////////////////////////////////////////
  case PH_RX_BULK:
    if (sts1 & I2C_RDBF_FLAG)
    {
      uint16_t remaining = rx_len - rx_idx;

      if (remaining > 3)
      {
        // Обычный байт середины посылки: ACK стабилен (=1) на всём
        // этом участке, гонок по времени нет вне зависимости от
        // задержки входа в прерывание.
        rx_buf[rx_idx++] = i2c_data_receive(I2C1);
      }
      else
      {
        // remaining == 3: байт N-2 уже готов, но НЕ читаем его здесь -
        // оставляем в DT непрочитанным, это специально стопорит SCL
        // (см. TDC), и мы ловим этот момент в PH_RX_TAIL1.
        phase = PH_RX_TAIL1;
      }
    }
    break;

  /////////////////////////////////////////////////////////////////////
  case PH_RX_TAIL1:
    if (sts1 & I2C_TDC_FLAG)
    {
      // DT = байт N-2 (не прочитан), сдвиговый регистр = байт N-1
      // (уже полностью принят и подтверждён ACK-ом, т.к. ACK ещё =1).
      // Взводим ACK=0 - он подействует на байт N, который начнёт
      // клокаться сразу после освобождения DT.
      i2c_ack_enable(I2C1, FALSE);
      rx_buf[rx_idx++] = i2c_data_receive(I2C1); // N-2
      phase = PH_RX_TAIL2;
    }
    break;

  /////////////////////////////////////////////////////////////////////
  case PH_RX_TAIL2:
    if (sts1 & I2C_TDC_FLAG)
    {
      // DT = байт N-1 (не прочитан), сдвиговый регистр = байт N
      // (уже принят, уже отправлен NACK, т.к. ACK был снят заранее).
      i2c_stop_generate(I2C1);
      rx_buf[rx_idx++] = i2c_data_receive(I2C1); // N-1
      rx_buf[rx_idx++] = i2c_data_receive(I2C1); // N (переносится автоматически)
      status = HW_I2C_OK;
      phase = PH_WAIT_STOP_CLEAR;
    }
    break;

  /////////////////////////////////////////////////////////////////////
  default:
    break;
  }
}

/******************************************************************************/

void I2C1_ERR_IRQHandler(void)
{
  uint32_t sts1 = I2C1->sts1;

  if (sts1 & I2C_ACKFAIL_FLAG)
  {
    i2c_flag_clear(I2C1, I2C_ACKFAIL_FLAG);

    hw_i2c_status_t err = (phase == PH_WAIT_ADDR_W || phase == PH_WAIT_ADDR_R)
                               ? HW_I2C_ERR_NACK_ADDR
                               : HW_I2C_ERR_NACK_DATA;

    abort_transaction(err);
  }

  if (sts1 & (I2C_BUSERR_FLAG | I2C_ARLOST_FLAG | I2C_OUF_FLAG))
  {
    i2c_flag_clear(I2C1, I2C_BUSERR_FLAG | I2C_ARLOST_FLAG | I2C_OUF_FLAG);
    abort_transaction(HW_I2C_ERR_BUS);
  }
}