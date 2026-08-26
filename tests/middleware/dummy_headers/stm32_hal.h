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

/**
 * @brief SPI and DMA
 */
#define SPI1	((SPI_TypeDef*) 0x40013000)

/**
 * @brief DMA config
 */
#define DMA_NORMAL				(0)
#define DMA_MEMORY_TO_PERIPH	(1)
#define DMA_PERIPH_TO_MEMORY	(2)
#define DMA_PINC_DISABLE		(3)
#define DMA_MINC_ENABLE			(4)

/**
 * @brief SPI config
 */
#define SPI_MODE_MASTER				(0)
#define SPI_DIRECTION_2LINES		(1)
#define SPI_DATASIZE_8BIT			(2)
#define SPI_POLARITY_LOW			(3)
#define SPI_PHASE_1EDGE				(4)
#define SPI_NSS_SOFT				(5)
#define SPI_BAUDRATEPRESCALER_4		(6)
#define SPI_FIRSTBIT_MSB			(7)
#define SPI_TIMODE_DISABLE			(8)
#define SPI_CRCCALCULATION_DISABLE	(9)

#define __HAL_LINKDMA(__HANDLE__, __PPP_DMA_FIELD__, __DMA_HANDLE__)	do {                                                      \
		(__HANDLE__)->__PPP_DMA_FIELD__ = &(__DMA_HANDLE__); \
		(__DMA_HANDLE__).Parent = (__HANDLE__);             \
	} while(0)


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
 * @brief DMA
 */
typedef struct {
	uint32_t Direction,
		PeriphInc,
		MemInc,
		Mode;
} DMA_InitTypeDef;

typedef struct {
	DMA_InitTypeDef Init;
	void* Parent;
} DMA_HandleTypeDef;

typedef enum {
	HAL_DMA_STATE_RESET,
	HAL_DMA_STATE_READY
} HAL_DMA_StateTypeDef;

/**
 * @brief SPI
 */
typedef struct {
	uint32_t dummy;
} SPI_TypeDef;

typedef struct {
	uint32_t Mode,
		Direction,
		DataSize,
		CLKPolarity,
		CLKPhase,
		NSS,
		BaudRatePrescaler,
		FirstBit,
		TIMode,
		CRCCalculation,
		CRCPolynomial;
} SPI_InitTypeDef;

typedef struct {
	SPI_TypeDef* Instance;
	SPI_InitTypeDef Init;
	DMA_HandleTypeDef *hdmatx, *hdmarx;
} SPI_HandleTypeDef;

/**
 * @brief IRQ
 */
typedef enum {
	EXTI0_IRQn,
	SPI1_IRQn,
	DMA1_Stream0_IRQn,
	DMA1_Stream1_IRQn
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

void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority,
	uint32_t SubPriority);
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn);
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn);

HAL_DMA_StateTypeDef HAL_DMA_GetState(DMA_HandleTypeDef* hdma);
HAL_StatusTypeDef HAL_DMA_Init(DMA_HandleTypeDef* hdma);

HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef* hspi);

HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef* hspi,
	uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef* hspi, uint8_t* pData,
	uint16_t Size);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef* hsi,
	uint8_t* pTxData, uint8_t* pRxData, uint16_t Size);

HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef* hspi,
	uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Receive_DMA(SPI_HandleTypeDef* hspi,
	uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef* hsi,
	uint8_t* pTxData, uint8_t* pRxData, uint16_t Size);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_STM32_HAL_H */
