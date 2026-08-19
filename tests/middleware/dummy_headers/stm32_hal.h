#ifndef MOCK_STM32_HAL_H
#define MOCK_STM32_HAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Defines -------------------------------------------------------------------*/
/**
 * @brief GPIO ports and pins
 */
#define GPIOA		((GPIO_TypeDef*) 0x40020000)
#define GPIO_PIN_0	(1 << 0)
#define GPIO_PIN_1	(1 << 1)
#define GPIO_PIN_2	(1 << 2)
#define GPIO_PIN_3	(1 << 3)

/**
 * @brief GPIO config
 */
#define GPIO_MODE_OUTPUT_PP		(0)
#define GPIO_MODE_IT_FALLING	(1)
#define GPIO_NOPULL				(2)
#define GPIO_PULLUP				(3)
#define GPIO_SPEED_FREQ_LOW		(4)

/**
 * @brief SysTick masks
 */
#define SysTick_CTRL_ENABLE_Msk		(1 << 0)
#define SysTick_CTRL_TICKINT_Msk	(1 << 1)

/**
 * @brief RCC GPIO clocks
 */
#define __HAL_RCC_GPIOA_CLK_ENABLE()
#define __HAL_RCC_GPIOB_CLK_ENABLE()
#define __HAL_RCC_GPIOC_CLK_ENABLE()
#define __HAL_RCC_GPIOD_CLK_ENABLE()
#define __HAL_RCC_GPIOE_CLK_ENABLE()
#define __HAL_RCC_GPIOH_CLK_ENABLE()
#define __HAL_RCC_SPI2_CLK_ENABLE()
#define __HAL_RCC_DMA1_CLK_ENABLE()


/* Types ---------------------------------------------------------------------*/
/**
 * @brief GPIO
 *
 * @note Deliberately holds a single dummy member so that sizeof is non-zero;
 * CMock pointer-argument comparisons require a measurable object size.
 */
typedef struct {
	uint32_t dummy;
} GPIO_TypeDef;

typedef struct {
	uint32_t Pin;
	uint32_t Mode;
	uint32_t Pull;
	uint32_t Speed;
} GPIO_InitTypeDef;

typedef enum {
	GPIO_PIN_RESET = 0,
	GPIO_PIN_SET   = 1
} GPIO_PinState;

/**
 * @brief SysTick
 */
typedef struct {
	uint32_t CTRL;
} SysTick_Type;

/**
 * @brief IRQ
 */
typedef enum {
	EXTI0_IRQn,
} IRQn_Type;

/**
 * @brief HAL Status
 */
typedef enum {
	HAL_OK       = 0,
	HAL_ERROR    = 1,
	HAL_BUSY     = 2,
	HAL_TIMEOUT  = 3
} HAL_StatusTypeDef;


/* Public variables ----------------------------------------------------------*/
extern SysTick_Type *SysTick;


/* Public functions ----------------------------------------------------------*/
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
	GPIO_PinState PinState);
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void HAL_GPIO_DeInit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_Delay(uint32_t Delay);
void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority);
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn);
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn);


#ifdef __cplusplus
}
#endif

#endif /* MOCK_STM32_HAL_H */
