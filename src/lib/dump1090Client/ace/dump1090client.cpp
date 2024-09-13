#include <stdio.h>

#include "dump1090client.hpp"

OpenAce::PostConstruct Dump1090Client::postConstruct()
{
    receiver = static_cast<BinaryReceiver *>(BaseModule::moduleByName(*this, "ADSBDecoder", false));
    if (receiver == nullptr)
    {
        return OpenAce::PostConstruct::DEP_NOT_FOUND;
    }

    auto result = tcpClient.postConstruct();
    if (result != OpenAce::PostConstruct::OK)
    {
        return result;
    }

    timerHandle = xTimerCreate("dump1090Timer", TASK_DELAY_MS(2'000), pdTRUE, this, dump1090Timer);
    if (timerHandle == nullptr)
    {
        return OpenAce::PostConstruct::TIMER_ERROR;
    }

    return OpenAce::PostConstruct::OK;
}

void Dump1090Client::dump1090Timer(TimerHandle_t xTimer)
{
    auto *handle = (Dump1090Client *)pvTimerGetTimerID(xTimer);
    if (handle->tcpClient.isStopped() && (handle->stoppedCounter++ == 2))
    {
        handle->stoppedCounter = 0;
        handle->tcpClient.start();
    }
}

void Dump1090Client::stop()
{
    // xTimerDelete(timerHandle, TASK_DELAY_MS(250));
    tcpClient.stop();
    getBus().unsubscribe(*this);
};

void Dump1090Client::start()
{
    xTaskCreate(dump1090Task, "Bmp280Task", configMINIMAL_STACK_SIZE + 128, this, tskIDLE_PRIORITY, &taskHandle);
    getBus().subscribe(*this);
};

void Dump1090Client::dump1090Task(void *arg)
{
    Dump1090Client *dump1090 = static_cast<Dump1090Client *>(arg);
    while (true)
    {

        if (ulTaskNotifyTake(pdTRUE, TASK_DELAY_MS(1'000)))
        {
            // TODO: Handle shutdown
        }
        else
        {
            if (dump1090->tcpClient.isStopped() && (dump1090->stoppedCounter++ == 2))
            {
                dump1090->stoppedCounter = 0;
                dump1090->tcpClient.start();
            }
        }
    }
}

void Dump1090Client::getData(etl::string_stream &stream, const etl::string_view optional) const
{
    stream << "{";
    stream << "\"totalReceived\":" << statistics.totalReceived;
    stream << "}\n";
}

inline uint8_t getHexVal(char hex)
{
    uint8_t val = (uint8_t)hex;
    // For uppercase A-F letters:
    return val - (val < 58 ? 48 : 55);
    // For lowercase a-f letters:
    // return val - (val < 58 ? 48 : 87);
    // Or the two combined, but a bit slower:
    // return val - (val < 58 ? 48 : (val < 97 ? 55 : 87));
}

void Dump1090Client::hexStrToByteArray(const char hex[], uint8_t numBytes, uint8_t *byteArray) const
{
    const uint8_t hexLength = numBytes << 1;
    for (uint8_t i = 0, j = 0; i < hexLength; i += 2, ++j)
    {
        byteArray[j] = (getHexVal(hex[i]) << 4) | getHexVal(hex[i + 1]);
    }
}
