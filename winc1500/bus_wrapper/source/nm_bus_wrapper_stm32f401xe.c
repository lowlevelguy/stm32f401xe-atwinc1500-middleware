#include <stdlib.h>

#include "nm_common.h"
#include "nm_bsp.h"
#include "nm_bus_wrapper.h"
#include "conf_winc.h"


#if !defined(CONF_WINC_USE_SPI) && !defined(CONF_WINC_USE_UART) && \
	!defined(CONF_WINC_USE_I2C)
#error "Please define a bus to use from the following: SPI, UART, I2C."
#endif

#if (defined(CONF_WINC_USE_UART) || defined(CONF_WINC_USE_I2C))
#error "Non-SPI buses are currently unsupported."
#endif

#ifndef BUILD_TESTING
#define STATIC static
#else
#define STATIC
#endif

#define UNUSED(x)	((void)(x))


/* Public variables ----------------------------------------------------------*/
tstrNmBusCapabilities egstrNmBusCapabilities = {
	.u16MaxTrxSz = 1024
};


/* Private functions ---------------------------------------------------------*/
#ifdef CONF_WINC_USE_SPI
/**
 * @brief Drives the Slave Select pin low.
 */
STATIC void spi_assert_ss(void) {
	HAL_GPIO_WritePin(CONF_WINC_SPI_SS_PORT, CONF_WINC_SPI_SS_PIN,
		GPIO_PIN_RESET);
}

/**
 * @brief Drives the Slave Select pin high.
 */
STATIC void spi_deassert_ss(void) {
	HAL_GPIO_WritePin(CONF_WINC_SPI_SS_PORT, CONF_WINC_SPI_SS_PIN,
		GPIO_PIN_SET);
}

/**
 * @brief IRQ Handler for successful SPI events
 *
 * @param hspi Pointer to SPI handle owning the IRQ
 */
STATIC void spi_on_irq_success(SPI_HandleTypeDef* hspi) {
	if (hspi->Instance == CONF_WINC_SPI_HANDLE.Instance) {
		CONF_WINC_SPI_SYNC_NOTIFY();
	}
}

/**
 * @brief IRQ Handler for error SPI events
 *
 * @param hspi Pointer to SPI handle owning the IRQ
 */
STATIC void spi_on_irq_error(SPI_HandleTypeDef* hspi) {
	if (hspi->Instance == CONF_WINC_SPI_HANDLE.Instance) {
		CONF_WINC_SPI_SYNC_NOTIFY_ERR();
	}
}
#endif


/* Public functions ----------------------------------------------------------*/
/**
 * @note Only supports SPI at the moment.
 *
 * @note Initialisiation is such that the Slave Select pin is deasserted by
 * default.
 */
