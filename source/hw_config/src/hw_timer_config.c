#include "hw_timer_config.h"
#include "at32f413.h"

/******************************************************************************/
static void wk_tmr2_init(void)
{
  gpio_init_type gpio_init_struct;
  tmr_input_config_type tmr_input_struct;

  crm_periph_reset(CRM_TMR2_PERIPH_RESET, TRUE);
  crm_periph_reset(CRM_TMR2_PERIPH_RESET, FALSE);

  gpio_default_para_init(&gpio_init_struct);

  /* configure the CH2 CH3 pin
    CH2 (PA1) - CRANK (датчик коленвала)
    CH3 (PA2) - CAM (датчик распредвала)
  */
  gpio_init_struct.gpio_pins = GPIO_PINS_1 | GPIO_PINS_2; // CH2->PA1; CH3->PA2
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_DOWN;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init(GPIOA, &gpio_init_struct);

  /* configure plus mode */
  tmr_32_bit_function_enable(TMR2, TRUE);

  /* configure counter settings */
  tmr_base_init(TMR2, 0xFFFFFFFF, 199);
  tmr_cnt_dir_set(TMR2, TMR_COUNT_UP);

  /* configure primary mode settings */
  // tmr_sub_sync_mode_set(TMR2, FALSE);
  // tmr_primary_mode_select(TMR2, TMR_PRIMARY_SEL_RESET);

  /* configure channel 2 input settings */
  tmr_input_struct.input_channel_select = TMR_SELECT_CHANNEL_2;
  tmr_input_struct.input_mapped_select = TMR_CC_CHANNEL_MAPPED_DIRECT;
  tmr_input_struct.input_polarity_select = TMR_INPUT_FALLING_EDGE;
  tmr_input_struct.input_filter_value = 15;
  tmr_input_channel_init(TMR2, &tmr_input_struct, TMR_CHANNEL_INPUT_DIV_1);

  /* configure channel 3 input settings */
  tmr_input_struct.input_channel_select = TMR_SELECT_CHANNEL_3;
  tmr_input_struct.input_mapped_select = TMR_CC_CHANNEL_MAPPED_DIRECT;
  tmr_input_struct.input_polarity_select = TMR_INPUT_RISING_EDGE;
  tmr_input_struct.input_filter_value = 15;
  tmr_input_channel_init(TMR2, &tmr_input_struct, TMR_CHANNEL_INPUT_DIV_1);

  /*
    tmr_interrupt_enable(TMR2, TMR_C2_INT, ...) и TMR_C3_INT сюда больше
    не входят: разным плагинам зажигания нужны разные каналы (например,
    ignition_cam6p1 использует только один из двух), поэтому включение и
    выключение прерываний по конкретным каналам перенесено в init()/deinit()
    каждого плагина. Здесь только "железная" конфигурация GPIO и каналов
    захвата, общая для всех плагинов.
  */
}

