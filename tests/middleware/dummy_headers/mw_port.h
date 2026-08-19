#ifndef MW_PORT_H
#define MW_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Public functions ----------------------------------------------------------*/
void mw_port_system_clock_config(void);

void mw_port_exti_register_isr(void (*isr)(uint16_t));
void mw_port_exti_deregister_isr(void);


#ifdef __cplusplus
}
#endif

#endif /* MW_PORT_H */
