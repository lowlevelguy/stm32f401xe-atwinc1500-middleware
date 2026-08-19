#include <string.h>

#include "unity.h"
#include "mock_mw_port.h"
#include "mock_stm32_hal.h"
#include "mock_conf_winc.h"

#include "nm_bsp.h"
#include "nm_common.h"


#define UNUSED(x)			((void)x)
#define CAPTURES_MAX_COUNT	8


/* Captures types ------------------------------------------------------------*/
typedef struct {
	GPIO_TypeDef* port;
	uint32_t pin, mode;
} gpio_init_capture_t;

typedef struct {
	GPIO_TypeDef* port;
	uint16_t pin;
	/* Explicitly defined padding is forced to 0 on partial initialisation in
	 * C11, avoiding UB. */
	char padding[6];
} gpio_deinit_capture_t;


/* External variables --------------------------------------------------------*/
extern volatile tpfNmBspIsr module_irqn_pin_isr;


/* Private variables ---------------------------------------------------------*/
static SysTick_Type systick_obj = {0};

static gpio_init_capture_t captured_gpio_inits[CAPTURES_MAX_COUNT] = {0};
static gpio_deinit_capture_t captured_gpio_deinits[CAPTURES_MAX_COUNT] = {0};
static uint32_t captured_sleep_ms[CAPTURES_MAX_COUNT] = {0};


/* Public variables ----------------------------------------------------------*/
SysTick_Type* SysTick = &systick_obj;


/* External functions --------------------------------------------------------*/
extern void module_ctrl_pins_init(void);
extern void irqn_on_irq(uint16_t pin);


/* Test helpers --------------------------------------------------------------*/
/**
 * @brief Check for an aligned occurrence of a byte sequence in a byte array.
 *
 * @param query pointer to the byte sequence to search for
 * @param search_array byte array to search within
 * @param word_size size of the byte sequence
 * @param word_count size of search_array in units of word_size bytes
 *
 * @return 1 if the byte sequence occurs at least once, 0 otherwise.
 */