sint8 nm_bus_init(void* config) {
	UNUSED(config);

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();

#ifdef CONF_WINC_USE_SPI
	__HAL_RCC_SPI2_CLK_ENABLE();

#ifdef CONF_WINC_SPI_USE_DMA
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* ---- Initialise DMA interfaces ---- */
	/* Unlike the SPI interface, the exact DMA configuration is mostly
	 * inconsequential to the driver's functionality. */
	if (HAL_DMA_GetState(&CONF_WINC_SPI_DMA_TX_HANDLE) == HAL_DMA_STATE_RESET) {
		if (CONF_WINC_SPI_DMA_TX_INIT() != HAL_OK) {
			M2M_ERR("Failed to initialise DMA for SPI TX.\n");
			return M2M_ERR_BUS_FAIL;
		}
	}

	if (HAL_DMA_GetState(&CONF_WINC_SPI_DMA_RX_HANDLE) == HAL_DMA_STATE_RESET) {
		if (CONF_WINC_SPI_DMA_RX_INIT() != HAL_OK) {
			M2M_ERR("Failed to initialise DMA for SPI RX.\n");
			return M2M_ERR_BUS_FAIL;
		}
	}


	/* ---- Validate DMA configs ---- */
	/* A minimal set of DMA configuration parameters are still crucial to driver
	 * functionality. */
	if (CONF_WINC_SPI_DMA_TX_HANDLE.Init.Direction != DMA_MEMORY_TO_PERIPH
	 || CONF_WINC_SPI_DMA_TX_HANDLE.Init.PeriphInc != DMA_PINC_DISABLE
	 || CONF_WINC_SPI_DMA_TX_HANDLE.Init.MemInc != DMA_MINC_ENABLE
	 || CONF_WINC_SPI_DMA_TX_HANDLE.Init.Mode != DMA_NORMAL) {

		M2M_DBG("Misconfigured DMA for SPI TX. Re-initialising...\n");

		CONF_WINC_SPI_DMA_TX_HANDLE.Init.Direction = DMA_MEMORY_TO_PERIPH;
		CONF_WINC_SPI_DMA_TX_HANDLE.Init.PeriphInc = DMA_PINC_DISABLE;
		CONF_WINC_SPI_DMA_TX_HANDLE.Init.MemInc = DMA_MINC_ENABLE;
		CONF_WINC_SPI_DMA_TX_HANDLE.Init.Mode = DMA_NORMAL;

		if (HAL_DMA_Init(&CONF_WINC_SPI_DMA_TX_HANDLE) != HAL_OK) {
			M2M_ERR("Failed to initialise DMA for SPI TX.\n");
			return M2M_ERR_BUS_FAIL;
		}
	 }

	if (CONF_WINC_SPI_DMA_RX_HANDLE.Init.Direction != DMA_PERIPH_TO_MEMORY
	 || CONF_WINC_SPI_DMA_RX_HANDLE.Init.PeriphInc != DMA_PINC_DISABLE
	 || CONF_WINC_SPI_DMA_RX_HANDLE.Init.MemInc != DMA_MINC_ENABLE
	 || CONF_WINC_SPI_DMA_RX_HANDLE.Init.Mode != DMA_NORMAL) {

		M2M_DBG("Misconfigured DMA for SPI RX. Re-initialising...\n");

		CONF_WINC_SPI_DMA_RX_HANDLE.Init.Direction = DMA_PERIPH_TO_MEMORY;
		CONF_WINC_SPI_DMA_RX_HANDLE.Init.PeriphInc = DMA_PINC_DISABLE;
		CONF_WINC_SPI_DMA_RX_HANDLE.Init.MemInc = DMA_MINC_ENABLE;
		CONF_WINC_SPI_DMA_RX_HANDLE.Init.Mode = DMA_NORMAL;

		if (HAL_DMA_Init(&CONF_WINC_SPI_DMA_RX_HANDLE) != HAL_OK) {
			M2M_ERR("Failed to initialise DMA for SPI RX.\n");
			return M2M_ERR_BUS_FAIL;
		}
	 }

	M2M_DBG("Initialised DMA.\n");

	/* ---- Link SPI to DMA, and configure NVIC ---- */
	__HAL_LINKDMA(&CONF_WINC_SPI_HANDLE, hdmatx, CONF_WINC_SPI_DMA_TX_HANDLE);
	HAL_NVIC_SetPriority(CONF_WINC_SPI_DMA_TX_IRQN, 0, 0);
	HAL_NVIC_EnableIRQ(CONF_WINC_SPI_DMA_TX_IRQN);

	__HAL_LINKDMA(&CONF_WINC_SPI_HANDLE, hdmarx, CONF_WINC_SPI_DMA_RX_HANDLE);
	HAL_NVIC_SetPriority(CONF_WINC_SPI_DMA_RX_IRQN, 0, 0);
	HAL_NVIC_EnableIRQ(CONF_WINC_SPI_DMA_RX_IRQN);
#endif /* CONF_WINC_SPI_USE_DMA */

	/* ---- Configure SPI interface ---- */
	/* Forcefully overwrite any previous user initialisation to ensure the
	 * configuration matches what's required by the module. */
	CONF_WINC_SPI_HANDLE.Instance = CONF_WINC_SPI_INSTANCE;
	CONF_WINC_SPI_HANDLE.Init.Mode = SPI_MODE_MASTER;
	CONF_WINC_SPI_HANDLE.Init.Direction = SPI_DIRECTION_2LINES;
	CONF_WINC_SPI_HANDLE.Init.DataSize = SPI_DATASIZE_8BIT;
	CONF_WINC_SPI_HANDLE.Init.CLKPolarity = SPI_POLARITY_LOW;
	CONF_WINC_SPI_HANDLE.Init.CLKPhase = SPI_PHASE_1EDGE;
	CONF_WINC_SPI_HANDLE.Init.NSS = SPI_NSS_SOFT;
	/* The SPI peripherals on the STM32F401xE can be fed from either of the APB1
	 * or APB2 peripheral clocks, capable of upto 42 MHz and 84 MHz frequencies
	 * respectively. Hence, irrespective of the SPI peripheral used, a prescaler
	 * value of 2 already suffices to conform to REQ-PERF-01.
	 * We still choose, however, a prescaler of 4 to reduce EM noise. */
	CONF_WINC_SPI_HANDLE.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	CONF_WINC_SPI_HANDLE.Init.FirstBit = SPI_FIRSTBIT_MSB;
	CONF_WINC_SPI_HANDLE.Init.TIMode = SPI_TIMODE_DISABLE;
	CONF_WINC_SPI_HANDLE.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	CONF_WINC_SPI_HANDLE.Init.CRCPolynomial = 10;

	if (HAL_SPI_Init(&CONF_WINC_SPI_HANDLE) != HAL_OK) {
		M2M_ERR("Failed to initialise SPI.\n");
		return M2M_ERR_BUS_FAIL;
	}

	M2M_DBG("Initialised SPI.\n");

	/* ---- Register SPI event ISRs ---- */
	/* We use one common IRQ handler callback since the only job we require of
	 * an ISR is to notify our bus wrapper of transfer completion. */
	CONF_WINC_SPI_REGISTER_TX_ISR(spi_on_irq_success);
	CONF_WINC_SPI_REGISTER_RX_ISR(spi_on_irq_success);
	CONF_WINC_SPI_REGISTER_TX_RX_ISR(spi_on_irq_success);
	CONF_WINC_SPI_REGISTER_ERROR_ISR(spi_on_irq_error);

	/* ---- Configure NVIC ---- */
	HAL_NVIC_SetPriority(CONF_WINC_SPI_IRQN, 0, 0);
	HAL_NVIC_EnableIRQ(CONF_WINC_SPI_IRQN);

	M2M_DBG("Configured SPI ISRs.\n");

	/* ---- Configure Slave Select ---- */
	GPIO_InitTypeDef gpio_init = {0};
	gpio_init.Pin = CONF_WINC_SPI_SS_PIN;
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_init.Pull = GPIO_NOPULL;
	gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(CONF_WINC_SPI_SS_PORT, &gpio_init);

	spi_deassert_ss();

	M2M_DBG("Initialised SS pin.\n");

	// Power module on
	nm_bsp_reset();
#endif /* CONF_WINC_USE_SPI */

	M2M_DBG("Initialised bus wrapper.\n");

	return M2M_SUCCESS;
}

