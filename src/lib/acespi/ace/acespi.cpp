#include "acespi.hpp"


OpenAce::PostConstruct AceSpi::postConstruct()
{
    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(static_cast<uint32_t>(miso), static_cast<uint32_t>(mosi), static_cast<uint32_t>(clk), GPIO_FUNC_SPI));
    spiConsumerQueue = xQueueCreate( 10, sizeof( ConsumerRequest ) );
    if (spiConsumerQueue == nullptr)
    {
        return OpenAce::PostConstruct::XQUEUE_ERROR;
    }
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(rst);
    gpio_set_dir(rst, GPIO_OUT);
    gpio_put(rst, 1);

    // Set default SPI bus frequency
    spi_init(OPENACE_SPI_DEFAULT, OPENOPENACE_SPI_DEFAULT_BUS_FREQUENCY * 1000 * 1000);
    gpio_set_function(miso, GPIO_FUNC_SPI);
    gpio_set_function(clk, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);

    // Reset ALL devices
    resetDevices();
    printf("Initialised on miso:%d clk:%d mosi:%d rst:%d (devices reset) ", miso, clk, mosi, rst);
    return OpenAce::PostConstruct::OK;
}


bool AceSpi::aquireSlot(uint8_t busFrequencyMhz, TaskHandle_t consumerHandle, uint32_t bits) const
{
    ConsumerRequest request{busFrequencyMhz, consumerHandle, bits};
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // fromISR functions are used for performance, they are 25% faster than the normal counterparts
    bool ret = xQueueSendFromISR(spiConsumerQueue, static_cast<void *>(&request), &xHigherPriorityTaskWoken) != pdPASS;
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    return ret;
}

void AceSpi::aceSpiTask(void *arg)
{
    AceSpi* aceSpi = static_cast<AceSpi*>(arg);
    ConsumerRequest comsumerRequest{};
    uint8_t lastBusFrequency = OPENOPENACE_SPI_DEFAULT_BUS_FREQUENCY;
    while (true)
    {
        // Wait for a client to request a slot
        if( xQueueReceive( aceSpi->spiConsumerQueue, &( comsumerRequest ),  portMAX_DELAY) == pdPASS )
        {
            if (lastBusFrequency!=comsumerRequest.busFrequency)
            {
                lastBusFrequency = comsumerRequest.busFrequency;
                spi_init(OPENACE_SPI_DEFAULT, lastBusFrequency * 1000'000);
            }
            xTaskNotify( comsumerRequest.taskHandle, comsumerRequest.notificationValue, eSetBits);
            if (!ulTaskNotifyTake( pdTRUE,  TASK_DELAY_MS(INIT_SPI_MAX_WAIT)))
            {
                // @techdebt: SHould we have all CPU consumer be registered and have some function to force CS back up
                // SO we can recover from this issue? To might be still communicating.. I should they pass the task handle so we can kill the task?
                // Or should this be a device reset??
                TaskStatus_t xTaskDetails;
                vTaskGetInfo(comsumerRequest.taskHandle, &xTaskDetails, pdTRUE, eInvalid);
                printf("Warning SPI bus never released by %s after %dms needs fxing\n", xTaskDetails.pcTaskName, INIT_SPI_MAX_WAIT);
            }
        }
    }
}

void AceSpi::on_receive_unknown(const etl::imessage& msg)
{

}