#ifndef MDP_H
#define MDP_H

#include <stdint.h>
void	MessageProcessEnCode(uint8_t CtlMode, uint8_t TrqLimLR, uint8_t TrqLimHR, uint32_t CheckCode, uint32_t MessageID, uint8_t *MessageArray, uint8_t *CheckCounter);

#endif