/**
 * @note Only supports SPI at the moment.
 */
sint8 nm_bus_deinit(void) {
#ifdef CONF_WINC_USE_SPI

	/* ---- De-initialise SS pin ---- */
	HAL_GPIO_DeInit(CONF_WINC_SPI_SS_PORT, CONF_WINC_SPI_SS_PIN);

	/* ---- Unregister ISRs ---- */
	CONF_WINC_SPI_DEREGISTER_TX_ISR();
	CONF_WINC_SPI_DEREGISTER_RX_ISR();
	CONF_WINC_SPI_DEREGISTER_TX_RX_ISR();
	CONF_WINC_SPI_DEREGISTER_ERROR_ISR();

#endif

	M2M_DBG("Deinitialised bus wrapper.\n");

	return M2M_SUCCESS;
}

sint8 nm_bus_ioctl(uint8 opcode, void* params) {
#ifdef CONF_WINC_USE_SPI
	tstrNmSpiRw* spi_rw_params = (tstrNmSpiRw*)params;
#endif

	switch (opcode) {
#ifdef CONF_WINC_USE_SPI
	case NM_BUS_IOCTL_RW:
		return nm_spi_rw(spi_rw_params->pu8InBuf, spi_rw_params->pu8OutBuf,
			spi_rw_params->u16Sz);
#endif

	default:
		return M2M_ERR_INVALID_ARG;
	}
}

