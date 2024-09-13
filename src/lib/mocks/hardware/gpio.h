#pragma once

#include <stdint.h>
#include "../pico.h"

typedef void (*gpio_irq_callback_t)(uint gpio, uint32_t event_mask);



inline void gpio_set_irq_enabled(uint gpio, uint32_t events, bool enabled)
{
    printf("gpio_set_irq_enabled: gpio=%d events=%d enabled=%d\n", gpio, events, enabled);
}

inline void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t events, bool enabled, gpio_irq_callback_t callback)
{
    printf("gpio_set_irq_enabled_with_callback: gpio=%d events=%d enabled=%d\n", gpio, events, enabled);
}
