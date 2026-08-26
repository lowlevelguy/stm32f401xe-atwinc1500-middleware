#ifndef CONF_WINC_H
#define CONF_WINC_H

#ifdef __cplusplus
extern "C" {

#endif

#include <stdio.h>
#include <stdint.h>
#include "stm32_hal.h"
#include "mw_port.h"


/* Defines -------------------------------------------------------------------*/
#ifndef STM32F401xE
#define STM32F401xE
#endif

/* Porting implementation ----------------------------------------------------*/
/**
 * @brief Debug
 */
#define CONF_WINC_DEBUG			1
#define CONF_WINC_PRINTF(...)	printf(__VA_ARGS__)

/**
 * @brief BSP config
 */
#define CONF_WINC_SYSTEM_CLOCK_INIT()  mw_port_system_clock_config()

#define CONF_WINC_IRQN_PORT     GPIOA
#define CONF_WINC_IRQN_PIN      GPIO_PIN_0
#define CONF_WINC_CHIP_EN_PORT  GPIOA
#define CONF_WINC_CHIP_EN_PIN   GPIO_PIN_1
#define CONF_WINC_RESET_N_PORT  GPIOA
#define CONF_WINC_RESET_N_PIN   GPIO_PIN_2

#define CONF_WINC_IRQN_EXTI_LINE		EXTI0_IRQn
#define CONF_WINC_EXTI_REGISTER_ISR(x)	mw_port_exti_register_isr(x)
#define CONF_WINC_EXTI_DEREGISTER_ISR()	mw_port_exti_deregister_isr()

/**
 * @brief Bus Wrapper config
 */
#define CONF_WINC_USE_SPI
#define CONF_WINC_SPI_HANDLE			hspi1
#define CONF_WINC_SPI_INSTANCE			SPI1
#define CONF_WINC_SPI_IRQN				SPI1_IRQn
#define CONF_WINC_SPI_SS_PORT			GPIOA
#define CONF_WINC_SPI_SS_PIN			GPIO_PIN_3

#define CONF_WINC_SPI_BUS_ACQUIRE()		mw_port_spi_bus_acquire()
#define CONF_WINC_SPI_BUS_RELEASE()		mw_port_spi_bus_release()

#define CONF_WINC_SPI_SYNC_PREPARE()	mw_port_spi_sync_prepare()
#define CONF_WINC_SPI_SYNC_WAIT()		mw_port_spi_sync_wait()
#define CONF_WINC_SPI_SYNC_NOTIFY()		mw_port_spi_sync_notify()
#define CONF_WINC_SPI_SYNC_NOTIFY_ERR()	mw_port_spi_sync_notify_error()
#define CONF_WINC_SPI_SYNC_ERR_STATUS	(mw_port_spi_sync_get_error_status())

#define CONF_WINC_SPI_REGISTER_TX_ISR(x)		mw_port_spi_register_tx_isr(x)
#define CONF_WINC_SPI_REGISTER_RX_ISR(x)		mw_port_spi_register_rx_isr(x)
#define CONF_WINC_SPI_REGISTER_TX_RX_ISR(x)		mw_port_spi_register_tx_rx_isr(x)
#define CONF_WINC_SPI_REGISTER_ERROR_ISR(x)		mw_port_spi_register_error_isr(x)
#define CONF_WINC_SPI_DEREGISTER_TX_ISR()		mw_port_spi_deregister_tx_isr()
#define CONF_WINC_SPI_DEREGISTER_RX_ISR()		mw_port_spi_deregister_rx_isr()
#define CONF_WINC_SPI_DEREGISTER_TX_RX_ISR()	mw_port_spi_deregister_tx_rx_isr()
#define CONF_WINC_SPI_DEREGISTER_ERROR_ISR()	mw_port_spi_deregister_error_isr()

/**
 * @brief DMA config
 */
#define CONF_WINC_SPI_USE_DMA

#define CONF_WINC_SPI_DMA_TX_HANDLE		hdma_tx
#define CONF_WINC_SPI_DMA_TX_INIT()		mw_port_spi_dma_tx_init()
#define CONF_WINC_SPI_DMA_TX_IRQN		DMA1_Stream0_IRQn

#define CONF_WINC_SPI_DMA_RX_HANDLE		hdma_rx
#define CONF_WINC_SPI_DMA_RX_INIT()		mw_port_spi_dma_rx_init()
#define CONF_WINC_SPI_DMA_RX_IRQN		DMA1_Stream1_IRQn

/* External variables --------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_tx, hdma_rx;

#ifdef __cplusplus
}
#endif

#endif /* CONF_WINC_H */