/**
 * @note Stub implementation; the module's current driver makes no reference to
 * it.
 */
sint8 nm_bus_reinit(void* config) {
	UNUSED(config);
	return M2M_SUCCESS;
}

#ifdef CONF_WINC_USE_SPI
sint8 nm_spi_rw(uint8* tx_buf, uint8* rx_buf, uint16 buf_size) {
	// At least one of the provided buffers should be non-NULL
	if ((tx_buf == NULL && rx_buf == NULL) || buf_size == 0) {
		return M2M_ERR_BUS_FAIL;
	}

	// Claim SPI bus
	CONF_WINC_SPI_BUS_ACQUIRE();
	M2M_DBG("Acquired SPI bus.\n");
	spi_assert_ss();

	CONF_WINC_SPI_SYNC_PREPARE();

	/* Fire SPI transfer.
	 * We ignore HAL return values, as a busy SPI interface would be the result
	 * of either hardware issues, or misuse of the bus sync interface by the
	 * user. */
#ifdef CONF_WINC_SPI_USE_DMA
	if (tx_buf == NULL) {
		(void)HAL_SPI_Receive_DMA(&CONF_WINC_SPI_HANDLE, rx_buf, buf_size);
		M2M_DBG("Awaiting RX of size %u...\n", buf_size);
	} else if (rx_buf == NULL) {
		(void)HAL_SPI_Transmit_DMA(&CONF_WINC_SPI_HANDLE, tx_buf, buf_size);
		M2M_DBG("Initiating TX of size %u...\n", buf_size);
	} else {
		(void)HAL_SPI_TransmitReceive_DMA(&CONF_WINC_SPI_HANDLE,
			tx_buf, rx_buf, buf_size);
		M2M_DBG("Initiating full-duplex TX/RX of size %u...\n", buf_size);
	}
#else /* !defined(CONF_WINC_SPI_USE_DMA) */
	if (tx_buf == NULL) {
		(void)HAL_SPI_Receive_IT(&CONF_WINC_SPI_HANDLE, rx_buf, buf_size);
	} else if (rx_buf == NULL) {
		(void)HAL_SPI_Transmit_IT(&CONF_WINC_SPI_HANDLE, tx_buf, buf_size);
	} else {
		(void)HAL_SPI_TransmitReceive_IT(&CONF_WINC_SPI_HANDLE,
			tx_buf, rx_buf, buf_size);
	}
#endif /* CONF_WINC_SPI_USE_DMA */

	// Wait until done or error
	CONF_WINC_SPI_SYNC_WAIT();

	// If a transfer error occurs, deassert SS, release SPI bus and report
	if (CONF_WINC_SPI_SYNC_ERR_STATUS == 1) {
		spi_deassert_ss();
		M2M_ERR("Transfer failed.\n");
		CONF_WINC_SPI_BUS_RELEASE();

		return M2M_ERR_BUS_FAIL;
	}
	M2M_DBG("Transfer complete.\n");

	// Free SPI bus
	spi_deassert_ss();
	M2M_DBG("Releasing SPI bus.\n");
	CONF_WINC_SPI_BUS_RELEASE();

	return M2M_SUCCESS;
}
#endif /* CONF_WINC_USE_SPI */