/*
  Инициализация таймера формирования импульсов зажигания.

  ВНИМАНИЕ: Нетривиальная последовательность инициализации — не копировать вслепую из WorkBench!

  Проблема: при стандартной инициализации (сначала GPIO, потом таймер) на выходе GPIO
  возникает глитч длительностью ~300 нс сразу после инициализации таймера. Это недопустимо,
  так как может привести к срабатыванию высоковольтной части системы зажигания и выходу
  её из строя. Опасны как слишком короткие импульсы (< 500 нс), так и зависание GPIO в лог. 1.

  Решение: GPIO намеренно инициализируется ПОСЛЕ таймера. Пока GPIO не переведён в режим
  альтернативной функции, сигнал таймера физически не подключён к ножке, и все переходные
  процессы внутри таймера завершаются безопасно. Цикл nop перед инициализацией GPIO
  гарантирует завершение этих переходных процессов до подключения ножки.
*/
static void wk_tmr5_init(void)
{
  gpio_init_type gpio_init_struct;
  tmr_output_config_type tmr_output_struct;

  crm_periph_reset(CRM_TMR5_PERIPH_RESET, TRUE);
  crm_periph_reset(CRM_TMR5_PERIPH_RESET, FALSE);

  /* configure plus mode */
  tmr_32_bit_function_enable(TMR5, TRUE);

  /* configure counter settings */
  tmr_cnt_dir_set(TMR5, TMR_COUNT_UP);
  tmr_clock_source_div_set(TMR5, TMR_CLOCK_DIV1);
  tmr_period_buffer_enable(TMR5, FALSE);
  tmr_base_init(TMR5, 200, 199);

  /* configure primary mode settings */
  tmr_sub_sync_mode_set(TMR5, FALSE);
  tmr_primary_mode_select(TMR5, TMR_PRIMARY_SEL_C1ORAW);

  /* configure one cycle mode */
  tmr_one_cycle_mode_enable(TMR5, TRUE);

  /* configure channel 3 output settings */
  tmr_output_struct.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_B;
  tmr_output_struct.oc_output_state = TRUE;
  tmr_output_struct.occ_output_state = FALSE;
  tmr_output_struct.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.occ_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.oc_idle_state = FALSE;
  tmr_output_struct.occ_idle_state = FALSE;
  tmr_output_channel_config(TMR5, TMR_SELECT_CHANNEL_1, &tmr_output_struct);
  tmr_channel_value_set(TMR5, TMR_SELECT_CHANNEL_1, 100);
  tmr_output_channel_buffer_enable(TMR5, TMR_SELECT_CHANNEL_1, FALSE);

  tmr_output_channel_immediately_set(TMR5, TMR_SELECT_CHANNEL_1, TRUE);

  // Пауза для завершения переходных процессов таймера
  for (int i = 0; i < 100; i++)
    asm("nop");

  gpio_default_para_init(&gpio_init_struct);

  /* configure the CH1 pin */
  gpio_init_struct.gpio_pins = GPIO_PINS_0;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init(GPIOA, &gpio_init_struct);
}

/*
  Таймер семплирования данных АЦП.
  Запускается от TMR5
*/
static void wk_tmr3_init(void)
{
  /* configure counter settings */
  tmr_cnt_dir_set(TMR3, TMR_COUNT_UP);
  tmr_clock_source_div_set(TMR3, TMR_CLOCK_DIV1);
  tmr_period_buffer_enable(TMR3, FALSE);
  tmr_base_init(TMR3, 199, 0);

  /* configure primary mode settings */
  tmr_sub_sync_mode_set(TMR3, FALSE);
  tmr_primary_mode_select(TMR3, TMR_PRIMARY_SEL_OVERFLOW);

  /* configure sub-mode */
  tmr_sub_mode_select(TMR3, TMR_SUB_TRIGGER_MODE);

  /* configure sub-mode input */
  tmr_trigger_input_select(TMR3, TMR_SUB_INPUT_SEL_IS2);
}

