#include <string.h>

#include "unity.h"
#include "mock_stm32_hal.h"
#include "mock_nm_bsp_api.h"
#include "mock_mw_port.h"
#include "mock_conf_winc.h"
#include "nm_bus_wrapper.h"


#define UNUSED(x)			((void)(x))
#define CAPTURES_MAX_COUNT	8


/* Types ---------------------------------------------------------------------*/
typedef DMA_InitTypeDef dma_init_capture_t;
typedef IRQn_Type nvic_capture_t;

typedef struct {
	SPI_TypeDef* Instance;
	uint32_t Mode,
		Direction,
		DataSize,
		CLKPolarity,
		CLKPhase,
		NSS,
		BaudRatePrescaler,
		FirstBit,
		TIMode,
		CRCCalculation;
} spi_init_capture_t;

typedef struct {
	GPIO_TypeDef* port;
	GPIO_InitTypeDef init;
} gpio_init_capture_t;


/* Private variables ---------------------------------------------------------*/
dma_init_capture_t dma_init_captures[CAPTURES_MAX_COUNT] = {0};
spi_init_capture_t spi_init_captures[CAPTURES_MAX_COUNT] = {0};
gpio_init_capture_t gpio_init_captures[CAPTURES_MAX_COUNT] = {0};
IRQn_Type nvic_priority_captures[CAPTURES_MAX_COUNT] = {0};
IRQn_Type nvic_enable_captures[CAPTURES_MAX_COUNT] = {0};


/* External functions --------------------------------------------------------*/
extern void spi_on_irq_success(SPI_HandleTypeDef*);
extern void spi_on_irq_error(SPI_HandleTypeDef*);


/* Helper functions ----------------------------------------------------------*/
static void helper_expect_ss_assert(void) {
	HAL_GPIO_WritePin_Expect(CONF_WINC_SPI_SS_PORT, CONF_WINC_SPI_SS_PIN,
		GPIO_PIN_RESET);
}

static void helper_expect_ss_deassert(void) {
	HAL_GPIO_WritePin_Expect(CONF_WINC_SPI_SS_PORT, CONF_WINC_SPI_SS_PIN,
		GPIO_PIN_SET);
}

/**
 * @brief Sets both DMA stream configurations to the module-required
 * parameters.
 */
static void helper_set_module_required_dma_configs(void) {
	hdma_tx.Init.Mode = DMA_NORMAL;
	hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;

	hdma_rx.Init.Mode = DMA_NORMAL;
	hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
}

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
 * @brief Find the first aligned occurrence of a byte sequence in a byte array.
 *
 * @param query pointer to the byte sequence to search for
 * @param search_array byte array to search within
 * @param word_size size of the byte sequence
 * @param word_count size of search_array in units of word_size bytes
 *
 * @return the offset of the first occurence of query in search_array in units
 * of word_size bytes if it exists, -1 otherwise.
 */
static int8_t helper_memfind(const void* query, void* search_array,
	uint8_t word_size, uint8_t word_count) {
	for (uint8_t i = 0; i < word_count; i++) {
		if (memcmp(query, search_array + i * word_size, word_size) == 0) {
			return (int8_t)i;
		}
	}

	return -1;
}

/**
 * @brief Captures the DMA init configuration passed to HAL_DMA_Init.
 *
 * @param hdma pointer to DMA handle
 * @param cmock_call_count CMock call counter
 *
 * @return HAL_OK
 */
static HAL_StatusTypeDef helper_capture_dma_init(DMA_HandleTypeDef* hdma,
										  int cmock_call_count) {
	// Reset capture array
	if (cmock_call_count == 0) {
		memset(dma_init_captures, 0, sizeof(dma_init_captures));
	}

	if (cmock_call_count >= 0 && cmock_call_count < CAPTURES_MAX_COUNT) {
		dma_init_captures[cmock_call_count] = hdma->Init;
	}
	return HAL_OK;
}

/**
 * @brief Captures the IRQ numbers passed to HAL_NVIC_SetPriority.
 *
 * @param irqn IRQ number
 * @param preempt_priority pre-empt priority (ignored)
 * @param sub_priority sub priority (ignored)
 * @param cmock_call_count CMock call counter
 */
