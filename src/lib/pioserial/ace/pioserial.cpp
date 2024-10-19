#include <stdio.h>

#include "pioserial.hpp"

#include "ace/utils.hpp"

OpenAce::PostConstruct PioSerial::postConstruct()
{

    int numElements = sizeof(interruptHandlers) / sizeof(interruptHandlers[0]);
    while (handlerIdx < numElements && interruptHandlers[handlerIdx] != nullptr)
    {
        ++handlerIdx;
    }
    if (handlerIdx >= numElements)
    {
        return OpenAce::PostConstruct::HARDWARE_ERROR;
    }
    interruptHandlers[handlerIdx] = this;

    xQueue = xQueueCreate(PIOSERIAL_MAX_QUEUE_LENGTH, OpenAce::NMEA_MAX_LENGTH);


    // Set tx to out to prevent it from floating. Attached devices might receive random data
    gpio_init(txPin);
    gpio_set_dir(txPin, GPIO_OUT);
    gpio_put(txPin, 1);

    // Set up the state machine to use to use
    if (!add_pio_program(&uart_rx_program, &pio, &smIndx, &offset))
    {
        return OpenAce::PostConstruct::HARDWARE_ERROR;
    }
    uart_rx_program_init(pio, smIndx, offset, rxPin, baudrate);

    static_assert(PIO0_IRQ_1 == PIO0_IRQ_0 + 1 && PIO1_IRQ_1 == PIO1_IRQ_0 + 1, "");
    uint8_t pio_irq = (pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0; // pio_irq will become 7,8,9,10
    if (irq_get_exclusive_handler(pio_irq))
    {
        pio_irq++;
        if (irq_get_exclusive_handler(pio_irq))
        {
            return OpenAce::PostConstruct::HARDWARE_ERROR;
        }
    }

    switch (pio_irq)
    {
    case PIO0_IRQ_0:
        handler = pio0_irq0_func_handler;
        break;
    case PIO0_IRQ_1:
        handler = pio0_irq1_func_handler;
        break;
    case PIO1_IRQ_0:
        handler = pio1_irq0_func_handler;
        break;
    case PIO1_IRQ_1:
        handler = pio1_irq1_func_handler;
        break;
    default:
        return OpenAce::PostConstruct::HARDWARE_NOT_FOUND;;
    }
    return OpenAce::PostConstruct::OK;
}

// TODO: CHange to a SPSC queue from etl::cpp
QueueHandle_t PioSerial::getHandle() const
{
    return xQueue;
}

void PioSerial::start()
{
    // Enable interrupt
    uint8_t pio_irq = (pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0; // pio_irq will become 7,8,9,10
    uint8_t irq_index = pio_irq - ((pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0);
    irq_add_shared_handler(pio_irq, handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY); // Add a shared IRQ handler
    irq_set_enabled(pio_irq, true); // Enable the IRQ
    pio_set_irqn_source_enabled(pio, irq_index, static_cast<pio_interrupt_source>(pis_sm0_rx_fifo_not_empty + smIndx), true); // Set pio to tell us when the FIFO is NOT empty
};

void PioSerial::stop()
{

    uint8_t pio_irq = (pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0; // pio_irq will become 7,8,9,10
    uint8_t irq_index = pio_irq - ((pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0);

    // Disable interrupt
    pio_set_irqn_source_enabled(pio, irq_index, static_cast<pio_interrupt_source>(pis_sm0_rx_fifo_not_empty + smIndx), false);
    irq_set_enabled(pio_irq, false);
    irq_remove_handler(pio_irq, handler);

    // Cleanup pio
    pio_sm_set_enabled(pio, smIndx, false);
    pio_remove_program(pio, &uart_rx_program, offset);
    pio_sm_unclaim(pio, smIndx);

    // Remove the handler
    smIndx = -1;
    offset = -1;
    pio_irq = -1;
    interruptHandlers[handlerIdx] = nullptr;

    vQueueDelete(xQueue);
    xQueue = nullptr;
};


/**
 * IRQ called when the pio fifo is not empty, i.e. there are some characters on the uart
 * When a NMEA string is found, it will send a message using FReeRTOS
 * The message is guaranteed to be zero terminated
*/
void PioSerial::pio_irq_func(uint8_t irqHandlerIndex)
{
    PioSerial &pioSerial = *interruptHandlers[irqHandlerIndex];
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    while (!pio_sm_is_rx_fifo_empty(pioSerial.pio, pioSerial.smIndx))
    {
        char c = uart_rx_program_getc(pioSerial.pio, pioSerial.smIndx);
        if (pioSerial.charIndex >= OpenAce::NMEA_MAX_LENGTH)
        {
            // @todo I don't think we need this anymore
            memset(pioSerial.buffer, 0, pioSerial.charIndex);
            pioSerial.charIndex = 0;
        }
        pioSerial.buffer[pioSerial.charIndex++] = c;

        if (c == '\n' || c == '\r' )
        {
            if (pioSerial.buffer[0] == '$' && pioSerial.charIndex > 8)
            {
                pioSerial.buffer[pioSerial.charIndex] = '\0';
                xQueueSendFromISR(pioSerial.xQueue, &pioSerial.buffer, &xHigherPriorityTaskWoken);
            }
            // @todo I don't think we need this anymore
            memset(pioSerial.buffer, 0, pioSerial.charIndex);
            pioSerial.charIndex = 0;
        }
    }
    portYIELD_FROM_ISR (xHigherPriorityTaskWoken);
}

/**
 * Send that to the uart using blocking IO
 * At least one PIO needs to be available
 * Note: Don't use this for continues sending of data. This is only for sending a few bytes occasionally,
 * use an hardware uart for that.
 * If it's really needed we should add the code for continues sending of data, but it will claim a PIO for that
 *
 * @param data
 * @param length
 * @param baudRate
 * @return true if the data was sent
*/
bool PioSerial::sendBlocking(uint32_t givenBaudRate, const uint8_t *data, uint16_t length)
{
    PIO txPio;
    int txSmIndx;
    uint txOffset;

    if (!add_pio_program(&uart_tx_program, &txPio, &txSmIndx, &txOffset))
    {
        puts("failed to setup pio for tx");
        return false;
    }
    uart_tx_program_init(txPio, txSmIndx, txOffset, txPin, givenBaudRate);
    uart_tx_program_put(txPio, txSmIndx, data, length);
    pio_sm_unclaim(txPio, txSmIndx);
    return true;
}

bool PioSerial::sendBlocking(const uint8_t *data, uint16_t length)
{
    return sendBlocking(baudrate, data, length);
}

bool PioSerial::setBaudRate(uint32_t baudRate)
{
    if(pio!=nullptr)
    {
        pio_sm_set_enabled(pio, smIndx, false);
        uart_rx_program_init(pio, smIndx, offset, rxPin, baudRate);
        return true;
    }
    return false;
}

/**
 * Validate if the uart is receiving any valid data at the given baudrate
*/
bool PioSerial::testUartAtBaudrate(uint32_t testBaudRate, uint32_t maximumScanTimeMs, uint32_t ignoreFirstMs, uint16_t numcharsConsideringValid)
{
    if(pio!=nullptr)
    {
        setBaudRate(testBaudRate);
        bool hasData = uart_rx_program_test(pio, smIndx, 0x0a, 0x80, maximumScanTimeMs, ignoreFirstMs, numcharsConsideringValid);
        setBaudRate(baudrate);
        return hasData;
    }
    return false;
}

/**
 * Find a buadrate where the uart is sending data on
*/
uint32_t PioSerial::findBaudRate(uint32_t maxTimeOutMs)
{
    for (uint32_t baudRate : commonBaudrates)
    {
        // printf("Scanning %ldBd\n", commonBaudrates[i]);
        if (testUartAtBaudrate(baudRate, maxTimeOutMs))
        {
            return baudRate;
        }
    }
    return 0;
}

bool PioSerial::rxFlush(uint32_t timeOut)
{
    return uart_rx_flush(pio, smIndx, timeOut);
}

