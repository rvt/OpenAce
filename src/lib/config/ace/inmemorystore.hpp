#pragma once

#include <stdint.h>
#include "configstore.hpp"


/**
 * In memory store that allows to keep it's configuration even between reboots as long as power is on the PICO
 */
class InMemoryStore : public ConfigStore
{
    uint16_t position;
public:
    InMemoryStore() : position(0) {};

    virtual void rewind() override;
    virtual size_t write(uint8_t c) override;
    virtual size_t write(const uint8_t *data, size_t size) override;

    virtual const uint8_t *data() const override;
};