static uint8_t helper_memsearch(const void* query, void* search_array,
	uint8_t word_size, uint8_t word_count) {
	for (uint8_t i = 0; i < word_count; i++) {
		if (memcmp(query, search_array + i * word_size, word_size) == 0) {
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Captures the GPIO init configuration passed to HAL_GPIO_Init.
 *
 * @param port GPIO port
 * @param gpio_init pointer to the init config struct
 * @param cmock_num_calls  CMock call counter
 */
static void helper_capture_gpio_init(GPIO_TypeDef* port,
	GPIO_InitTypeDef* gpio_init, int cmock_num_calls) {
	// Reset capture array
	if (cmock_num_calls == 0) {
		memset(captured_gpio_inits, 0, sizeof(captured_gpio_inits));
	}

	if (cmock_num_calls < CAPTURES_MAX_COUNT && cmock_num_calls >= 0) {
		captured_gpio_inits[cmock_num_calls].port = port;
		captured_gpio_inits[cmock_num_calls].pin = gpio_init->Pin;
		captured_gpio_inits[cmock_num_calls].mode = gpio_init->Mode;
	}
}

/**
 * @brief Captures the GPIO init configuration passed to HAL_GPIO_Init.
 *
 * @param port GPIO port
 * @param pin GPIO pin
 * @param cmock_num_calls  CMock call counter
 */
static void helper_capture_gpio_deinit(GPIO_TypeDef* port, uint16_t pin,
	int cmock_num_calls) {
	// Reset capture array
	if (cmock_num_calls == 0) {
		memset(captured_gpio_deinits, 0, sizeof(captured_gpio_deinits));
	}

	if (cmock_num_calls < CAPTURES_MAX_COUNT && cmock_num_calls >= 0) {
		captured_gpio_deinits[cmock_num_calls].port = port;
		captured_gpio_deinits[cmock_num_calls].pin = pin;
	}
}

/**
 * @brief Captures the sleep duration in milliseconds passed to HAL_Delay.
 *
 * @param ms sleep duration in milliseconds
 * @param cmock_num_calls CMock call counter
 */
static void helper_capture_sleep_duration_ms(uint32_t ms, int cmock_num_calls) {
	// Reset capture array
	if (cmock_num_calls == 0) {
		memset(captured_sleep_ms, 0, sizeof(captured_sleep_ms));
	}

	if (cmock_num_calls < 2 && cmock_num_calls >= 0) {
		captured_sleep_ms[cmock_num_calls] = ms;
	}
}

/**
 * @brief No-op function acting as stub ISR for the IRQN signal.
 */
static void helper_irqn_stub_isr() {}


/* Unit tests ----------------------------------------------------------------*/
/**
 * @brief Tests that module_ctrl_pins configures GPIO pins for CHIP_EN, RESET_N
 * and IRQN signals, de-registers any driver ISR callbacks, enables IRQN signal
 * interrupt and powers off the module.
 *
 * @note Covers REQ-FUN-01..04.
 *
 * @note The value of the internal ISR callback module_irqn_pin_isr is not
 * asserted to be NULL, to allow flexibility to the middleware as to what the
 * "default ISR" should be.
 */
static void test_nm_bsp_init_configures_gpio_pins_deregisters_isr_enables_irqn_irq_and_powers_off_module_and_returns_M2M_SUCCESS(void) {
	/* System clock initialised when control flags are unset.
	 * We don't test the case when the flags are set separately, since it's
	 * inconsequential whether system clock init is then skipped or still
	 * done. */
	SysTick->CTRL = 0;
	mw_port_system_clock_config_Expect();

	/* Allow the GPIO configurations to occur in any order by re-routing _Expect
	 * to a custom callback */
	HAL_GPIO_Init_StubWithCallback(helper_capture_gpio_init);

	// EXTI ISR de-registered before interrupt enabling
	mw_port_exti_deregister_isr_Expect();

	// EXTI interrupt enabled in NVIC, with arbitrary priority and sub-priority
	HAL_NVIC_SetPriority_Expect(CONF_WINC_IRQN_EXTI_LINE, 0, 0);
	HAL_NVIC_SetPriority_IgnoreArg_PreemptPriority();
	HAL_NVIC_SetPriority_IgnoreArg_SubPriority();
	HAL_NVIC_EnableIRQ_Expect(CONF_WINC_IRQN_EXTI_LINE);

	// Module off: RESET_N and CHIP_EN both low
	HAL_GPIO_WritePin_Expect(CONF_WINC_RESET_N_PORT, CONF_WINC_RESET_N_PIN,
		GPIO_PIN_RESET);
	HAL_GPIO_WritePin_Expect(CONF_WINC_CHIP_EN_PORT, CONF_WINC_CHIP_EN_PIN,
		GPIO_PIN_RESET);

	// Assert return value
	TEST_ASSERT_EQUAL_UINT8(M2M_SUCCESS, nm_bsp_init());

	// Assert at least 3 HAL_GPIO_Init calls
	TEST_ASSERT_GREATER_OR_EQUAL_UINT32(3, HAL_GPIO_Init_CallCount());

	// Assert GPIO configurations
	gpio_init_capture_t chip_en_conf = {
		.port = CONF_WINC_CHIP_EN_PORT,
		.pin = CONF_WINC_CHIP_EN_PIN,
		.mode = GPIO_MODE_OUTPUT_PP
	}, reset_n_conf = {
		.port = CONF_WINC_RESET_N_PORT,
		.pin = CONF_WINC_RESET_N_PIN,
		.mode = GPIO_MODE_OUTPUT_PP
	}, irqn_conf = {
		.port = CONF_WINC_IRQN_PORT,
		.pin = CONF_WINC_IRQN_PIN,
		.mode = GPIO_MODE_IT_FALLING
	};

	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(
		&chip_en_conf, captured_gpio_inits,
		sizeof(gpio_init_capture_t), CAPTURES_MAX_COUNT));
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(
		&reset_n_conf, captured_gpio_inits,
		sizeof(gpio_init_capture_t), CAPTURES_MAX_COUNT));
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(
		&irqn_conf, captured_gpio_inits,
		sizeof(gpio_init_capture_t), CAPTURES_MAX_COUNT));

#if (CONF_WINC_USE_WAKE_PIN == 1)
	gpio_init_capture_t wake_conf = {
		.port = CONF_WINC_WAKE_PORT,
		.pin = CONF_WINC_WAKE_PIN,
		.mode = GPIO_MODE_OUTPUT_PP
	};

	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(
		&wake_conf, captured_gpio_inits,
		sizeof(gpio_init_capture_t), CAPTURES_MAX_COUNT));
#endif
}

/**
 * @brief Tests that nm_bsp_deinit disables the IRQN signal interrupt, de-inits
 * the module control GPIO pins and returns M2M_SUCCESS.
 *
 * @note Covers REQ-FUN-05,06.
 */