static void helper_capture_nvic_priority(IRQn_Type irqn,
	uint32_t preempt_priority, uint32_t sub_priority, int cmock_call_count) {
	UNUSED(preempt_priority);
	UNUSED(sub_priority);

	// Reset capture array
	if (cmock_call_count == 0) {
		memset(nvic_priority_captures, 0, sizeof(nvic_priority_captures));
	}

	if (cmock_call_count >= 0 && cmock_call_count < CAPTURES_MAX_COUNT) {
		// Capture with a bias of +1 to reserve a 0 value for "uninitialised"
		nvic_priority_captures[cmock_call_count] = irqn + 1;
	}
}

/**
 * @brief Captures the IRQ numbers passed to HAL_NVIC_EnableIRQ.
 *
 * @param irqn IRQ number
 * @param cmock_call_count CMock call counter
 */
static void helper_capture_nvic_enable(IRQn_Type irqn, int cmock_call_count) {
	// Reset capture array
	if (cmock_call_count == 0) {
		memset(nvic_enable_captures, 0, sizeof(nvic_enable_captures));
	}

	if (cmock_call_count >= 0 && cmock_call_count < CAPTURES_MAX_COUNT) {
		// Capture with a bias of +1 to reserve a 0 value for "uninitialised"
		nvic_enable_captures[cmock_call_count] = irqn + 1;
	}
}

HAL_StatusTypeDef helper_capture_spi_init(SPI_HandleTypeDef* hspi,
	int cmock_num_calls) {
	// Reset capture array
	if (cmock_num_calls == 0) {
		memset(spi_init_captures, 0, sizeof(spi_init_captures));
	}

	if (cmock_num_calls >= 0 && cmock_num_calls < CAPTURES_MAX_COUNT) {
		spi_init_captures[cmock_num_calls].Instance = hspi->Instance;
		spi_init_captures[cmock_num_calls].Mode = hspi->Init.Mode;
		spi_init_captures[cmock_num_calls].Direction = hspi->Init.Direction;
		spi_init_captures[cmock_num_calls].DataSize = hspi->Init.DataSize;
		spi_init_captures[cmock_num_calls].CLKPolarity = hspi->Init.CLKPolarity;
		spi_init_captures[cmock_num_calls].CLKPhase = hspi->Init.CLKPhase;
		spi_init_captures[cmock_num_calls].NSS = hspi->Init.NSS;
		spi_init_captures[cmock_num_calls].BaudRatePrescaler = hspi->Init.BaudRatePrescaler;
		spi_init_captures[cmock_num_calls].FirstBit = hspi->Init.FirstBit;
		spi_init_captures[cmock_num_calls].TIMode = hspi->Init.TIMode;
		spi_init_captures[cmock_num_calls].CRCCalculation = hspi->Init.CRCCalculation;
	}

	return HAL_OK;
}

void helper_capture_gpio_init(GPIO_TypeDef* gpio_port,
	GPIO_InitTypeDef* gpio_init, int cmock_num_calls) {
	// Reset capture array
	if (cmock_num_calls == 0) {
		memset(gpio_init_captures, 0, sizeof(gpio_init_captures));
	}

	if (cmock_num_calls >= 0 && cmock_num_calls < CAPTURES_MAX_COUNT) {
		gpio_init_captures[cmock_num_calls].port = gpio_port;
		gpio_init_captures[cmock_num_calls].init = *gpio_init;
	}
}


/* Unit tests ----------------------------------------------------------------*/
/**
 * @brief Tests that the configured SPI transfer capacity is greater than 16
 * bytes.
 *
 * @note Covers REQ-PERF-02.
 */
void test_spi_transfer_capacity_greater_than_16_bytes(void) {
	TEST_ASSERT_GREATER_OR_EQUAL(16, egstrNmBusCapabilities.u16MaxTrxSz);
}

/**
 * @brief Tests that nm_bus_init, when the DMA interfaces for SPI TX and SPI RX
 * are misconfigured by the user-specified init functions, enforces the module-
 * required parameters.
 *
 * @note Covers REQ-FUN-13,14,15 and REQ-FUN-19 partially.
 */
