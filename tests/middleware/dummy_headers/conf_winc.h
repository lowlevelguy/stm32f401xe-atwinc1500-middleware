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


#ifdef __cplusplus
}
#endif

#endif /* CONF_WINC_H */