static void wk_tmr4_init(void)
{
  gpio_init_type gpio_init_struct;
  tmr_output_config_type tmr_output_struct;

  gpio_default_para_init(&gpio_init_struct);

  /* configure the CH1 pin */
  gpio_init_struct.gpio_pins = GPIO_PINS_6;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init(GPIOB, &gpio_init_struct);

  /* configure the CH2 pin */
  gpio_init_struct.gpio_pins = GPIO_PINS_7;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init(GPIOB, &gpio_init_struct);

  /* configure the CH4 pin */
  gpio_init_struct.gpio_pins = GPIO_PINS_9;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init(GPIOB, &gpio_init_struct);

  /* configure counter settings */
  tmr_cnt_dir_set(TMR4, TMR_COUNT_UP);
  tmr_clock_source_div_set(TMR4, TMR_CLOCK_DIV1);
  tmr_period_buffer_enable(TMR4, FALSE);
  tmr_base_init(TMR4, 1023, 0);

  /* configure primary mode settings */
  tmr_sub_sync_mode_set(TMR4, FALSE);
  tmr_primary_mode_select(TMR4, TMR_PRIMARY_SEL_RESET);

  /* configure channel 1 output settings */
  tmr_output_struct.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
  tmr_output_struct.oc_output_state = TRUE;
  tmr_output_struct.occ_output_state = FALSE;
  tmr_output_struct.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.occ_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.oc_idle_state = FALSE;
  tmr_output_struct.occ_idle_state = FALSE;
  tmr_output_channel_config(TMR4, TMR_SELECT_CHANNEL_1, &tmr_output_struct);
  tmr_channel_value_set(TMR4, TMR_SELECT_CHANNEL_1, 0);
  tmr_output_channel_buffer_enable(TMR4, TMR_SELECT_CHANNEL_1, FALSE);

  tmr_output_channel_immediately_set(TMR4, TMR_SELECT_CHANNEL_1, FALSE);

  /* configure channel 2 output settings */
  tmr_output_struct.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
  tmr_output_struct.oc_output_state = TRUE;
  tmr_output_struct.occ_output_state = FALSE;
  tmr_output_struct.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.occ_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.oc_idle_state = FALSE;
  tmr_output_struct.occ_idle_state = FALSE;
  tmr_output_channel_config(TMR4, TMR_SELECT_CHANNEL_2, &tmr_output_struct);
  tmr_channel_value_set(TMR4, TMR_SELECT_CHANNEL_2, 0);
  tmr_output_channel_buffer_enable(TMR4, TMR_SELECT_CHANNEL_2, FALSE);

  tmr_output_channel_immediately_set(TMR4, TMR_SELECT_CHANNEL_2, FALSE);

  /* configure channel 4 output settings */
  tmr_output_struct.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
  tmr_output_struct.oc_output_state = TRUE;
  tmr_output_struct.occ_output_state = FALSE;
  tmr_output_struct.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.occ_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.oc_idle_state = FALSE;
  tmr_output_struct.occ_idle_state = FALSE;
  tmr_output_channel_config(TMR4, TMR_SELECT_CHANNEL_4, &tmr_output_struct);
  tmr_channel_value_set(TMR4, TMR_SELECT_CHANNEL_4, 0);
  tmr_output_channel_buffer_enable(TMR4, TMR_SELECT_CHANNEL_4, FALSE);

  tmr_output_channel_immediately_set(TMR4, TMR_SELECT_CHANNEL_4, FALSE);

  tmr_counter_enable(TMR4, TRUE);
}

static void wk_tmr10_init(void)
{
  gpio_init_type gpio_init_struct;
  tmr_output_config_type tmr_output_struct;
  gpio_default_para_init(&gpio_init_struct);

  /* configure the CH1 pin */
  gpio_init_struct.gpio_pins = GPIO_PINS_8;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init(GPIOB, &gpio_init_struct);

  /* configure counter settings */
  tmr_cnt_dir_set(TMR10, TMR_COUNT_UP);
  tmr_clock_source_div_set(TMR10, TMR_CLOCK_DIV1);
  tmr_period_buffer_enable(TMR10, FALSE);
  tmr_base_init(TMR10, 8191, 0);

  /* configure channel 1 output settings */
  tmr_output_struct.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
  tmr_output_struct.oc_output_state = TRUE;
  tmr_output_struct.occ_output_state = FALSE;
  tmr_output_struct.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.occ_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_output_struct.oc_idle_state = FALSE;
  tmr_output_struct.occ_idle_state = FALSE;
  tmr_output_channel_config(TMR10, TMR_SELECT_CHANNEL_1, &tmr_output_struct);
  tmr_channel_value_set(TMR10, TMR_SELECT_CHANNEL_1, 0);
  tmr_output_channel_buffer_enable(TMR10, TMR_SELECT_CHANNEL_1, FALSE);

  tmr_output_channel_immediately_set(TMR10, TMR_SELECT_CHANNEL_1, FALSE);

  tmr_counter_enable(TMR10, TRUE);
}

/******************************************************************************/

void hw_timer_config(void)
{
  wk_tmr2_init();
  wk_tmr5_init();

  wk_tmr3_init();

  wk_tmr4_init();
  wk_tmr10_init();
}