void test_nm_bus_init_on_dma_misconfigured_overwrites_config(void) {
	// Ignores: SPI init, SPI ISR registration, SS pin config, module reset
	HAL_SPI_Init_IgnoreAndReturn(HAL_OK);
	mw_port_spi_register_tx_isr_Ignore();
	mw_port_spi_register_rx_isr_Ignore();
	mw_port_spi_register_tx_rx_isr_Ignore();
	mw_port_spi_register_error_isr_Ignore();
	HAL_GPIO_Init_Ignore();
	HAL_GPIO_WritePin_Ignore();
	nm_bsp_reset_Ignore();

	// Simulate misconfigured DMA itf for SPI TX
	hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH + 1;
	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_TX_HANDLE,
		HAL_DMA_STATE_RESET);
	mw_port_spi_dma_tx_init_ExpectAndReturn(HAL_OK);

	// Simulate misconfigured DMA itf for SPI RX
	hdma_rx.Init.Mode = DMA_NORMAL + 1;
	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_RX_HANDLE,
		HAL_DMA_STATE_RESET);
	mw_port_spi_dma_rx_init_ExpectAndReturn(HAL_OK);

	/* Configs fail validation; overwritten.
	 * Note: the exact order of config overwrite is irrelevant. */
	HAL_DMA_Init_Stub(helper_capture_dma_init);

	/* Enable DMA interrupts in NVIC.
	 * The relative order of priority configuration and interrupt enabling
	 * is irrelevant, so both calls are captured per IRQ number and verified
	 * via membership searches below. */
	HAL_NVIC_SetPriority_StubWithCallback(helper_capture_nvic_priority);
	HAL_NVIC_EnableIRQ_StubWithCallback(helper_capture_nvic_enable);


	// Assert successful initialisation
	TEST_ASSERT_EQUAL_UINT8(M2M_SUCCESS, nm_bus_init(NULL));


	// Assert DMA configs
	DMA_InitTypeDef dma_tx_init = {
		.Mode = DMA_NORMAL,
		.Direction = DMA_MEMORY_TO_PERIPH,
		.MemInc = DMA_MINC_ENABLE,
		.PeriphInc = DMA_PINC_DISABLE
	}, dma_rx_init = {
		.Mode = DMA_NORMAL,
		.Direction = DMA_PERIPH_TO_MEMORY,
		.MemInc = DMA_MINC_ENABLE,
		.PeriphInc = DMA_PINC_DISABLE
	};

	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&dma_tx_init,
		dma_init_captures, sizeof(dma_init_capture_t), CAPTURES_MAX_COUNT));
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&dma_rx_init,
		dma_init_captures, sizeof(dma_init_capture_t), CAPTURES_MAX_COUNT));

	// Assert DMA IRQ config in NVIC
	IRQn_Type dma_tx_irqn = CONF_WINC_SPI_DMA_TX_IRQN + 1;
	IRQn_Type dma_rx_irqn = CONF_WINC_SPI_DMA_RX_IRQN + 1;

	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&dma_tx_irqn,
		nvic_priority_captures, sizeof(IRQn_Type), CAPTURES_MAX_COUNT));
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&dma_rx_irqn,
		nvic_priority_captures, sizeof(IRQn_Type), CAPTURES_MAX_COUNT));

	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&dma_tx_irqn,
		nvic_enable_captures, sizeof(IRQn_Type), CAPTURES_MAX_COUNT));
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&dma_rx_irqn,
		nvic_enable_captures, sizeof(IRQn_Type), CAPTURES_MAX_COUNT));


	// Assert SPI <-> DMA linkage
	TEST_ASSERT_EQUAL_PTR(CONF_WINC_SPI_HANDLE.hdmatx,
		&CONF_WINC_SPI_DMA_TX_HANDLE);
	TEST_ASSERT_EQUAL_PTR(CONF_WINC_SPI_DMA_TX_HANDLE.Parent,
		&CONF_WINC_SPI_HANDLE);

	TEST_ASSERT_EQUAL_PTR(CONF_WINC_SPI_HANDLE.hdmarx,
		&CONF_WINC_SPI_DMA_RX_HANDLE);
	TEST_ASSERT_EQUAL_PTR(CONF_WINC_SPI_DMA_RX_HANDLE.Parent,
		&CONF_WINC_SPI_HANDLE);
}

