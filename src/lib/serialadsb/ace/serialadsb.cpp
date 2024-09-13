#include <stdio.h>

#include "serialadsb.hpp"

#include "message_buffer.h"

#include "string.h"

#include "ace/messagerouter.hpp"
#include "ace/basemodule.hpp"
#include "ace/messages.hpp"
#include "ace/coreutils.hpp"
#include "ace/constants.hpp"

#include "etl/map.h"
#include "etl/string.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"



void SerialADSB::start()
{
    pioSerial.start();
    xTaskCreate(serialADSBTask, "serialADSBTask", configMINIMAL_STACK_SIZE+512, this, tskIDLE_PRIORITY, &taskHandle);
};

void SerialADSB::stop()
{
    if (taskHandle!=nullptr)
    {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }

    pioSerial.stop();
};

void SerialADSB::serialADSBTask(void *arg)
{
    SerialADSB *serialADSB = static_cast<SerialADSB*>(arg);
    // @techdebt: THis does not look feel safe setting message handler after started
    QueueHandle_t xQueue = serialADSB->pioSerial.getHandle();

    while (true)
    {
        char receivedMessage[33];
        if (xQueueReceive(xQueue, &receivedMessage, portMAX_DELAY) == pdPASS)
        {
            serialADSB->statistics.totalReceived++;
//            serialADSB->getBus().receive(OpenAce::ADSBMessageBin{receivedMessage});
        }
    }
}


OpenAce::PostConstruct SerialADSB::postConstruct()
{
    pioSerial.postConstruct();

    // Initialise the GPS hardware
    puts("Looking for ADSB... ");
    uint32_t scanBaudRate = pioSerial.findBaudRate(1500);
    if (!scanBaudRate)
    {
        puts("No Serial ADSB");
        return OpenAce::PostConstruct::HARDWARE_NOT_FOUND;
    }

    return OpenAce::PostConstruct::OK;
}

void SerialADSB::getData(etl::string_stream &stream, const etl::string_view optional) const
{
    stream << "{";
    stream << "\"totalReceived\":" << statistics.totalReceived;
    stream << "}\n";
}
