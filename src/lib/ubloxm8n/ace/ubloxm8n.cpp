#include <stdio.h>

#include "ubloxm8n.hpp"
#include "message_buffer.h"

#include "string.h"

#include "ace/messagerouter.hpp"
#include "ace/basemodule.hpp"
#include "ace/messages.hpp"
#include "ace/coreutils.hpp"
#include "ace/constants.hpp"
#include "ace/utils.hpp"

#include "etl/map.h"
#include "etl/string.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "ace/utils.hpp"

// *INDENT-OFF*
inline constexpr uint8_t UbloxM8N_baudrate[] = {
    0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00,
    0x00, 0xC2, 0x01, 0x00, 0x23, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x5E}; // CFG_PRT Baudrate 115200

inline constexpr uint8_t UbloxM8N_warmstart[] = {
    0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x64}; // CFG_RST warm start

inline constexpr uint8_t UbloxM8N_saveBBR[] = {
    0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x1B, 0xA9}; // CFG_CFG, Safe into BBR

inline constexpr uint8_t UbloxM8N_saveBBR_FLASH[] = {
    0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x1D, 0xAB};

inline constexpr uint8_t UbloxM8N_defaultCfg[] = {
    0xB5, 0x62, 0x06, 0x07, 0x14, 0x00, 0x40, 0x42, 0x0F, 0x00, 0x18, 0x73, 0x01, 0x00, // CFG_CFG, reset default
    0x01, 0x01, 0x00, 0x00, 0x34, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0xF4};

constexpr uint8_t UbloxM8N_m8nConfig_size = 8;
inline constexpr uint8_t *UbloxM8N_m8nConfig[UbloxM8N_m8nConfig_size] = {
    (uint8_t[]){12, 0xB5, 0x62, 0x06, 0x13, 0x04, 0x00, 0x07, 0x00, 0xF0, 0x39, 0x4D, 0x02}, // CFG_ANT Settings Enable Voltage + Sort Circuit + Open Circuit

    (uint8_t[]){11, 0xB5, 0x62, 0x06, 0x06, 0x02, 0x00, 0x00, 0x00, 0x0E, 0x4A}, // CFG_DAT WSG84

    // (uint8_t[]){44, 0xB5, 0x62, 0x06, 0x3E, 0x24, 0x00, 0x00, 0x00, 0x20, 0x04, // CFG_GNSS COnfig GPS 8-16, SBAS 1-3, Gal 4-8, Beido 8-16
    //             0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
    //             0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01,
    //             0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01,
    //             0x03, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01, 0x01,
    //             0xDE, 0xEF},

    (uint8_t[]){44,
                0xB5, 0x62, 0x06, 0x3E, 0x24, 0x00, 0x00, 0x00, 0x20, 0x04, // CFG_GNSS COnfig GPS 8-16, SBAS 1-3, Gal 4-8, Glonass 4-12
                0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x00, 0x01,
                0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x01,
                0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x00, 0x01,
                0x06, 0x04, 0x0C, 0x00, 0x01, 0x00, 0x00, 0x01,
                0xD5, 0x9B},

    (uint8_t[]){44, 0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x07, 0x02, 0x00, 0x00, 0x00, 0x00, // CFG_NAV5 2G Airborn Fixmode 3D only
                0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x5E, 0x01,
                0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x85, 0x2A},

    (uint8_t[]){16, 0xB5, 0x62, 0x06, 0x16, 0x08, 0x00, 0x01, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, // CFG_SBAS
                0x2B, 0xB9},

    (uint8_t[]){28, 0xB5, 0x62, 0x06, 0x07, 0x14, 0x00, 0x40, 0x42, 0x0F, 0x00, 0x30, 0x1B, 0x0F, 0x00, // CFG_TP Timepulse
                0x01, 0x01, 0x00, 0x00, 0x34, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x10},

    (uint8_t[]){28, 0xB5, 0x62, 0x06, 0x17, 0x14, 0x00, 0x00, 0x21, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, // CFG NMEA 2.1, MAIN talker ID GP
                0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0x0C},

    (uint8_t[]){14, 0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A} // CFG_RATE 200ms GPS time

};
// *INDENT-ON*

// TODO: For some reason casting to RTC did not work, so we use a global pointer PicoRtc, properly some casting did not go well
RtcModule *UbloxM8N_rtc = nullptr;

// Call RTC in inteerupt to notive of when last second pulse happened
void UbloxM8N_pps_callback(uint32_t events)
{
    UbloxM8N_rtc->ppsEvent();
}

void UbloxM8N::start()
{
    pioSerial.start();
    xTaskCreate(ubloxM8NTask, "UbloxM8N"
                "Task",
                configMINIMAL_STACK_SIZE + 512, this, tskIDLE_PRIORITY, &taskHandle);
    registerPinInterupt(ppsPin, GPIO_IRQ_EDGE_RISE, UbloxM8N_pps_callback);
};

void UbloxM8N::stop()
{
    if (taskHandle != nullptr)
    {
        unregisterPinInterupt(ppsPin);
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }

    pioSerial.stop();
};