/**
 * @brief Tests that nm_bus_init initialises the SPI interface and SS pin with
 * the module-required configuration; in particular, it enables SPI IRQs in the
 * NVIC and registers ISRs for SPI events.
 *
 * @note Covers REQ-FUN-12,16,38 and REQ-FUN-19 partially.
 */
void test_nm_bus_init_configures_spi_interface_and_ss_pin(void) {
	// Ignores: DMA, SS deassert and nm_bsp_reset
	HAL_DMA_GetState_IgnoreAndReturn(HAL_DMA_STATE_RESET);
	mw_port_spi_dma_tx_init_IgnoreAndReturn(HAL_OK);
	mw_port_spi_dma_rx_init_IgnoreAndReturn(HAL_OK);
	HAL_DMA_Init_IgnoreAndReturn(HAL_OK);
	HAL_GPIO_WritePin_Ignore();
	nm_bsp_reset_Ignore();

	// ISR registering
	mw_port_spi_register_tx_isr_Expect(spi_on_irq_success);
	mw_port_spi_register_rx_isr_Expect(spi_on_irq_success);
	mw_port_spi_register_tx_rx_isr_Expect(spi_on_irq_success);
	mw_port_spi_register_error_isr_Expect(spi_on_irq_error);

	// Capture SPI init and GPIO init calls
	HAL_SPI_Init_Stub(helper_capture_spi_init);
	HAL_GPIO_Init_Stub(helper_capture_gpio_init);

	// Capture NVIC calls
	HAL_NVIC_SetPriority_Stub(helper_capture_nvic_priority);
	HAL_NVIC_EnableIRQ_Stub(helper_capture_nvic_enable);

	// Assert return value
	TEST_ASSERT_EQUAL(M2M_SUCCESS, nm_bus_init(NULL));

	// Validate registered ISR callbacks
	mw_port_spi_sync_notify_Expect();
	spi_on_irq_success(&CONF_WINC_SPI_HANDLE);

	mw_port_spi_sync_notify_error_Expect();
	spi_on_irq_error(&CONF_WINC_SPI_HANDLE);

	// Assert SPI and GPIO inits
	spi_init_capture_t spi_init = {
		.Instance = CONF_WINC_SPI_INSTANCE,
		.Mode = SPI_MODE_MASTER,
		.Direction = SPI_DIRECTION_2LINES,
		.DataSize = SPI_DATASIZE_8BIT,
		.CLKPolarity = SPI_POLARITY_LOW,
		.CLKPhase = SPI_PHASE_1EDGE,
		.NSS = SPI_NSS_SOFT,
		.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4,
		.FirstBit = SPI_FIRSTBIT_MSB,
		.TIMode = SPI_TIMODE_DISABLE,
		.CRCCalculation = SPI_CRCCALCULATION_DISABLE,
	};
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&spi_init, spi_init_captures,
		sizeof(spi_init_capture_t), CAPTURES_MAX_COUNT));

	gpio_init_capture_t gpio_init = {
		.port = CONF_WINC_SPI_SS_PORT,
		.init = {
			.Pin = CONF_WINC_SPI_SS_PIN,
			.Mode = GPIO_MODE_OUTPUT_PP,
			.Pull = GPIO_NOPULL,
			.Speed = GPIO_SPEED_FREQ_LOW
		}
	};
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&gpio_init, gpio_init_captures,
		sizeof(gpio_init_capture_t), CAPTURES_MAX_COUNT));

	// Assert SPI IRQ in NVIC
	nvic_capture_t spi_irqn = CONF_WINC_SPI_IRQN + 1;

	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&spi_irqn,
		nvic_priority_captures, sizeof(nvic_capture_t), CAPTURES_MAX_COUNT));
	TEST_ASSERT_EQUAL_UINT8(1, helper_memsearch(&spi_irqn,
		nvic_enable_captures, sizeof(nvic_capture_t), CAPTURES_MAX_COUNT));
}