static void test_nm_bsp_deinit_disables_irqn_interrupt_deinitialises_gpio_pins_and_returns_M2M_SUCCESS(void) {
	/* Whether the ISR is de-registered or not is unimportant; subsequent
	 * nm_bsp_init calls already ensure clean-up. */
	mw_port_exti_deregister_isr_Ignore();

	// IRQN interrupt disabled
	HAL_NVIC_DisableIRQ_Expect(CONF_WINC_IRQN_EXTI_LINE);

	// Capture deinit calls
	HAL_GPIO_DeInit_StubWithCallback(helper_capture_gpio_deinit);

	// Assert return value
	TEST_ASSERT_EQUAL_UINT8(M2M_SUCCESS, nm_bsp_deinit());

	// Assert GPIO de-inits
	gpio_deinit_capture_t chip_en_deinit = {
		.port = CONF_WINC_CHIP_EN_PORT, .pin = CONF_WINC_CHIP_EN_PIN
	}, reset_n_deinit = {
		.port = CONF_WINC_RESET_N_PORT, .pin = CONF_WINC_RESET_N_PIN
	}, irqn_deinit = {
		.port = CONF_WINC_IRQN_PORT, .pin = CONF_WINC_IRQN_PIN
	};

	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(
		&chip_en_deinit, captured_gpio_deinits,
		sizeof(gpio_deinit_capture_t), CAPTURES_MAX_COUNT));
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(
		&reset_n_deinit, captured_gpio_deinits,
		sizeof(gpio_deinit_capture_t), CAPTURES_MAX_COUNT));
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(
		&irqn_deinit, captured_gpio_deinits,
		sizeof(gpio_deinit_capture_t), CAPTURES_MAX_COUNT));

#if (CONF_WINC_USE_WAKE_PIN == 1)
	gpio_deinit_capture_t wake_deinit = {
		.port = CONF_WINC_WAKE_PORT, .pin = CONF_WINC_WAKE_PIN
	};
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(
		&wake_deinit, captured_gpio_deinits,
		sizeof(gpio_deinit_capture_t), CAPTURES_MAX_COUNT));
#endif
}

/**
 * @brief Tests that nm_bsp_reset executes the module reset sequence correctly:
 * RESET_N,CHIP_EN low --(>= 2us)--> CHIP_EN high --(>= 10ms)--> RESET_N high.
 *
 * @note Covers REQ-FUN-07,08.
 */
static void test_nm_bsp_reset_executes_reset_sequence(void) {
	/**************************************************************************/
	/*                        Call correctness testing                        */
	/**************************************************************************/

	// RESET_N,CHIP_EN low (in order, for extra safety)
	HAL_GPIO_WritePin_Expect(CONF_WINC_RESET_N_PORT, CONF_WINC_RESET_N_PIN,
		GPIO_PIN_RESET);
	HAL_GPIO_WritePin_Expect(CONF_WINC_CHIP_EN_PORT, CONF_WINC_CHIP_EN_PIN,
		GPIO_PIN_RESET);

	// Capture sleep durations
	HAL_Delay_StubWithCallback(helper_capture_sleep_duration_ms);

	// CHIP_EN high
	HAL_GPIO_WritePin_Expect(CONF_WINC_CHIP_EN_PORT, CONF_WINC_CHIP_EN_PIN,
		GPIO_PIN_SET);

	// RESET_N high
	HAL_GPIO_WritePin_Expect(CONF_WINC_RESET_N_PORT, CONF_WINC_RESET_N_PIN,
		GPIO_PIN_SET);

	nm_bsp_reset();

	/* Assert sleep durations.
	 * First sleep: >= 1ms (since we can't sleep for microseconds).
	 * Second sleep: >= 10ms. */
	TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1, captured_sleep_ms[0]);
	TEST_ASSERT_GREATER_OR_EQUAL_UINT32(10, captured_sleep_ms[1]);


	/**************************************************************************/
	/*                         Call ordering testing                          */
	/**************************************************************************/

	// Disable HAL_Delay stub
	HAL_Delay_StubWithCallback(NULL);

	// RESET_N,CHIP_EN low (in order, for extra safety)
	HAL_GPIO_WritePin_Expect(CONF_WINC_RESET_N_PORT, CONF_WINC_RESET_N_PIN,
		GPIO_PIN_RESET);
	HAL_GPIO_WritePin_Expect(CONF_WINC_CHIP_EN_PORT, CONF_WINC_CHIP_EN_PIN,
		GPIO_PIN_RESET);

	// First sleep
	/* Note: this assumes nm_bsp_reset to be a pure function, one which sleeps
	 * for equal amounts of milliseconds at each instance across all calls. */
	HAL_Delay_Expect(captured_sleep_ms[0]);

	// CHIP_EN high
	HAL_GPIO_WritePin_Expect(CONF_WINC_CHIP_EN_PORT, CONF_WINC_CHIP_EN_PIN,
		GPIO_PIN_SET);

	// Second sleep
	HAL_Delay_Expect(captured_sleep_ms[1]);

	// RESET_N high
	HAL_GPIO_WritePin_Expect(CONF_WINC_RESET_N_PORT, CONF_WINC_RESET_N_PIN,
		GPIO_PIN_SET);

	nm_bsp_reset();
}