void UbloxM8N::ubloxM8NTask(void *arg)
{
    UbloxM8N *ubloxM8N = static_cast<UbloxM8N *>(arg);

    // Initialise and find the GPS
    while (!ubloxM8N->detectAndConfigureGPS())
    {
        vTaskDelay(TASK_DELAY_MS(1000));
    }

    QueueHandle_t xQueue = ubloxM8N->pioSerial.getHandle();
    while (true)
    {
        char receivedMessage[OpenAce::NMEA_MAX_LENGTH];
        if (xQueueReceive(xQueue, &receivedMessage, portMAX_DELAY) == pdPASS)
        {
            // @todo Harden by adding CRC checking
            const char *crcPos = strchr(receivedMessage, '*');
            if (crcPos == nullptr)
            {
                ubloxM8N->statistics.crcErrors++;
                continue;
            }
            ubloxM8N->statistics.totalReceived++;

            // GPS should have been configured to turn Talkers into the Main Talker (GP)
            // If that is not possible, we can turn on OPENACE_UBLOX_GNXXX_TO_GPXXX
            if (OPENACE_UBLOX_GNXXX_TO_GPXXX && receivedMessage[2] == 'N')
            {
                // Trun GNXXX sentences into GPXXX sentences
                // GNXXX messages are the combined receivers
                receivedMessage[2] = 'P';
                etl::string_ext sentence(receivedMessage, receivedMessage, OpenAce::NMEA_MAX_LENGTH);
                CoreUtils::addChecksumToNMEA(sentence);
            }
            // printf("%s\n", receivedMessage);
            ubloxM8N->getBus().receive(OpenAce::GPSMessage{receivedMessage});
        }
    }
}

bool UbloxM8N::detectAndConfigureGPS()
{
    // Initialise the GPS hardware
    statistics.status = "Search";
    uint32_t scanBaudRate = pioSerial.findBaudRate(25000);
    if (!scanBaudRate)
    {
        statistics.status = "NO GPS";
        return false;
    }
    statistics.baudrate = scanBaudRate;
    if (scanBaudRate != GPS_BAUDRATE)
    {
        statistics.status = "Found";
        // printf("GPS found at %ldBd setting to %ldBd, waiting for GPS to come back on... ", scanBaudRate, GPS_BAUDRATE);
        // RVT: when we use rxFlush we read a few characters from the uart which seems to be enough to get the GPS to respond?
        // @todo Need to understand this better because sometimes the GPS still fails to be detected
        pioSerial.sendBlocking(scanBaudRate, UbloxM8N_baudrate, sizeof(UbloxM8N_baudrate));
        pioSerial.rxFlush(100);
        pioSerial.sendBlocking(scanBaudRate, UbloxM8N_warmstart, sizeof(UbloxM8N_warmstart));
        pioSerial.rxFlush(100);
        for (uint8_t i = 0; i < 60; i++)
        {
            statistics.status = "NO GPS";
            if (pioSerial.testUartAtBaudrate(GPS_BAUDRATE, 250))
            {
                break;
            }
        }
        scanBaudRate = pioSerial.findBaudRate(25000);
        statistics.baudrate = scanBaudRate;
        if (scanBaudRate != GPS_BAUDRATE)
        {
            statistics.status = "Cfg Err";
            return false;
        }
        // Save to BBR so we don't have slow startup delays finding the uart
        // Temporary disabled to test finding of uBlox
        statistics.status = "BBR";
        // pioSerial.sendBlocking(GPS_BAUDRATE, UbloxM8N_saveBBR, sizeof(UbloxM8N_saveBBR));
        // pioSerial.rxFlush();
    }

    // Configure GPS
    for (uint8_t i = 0; i < UbloxM8N_m8nConfig_size; i++)
    {
        pioSerial.sendBlocking(GPS_BAUDRATE, &UbloxM8N_m8nConfig[i][1], UbloxM8N_m8nConfig[i][0]);
        pioSerial.rxFlush(100);
    }

    // ublox uses rizing pulse to trigger
    // https://portal.u-blox.com/s/question/0D52p0000D35wjlCQA/how-to-minimize-serial-output-time-variance
    // Note: when we really have a GPS without PPS, perhaps we can just call UbloxM8N_rtc->ppsEvent();
    // after detecting GMC? and just 'add' a few us to compensate for incomming GPS time messages?
    statistics.status = "Configured";
    statistics.baudrate = scanBaudRate;
    return true;
}

OpenAce::PostConstruct UbloxM8N::postConstruct()
{
    pioSerial.postConstruct();

    UbloxM8N_rtc = static_cast<RtcModule *>(moduleByName(*this, RtcModule::NAME));

    printf("configuring GPS ppm:%d ", ppsPin);
    return OpenAce::PostConstruct::OK;
}

void UbloxM8N::getData(etl::string_stream &stream, const etl::string_view optional) const
{
    stream << "{";
    stream << "\"crcErrors\":" << statistics.crcErrors;
    stream << ",\"totalReceived\":" << statistics.totalReceived;
    stream << ",\"status\":\"" << statistics.status << "\"";
    stream << ",\"baudrate\":" << statistics.baudrate;
    stream << "}\n";
}