/**
 * @brief Tests that nm_bus_init deasserts the SS pin and resets the module
 * via the nm_bsp_reset call.
 *
 * @note Covers REQ-FUN-17,18 and REQ-FUN-19 partially.
 */
void test_nm_bus_init_deasserts_ss_pin_and_calls_nm_bsp_reset(void) {
	// Ignores: DMA, SPI, NVIC, SS pin init
	HAL_DMA_GetState_IgnoreAndReturn(HAL_DMA_STATE_RESET);
	mw_port_spi_dma_tx_init_IgnoreAndReturn(HAL_OK);
	mw_port_spi_dma_rx_init_IgnoreAndReturn(HAL_OK);
	HAL_DMA_Init_IgnoreAndReturn(HAL_OK);
	HAL_SPI_Init_IgnoreAndReturn(HAL_OK);
	mw_port_spi_register_tx_isr_Ignore();
	mw_port_spi_register_rx_isr_Ignore();
	mw_port_spi_register_tx_rx_isr_Ignore();
	mw_port_spi_register_error_isr_Ignore();
	HAL_NVIC_SetPriority_Ignore();
	HAL_NVIC_EnableIRQ_Ignore();
	HAL_GPIO_Init_Ignore();

	// SS deasserted, module reset
	HAL_GPIO_WritePin_Expect(CONF_WINC_SPI_SS_PORT, CONF_WINC_SPI_SS_PIN,
		GPIO_PIN_SET);
	nm_bsp_reset_Expect();

	// Assert return value
	TEST_ASSERT_EQUAL(M2M_SUCCESS, nm_bus_init(NULL));
}

/**
 * @brief Tests that nm_bus_init returns a negative error code when any of its
 * initialisation steps fails: DMA port initialisation (TX or RX), DMA
 * configuration validation (TX or RX), or SPI interface initialisation.
 *
 * @note Covers the rest of REQ-FUN-19.
 */
void test_nm_bus_init_on_failure_returns_negative_error_code(void) {
	// Ignores: NVIC
	HAL_NVIC_SetPriority_Ignore();
	HAL_NVIC_EnableIRQ_Ignore();

	/* ---- SPI TX DMA port init failure ---- */
	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_TX_HANDLE,
		HAL_DMA_STATE_RESET);
	mw_port_spi_dma_tx_init_ExpectAndReturn(HAL_ERROR);

	TEST_ASSERT_EQUAL(M2M_ERR_BUS_FAIL, nm_bus_init(NULL));

	/* ---- SPI RX DMA port init failure ---- */
	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_TX_HANDLE,
		HAL_DMA_STATE_READY);
	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_RX_HANDLE,
		HAL_DMA_STATE_RESET);
	mw_port_spi_dma_rx_init_ExpectAndReturn(HAL_ERROR);

	TEST_ASSERT_EQUAL(M2M_ERR_BUS_FAIL, nm_bus_init(NULL));

	/* ---- SPI TX DMA config validation failure ---- */
	helper_set_module_required_dma_configs();
	hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH + 1;

	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_TX_HANDLE,
		HAL_DMA_STATE_READY);
	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_RX_HANDLE,
		HAL_DMA_STATE_READY);
	HAL_DMA_Init_ExpectAndReturn(&CONF_WINC_SPI_DMA_TX_HANDLE, HAL_ERROR);

	TEST_ASSERT_EQUAL(M2M_ERR_BUS_FAIL, nm_bus_init(NULL));

	/* ---- SPI RX DMA config validation failure ---- */
	helper_set_module_required_dma_configs();
	hdma_rx.Init.Mode = DMA_NORMAL + 1;

	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_TX_HANDLE,
		HAL_DMA_STATE_READY);
	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_RX_HANDLE,
		HAL_DMA_STATE_READY);
	HAL_DMA_Init_ExpectAndReturn(&CONF_WINC_SPI_DMA_RX_HANDLE, HAL_ERROR);

	TEST_ASSERT_EQUAL(M2M_ERR_BUS_FAIL, nm_bus_init(NULL));

	/* ---- SPI interface init failure ---- */
	helper_set_module_required_dma_configs();

	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_TX_HANDLE,
		HAL_DMA_STATE_READY);
	HAL_DMA_GetState_ExpectAndReturn(&CONF_WINC_SPI_DMA_RX_HANDLE,
		HAL_DMA_STATE_READY);
	HAL_SPI_Init_ExpectAndReturn(&CONF_WINC_SPI_HANDLE, HAL_ERROR);

	TEST_ASSERT_EQUAL(M2M_ERR_BUS_FAIL, nm_bus_init(NULL));
}

