#pragma once

#include <stdint.h>
#include "configstore.hpp"
/* PICO */
#include "hardware/flash.h"

#if defined(PICO_RP2040)

/**
 * @class FlashStore
 * @brief A class for storing and managing data in the flash memory of a microcontroller.
 *
 * This class provides functionality to store and retrieve data in the flash memory,
 * utilizing a specified size and offset from the end of the flash memory. It inherits
 * from the ConfigStore base class.
 */
class FlashStore : public ConfigStore
{
    uint32_t address() const;

    const uint16_t size;
    const uint32_t offsetFromEnd;

public:
    /**
     * @brief Constructor for FlashStore.
     *
     * Initializes a new instance of the FlashStore class with a specified size and offset from the end of the flash memory.
     *
     * The size and offset will be aligned on a FLASH_SECTOR_SIZE boundary. Multiple FlashStores should not overlap
     *
     * @param size_ The size of the memory area to use for storage, in bytes and rounded to FLASH_SECTOR_SIZE
     * @param offsetFromEnd_ The offset from the end of the flash memory, in bytes, where the storage begins and rounded to FLASH_SECTOR_SIZE
     */
    FlashStore(uint16_t size_, uint32_t offsetFromEnd_);

    virtual void rewind() override;

    virtual size_t write(uint8_t c) override;
    virtual size_t write(const uint8_t *data, size_t size) override;

    virtual const uint8_t *data() const override;

};
#endif