/**
 * @brief Tests that nm_bsp_sleep sleeps for the exact specified number of
 * milliseconds by forwarding its parameter as-is to HAL_Delay.
 *
 * @note Covers REQ-FUN-09.
 */
static void test_nm_bsp_sleep_forwards_to_HAL_Delay(void) {
	const uint32_t ms = 0xAAFF;

	HAL_Delay_Expect(ms);
	nm_bsp_sleep(ms);
}

/**
 * @brief Tests that nm_bsp_register_isr updates its internal ISR callback with
 * the one provided as input, and registers its ISR via the porting interface.
 *
 * @note Covers REQ-FUN-10.
 */
static void test_nm_bsp_register_isr_updates_internal_isr_callback_and_registers_isr(void) {
	// Porting interface ISR register call
	mw_port_exti_register_isr_Expect(irqn_on_irq);

	nm_bsp_register_isr(helper_irqn_stub_isr);

	// Assert internal ISR callback value
	TEST_ASSERT_EQUAL_PTR(module_irqn_pin_isr, &helper_irqn_stub_isr);
}

/**
 * @brief Tests that nm_bsp_interrupt_ctrl enables the EXTI IRQ in NVIC when
 * called with a parameter of 1, and disables it when called with a parameter
 * of 0.
 *
 * @note Covers REQ-FUN-11.
 */
static void test_nm_bsp_interrupt_ctrl_enables_and_disables_irqn_irq(void) {
	// On state = 1, enables IRQN interrupt in NVIC
	HAL_NVIC_SetPriority_Expect(CONF_WINC_IRQN_EXTI_LINE, 0, 0);
	HAL_NVIC_SetPriority_IgnoreArg_PreemptPriority();
	HAL_NVIC_SetPriority_IgnoreArg_SubPriority();
	HAL_NVIC_EnableIRQ_Expect(CONF_WINC_IRQN_EXTI_LINE);

	nm_bsp_interrupt_ctrl(1);

	// On state = 0, disables IRQ in NVIC
	HAL_NVIC_DisableIRQ_Expect(CONF_WINC_IRQN_EXTI_LINE);

	nm_bsp_interrupt_ctrl(0);
}

/* Unity harness -------------------------------------------------------------*/
void setUp(void) {
	mock_conf_winc_Init();
	mock_mw_port_Init();
	mock_stm32_hal_Init();
}

void tearDown(void) {
	mock_conf_winc_Verify();
	mock_conf_winc_Destroy();

	mock_mw_port_Verify();
	mock_mw_port_Destroy();

	mock_stm32_hal_Verify();
	mock_stm32_hal_Destroy();
}

int main(void) {
	UNITY_BEGIN();

	RUN_TEST(test_nm_bsp_init_configures_gpio_pins_deregisters_isr_enables_irqn_irq_and_powers_off_module_and_returns_M2M_SUCCESS);
	RUN_TEST(test_nm_bsp_deinit_disables_irqn_interrupt_deinitialises_gpio_pins_and_returns_M2M_SUCCESS);
	RUN_TEST(test_nm_bsp_reset_executes_reset_sequence);
	RUN_TEST(test_nm_bsp_sleep_forwards_to_HAL_Delay);
	RUN_TEST(test_nm_bsp_register_isr_updates_internal_isr_callback_and_registers_isr);
	RUN_TEST(test_nm_bsp_interrupt_ctrl_enables_and_disables_irqn_irq);

	return UNITY_END();
}