/**
 * @brief Tests that nm_bus_deinit deinitialises the SS pin, deregisters the
 * SPI event ISR callbacks and returns M2M_SUCCESS.
 *
 * @note Covers REQ-FUN-20,21,22.
 *
 * @note Since no mock expectation is set for any function other than the SS
 * de-initialisation and the ISR de-registrations above, any call affecting
 * the SPI or DMA interfaces fails the test via CMock's fail-on-unexpected-
 * calls behaviour.
 */
void test_nm_bus_deinit_deinitialises_ss_pin_and_deregisters_spi_isrs(void) {
	// SS pin de-initialised
	HAL_GPIO_DeInit_Expect(CONF_WINC_SPI_SS_PORT, CONF_WINC_SPI_SS_PIN);

	// SPI event ISR callbacks deregistered
	mw_port_spi_deregister_tx_isr_Expect();
	mw_port_spi_deregister_rx_isr_Expect();
	mw_port_spi_deregister_tx_rx_isr_Expect();
	mw_port_spi_deregister_error_isr_Expect();

	// Assert return value
	TEST_ASSERT_EQUAL(M2M_SUCCESS, nm_bus_deinit());
}

/**
 * @brief Tests that nm_bus_ioctl, when invoked with the RW opcode, forwards to
 * its second parameter to nm_spi_rw and propagates back its return value.
 *
 * @note Since the only supported SPI I/O operation is read-write, this test
 * fully covers REQ-FUN-23.
 *
 * @note Covers REQ-FUN-23,25,26,31 and REQ-FUN-32:37 partially.
 *
 * @note nm_spi_rw resides in the same translation unit as nm_bus_ioctl, so it
 * cannot be mocked; the selection is instead verified behaviourally, via the
 * read/write chain it triggers.
 */
void test_nm_bus_ioctl_on_rw_opcode_forwards_its_second_parameter_to_nm_spi_rw_and_propagates_back_its_return_value(void) {
	uint8_t tx_buf[4] = {0};
	uint8_t rx_buf[4] = {0};

	tstrNmSpiRw spi_rw_params = {
		.pu8InBuf = tx_buf,
		.pu8OutBuf = rx_buf,
		.u16Sz = sizeof(tx_buf)
	};

	/* ---- Success propagation ---- */

	// Read/write operation: full-duplex transfer of the forwarded parameters
	mw_port_spi_bus_acquire_Expect();
	helper_expect_ss_assert();
	mw_port_spi_sync_prepare_Expect();
#ifdef CONF_WINC_SPI_USE_DMA
	HAL_SPI_TransmitReceive_DMA_ExpectAndReturn(&CONF_WINC_SPI_HANDLE,
		tx_buf, rx_buf, sizeof(tx_buf), HAL_OK);
#else
	HAL_SPI_TransmitReceive_IT_ExpectAndReturn(&CONF_WINC_SPI_HANDLE,
		tx_buf, rx_buf, sizeof(tx_buf), HAL_OK);
#endif
	mw_port_spi_sync_wait_Expect();
	mw_port_spi_sync_get_error_status_ExpectAndReturn(0);
	helper_expect_ss_deassert();
	mw_port_spi_bus_release_Expect();

	TEST_ASSERT_EQUAL(M2M_SUCCESS,
		nm_bus_ioctl(NM_BUS_IOCTL_RW, &spi_rw_params));

	/* ---- Failure propagation ---- */

	// Read/write operation: full-duplex transfer of the forwarded parameters
	mw_port_spi_bus_acquire_Expect();
	helper_expect_ss_assert();
	mw_port_spi_sync_prepare_Expect();
#ifdef CONF_WINC_SPI_USE_DMA
	HAL_SPI_TransmitReceive_DMA_ExpectAndReturn(&CONF_WINC_SPI_HANDLE,
		tx_buf, rx_buf, sizeof(tx_buf), HAL_OK);
#else
	HAL_SPI_TransmitReceive_IT_ExpectAndReturn(&CONF_WINC_SPI_HANDLE,
		tx_buf, rx_buf, sizeof(tx_buf), HAL_OK);
#endif
	mw_port_spi_sync_wait_Expect();
	mw_port_spi_sync_get_error_status_ExpectAndReturn(1);
	helper_expect_ss_deassert();
	mw_port_spi_bus_release_Expect();

	TEST_ASSERT_EQUAL(M2M_ERR_BUS_FAIL,
		nm_bus_ioctl(NM_BUS_IOCTL_RW, &spi_rw_params));
}

