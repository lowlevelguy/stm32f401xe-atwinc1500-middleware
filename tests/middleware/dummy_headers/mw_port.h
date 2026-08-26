#ifndef MW_PORT_H
#define MW_PORT_H

#include <stdint.h>
#include "stm32_hal.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Public functions ----------------------------------------------------------*/
void mw_port_system_clock_config(void);

void mw_port_exti_register_isr(void (*isr)(uint16_t));
void mw_port_exti_deregister_isr(void);

void mw_port_spi_bus_acquire(void);
void mw_port_spi_bus_release(void);

void mw_port_spi_sync_prepare(void);
void mw_port_spi_sync_wait(void);
void mw_port_spi_sync_notify(void);
void mw_port_spi_sync_notify_error(void);
uint8_t mw_port_spi_sync_get_error_status(void);

void mw_port_spi_register_tx_isr(void (*isr)(SPI_HandleTypeDef*));
void mw_port_spi_register_rx_isr(void (*isr)(SPI_HandleTypeDef*));
void mw_port_spi_register_tx_rx_isr(void (*isr)(SPI_HandleTypeDef*));
void mw_port_spi_register_error_isr(void (*isr)(SPI_HandleTypeDef*));
void mw_port_spi_deregister_tx_isr(void);
void mw_port_spi_deregister_rx_isr(void);
void mw_port_spi_deregister_tx_rx_isr(void);
void mw_port_spi_deregister_error_isr(void);

HAL_StatusTypeDef mw_port_spi_dma_tx_init(void);
HAL_StatusTypeDef mw_port_spi_dma_rx_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MW_PORT_H */
