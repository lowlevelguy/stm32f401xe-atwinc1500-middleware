#include "conf_winc.h"

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_tx;
DMA_HandleTypeDef hdma_rx;

/**
 * @brief CMock strict-ordering counters (:enforce_strict_ordering: true).
 *
 * @note The generated mocks declare these extern, but CMock never defines
 * them; the harness must provide exactly one definition per test executable.
 */
int GlobalExpectCount;
int GlobalVerifyOrder;