/**
 * @brief Tests that nm_bus_ioctl rejects any non-read/write opcode by returning
 * M2M_ERR_INVALID_ARG, leaving the bus untouched.
 *
 * @note Covers REQ-FUN-24.
 *
 * @note Since no mock expectation is set, any call affecting the bus fails
 * the test via CMock's fail-on-unexpected-calls behaviour.
 */
void test_nm_bus_ioctl_on_non_rw_opcode_returns_invalid_arg(void) {
	const uint8_t invalid_opcodes[] = {
		NM_BUS_IOCTL_R,
		NM_BUS_IOCTL_W,
		NM_BUS_IOCTL_W_SPECIAL,
		NM_BUS_IOCTL_WR_RESTART,
		0xFF
	};

	tstrNmSpiRw spi_rw_params = {0};

	for (size_t i = 0; i < sizeof(invalid_opcodes); i++) {
		TEST_ASSERT_EQUAL_UINT8(M2M_ERR_INVALID_ARG,
			nm_bus_ioctl(invalid_opcodes[i], &spi_rw_params));
	}
}

/**
 * @brief Tests that nm_spi_rw, when invoked with a transmit buffer and a NULL
 * receive buffer, initiates a transmit-only transfer of the specified number
 * of bytes and returns M2M_SUCCESS.
 *
 * @note Covers REQ-FUN-29 and REQ-FUN-32:37 partially.
 */
void test_nm_spi_rw_on_null_rx_buffer_transmits_half_duplex(void) {
	uint8_t tx_buf[4] = {0};

	// Transmit-only operation
	mw_port_spi_bus_acquire_Expect();
	helper_expect_ss_assert();
	mw_port_spi_sync_prepare_Expect();
#ifdef CONF_WINC_SPI_USE_DMA
	HAL_SPI_Transmit_DMA_ExpectAndReturn(&CONF_WINC_SPI_HANDLE,
		tx_buf, sizeof(tx_buf), HAL_OK);
#else
	HAL_SPI_Transmit_IT_ExpectAndReturn(&CONF_WINC_SPI_HANDLE,
		tx_buf, sizeof(tx_buf), HAL_OK);
#endif
	mw_port_spi_sync_wait_Expect();
	mw_port_spi_sync_get_error_status_ExpectAndReturn(0);
	helper_expect_ss_deassert();
	mw_port_spi_bus_release_Expect();

	// Assert return value
	TEST_ASSERT_EQUAL(M2M_SUCCESS, nm_spi_rw(tx_buf, NULL, sizeof(tx_buf)));
}

/**
 * @brief Tests that nm_spi_rw, when invoked with a NULL transmit buffer and a
 * non-NULL receive buffer, initiates a receive-only transfer of the specified
 * number of bytes and returns M2M_SUCCESS.
 *
 * @note Covers REQ-FUN-30 and what remains of REQ-FUN-32:37.
 */
void test_nm_spi_rw_on_null_tx_buffer_receives_half_duplex(void) {
	uint8_t rx_buf[4] = {0};

	// Receive-only operation
	mw_port_spi_bus_acquire_Expect();
	helper_expect_ss_assert();
	mw_port_spi_sync_prepare_Expect();
#ifdef CONF_WINC_SPI_USE_DMA
	HAL_SPI_Receive_DMA_ExpectAndReturn(&CONF_WINC_SPI_HANDLE,
		rx_buf, sizeof(rx_buf), HAL_OK);
#else
	HAL_SPI_Receive_IT_ExpectAndReturn(&CONF_WINC_SPI_HANDLE,
		rx_buf, sizeof(rx_buf), HAL_OK);
#endif
	mw_port_spi_sync_wait_Expect();
	mw_port_spi_sync_get_error_status_ExpectAndReturn(0);
	helper_expect_ss_deassert();
	mw_port_spi_bus_release_Expect();

	// Assert return value
	TEST_ASSERT_EQUAL(M2M_SUCCESS, nm_spi_rw(NULL, rx_buf, sizeof(rx_buf)));
}

/**
 * @brief Tests that nm_spi_rw returns M2M_ERR_BUS_FAIL when invoked with a
 * transfer size of zero, or with both the transmit and receive buffers NULL.
 *
 * @note Covers REQ-FUN-27,28.
 *
 * @note Since no mock expectation is set, any call affecting the bus fails
 * the test via CMock's fail-on-unexpected-calls behaviour.
 */
void test_nm_spi_rw_on_invalid_arguments_returns_bus_fail(void) {
	uint8_t tx_buf[4] = {0};
	uint8_t rx_buf[4] = {0};

	// Zero transfer size
	TEST_ASSERT_EQUAL(M2M_ERR_BUS_FAIL, nm_spi_rw(tx_buf, rx_buf, 0));

	// Both buffers NULL
	TEST_ASSERT_EQUAL(M2M_ERR_BUS_FAIL,
		nm_spi_rw(NULL, NULL, sizeof(tx_buf)));
}


/* Unity harness -------------------------------------------------------------*/
void setUp(void) {
	memset(&CONF_WINC_SPI_HANDLE, 0, sizeof(CONF_WINC_SPI_HANDLE));
	memset(&CONF_WINC_SPI_DMA_TX_HANDLE, 0, sizeof(CONF_WINC_SPI_DMA_TX_HANDLE));
	memset(&CONF_WINC_SPI_DMA_RX_HANDLE, 0, sizeof(CONF_WINC_SPI_DMA_RX_HANDLE));

	mock_stm32_hal_Init();
	mock_nm_bsp_api_Init();
	mock_mw_port_Init();
	mock_conf_winc_Init();
}

void tearDown(void) {
	mock_stm32_hal_Verify();
	mock_stm32_hal_Destroy();

	mock_nm_bsp_api_Verify();
	mock_nm_bsp_api_Destroy();

	mock_mw_port_Verify();
	mock_mw_port_Destroy();

	mock_conf_winc_Verify();
	mock_conf_winc_Destroy();
}

int main(void) {
	UNITY_BEGIN();

	RUN_TEST(test_spi_transfer_capacity_greater_than_16_bytes);
	RUN_TEST(test_nm_bus_init_on_dma_misconfigured_overwrites_config);
	RUN_TEST(test_nm_bus_init_configures_spi_interface_and_ss_pin);
	RUN_TEST(test_nm_bus_init_deasserts_ss_pin_and_calls_nm_bsp_reset);
	RUN_TEST(test_nm_bus_init_on_failure_returns_negative_error_code);
	RUN_TEST(test_nm_bus_deinit_deinitialises_ss_pin_and_deregisters_spi_isrs);
	RUN_TEST(test_nm_bus_ioctl_on_rw_opcode_forwards_its_second_parameter_to_nm_spi_rw_and_propagates_back_its_return_value);
	RUN_TEST(test_nm_bus_ioctl_on_non_rw_opcode_returns_invalid_arg);
	RUN_TEST(test_nm_spi_rw_on_null_rx_buffer_transmits_half_duplex);
	RUN_TEST(test_nm_spi_rw_on_null_tx_buffer_receives_half_duplex);
	RUN_TEST(test_nm_spi_rw_on_invalid_arguments_returns_bus_fail);

	return UNITY_END();
}
