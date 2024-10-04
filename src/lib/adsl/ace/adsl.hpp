#pragma once

/* System. */
#include <stdint.h>

/* Vendor. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "message_buffer.h"

/* PICO. */
#include "pico/stdlib.h"
#include "pico/binary_info.h"

/* Vendor. */
#include "etl/map.h"
#include "etl/message_bus.h"
#include "etl/string.h"
#include "etl/bitset.h"

/* OpenACE. */
#include "ace/constants.hpp"
#include "ace/messagerouter.hpp"
#include "ace/basemodule.hpp"
#include "ace/messages.hpp"
#include "ace/utils.hpp"
#include "ace/coreutils.hpp"
#include "ace/basemodule.hpp"

/* Utils. */
#include "ace/ldpc.hpp"
#include "adsl_packet.hpp"


class ADSL : public BaseModule, public etl::message_router<ADSL,
    OpenAce::RadioRxFrame,
    OpenAce::OwnshipPositionMsg,
    OpenAce::RadioTxPositionRequest,
    OpenAce::GpsStatsMsg,
    OpenAce::ConfigUpdatedMsg>
{
    static constexpr int32_t DEFAULT_IGNORE_DISTANCE = 25000;
    static constexpr int32_t MAX_IGNORE_DISTANCE = 50000;

    friend class message_router;
    static constexpr uint8_t OGN_PACKET_LENGTH = 20;
    static constexpr uint8_t OGN_PACKET_LENGTH_FEC = 26;
    mutable struct
    {
        uint32_t receivedAircraftPositions = 0;
        uint32_t transmittedAircraftPositions = 0;
        uint32_t fecErr = 0;
        uint32_t outOfDistance = 0;
        uint32_t addressTypeFlarm = 0;
        uint32_t addressTypeRandom = 0;
        uint32_t addressTypeOgn = 0;
        uint32_t addressTypeICAO = 0;
        uint32_t addressTypeFanet = 0;
        uint32_t addressTypeOther = 0;
        uint32_t addressTypeReserved = 0;
        uint32_t encrypted = 0;
        uint32_t queueFullErr = 0;
        uint32_t relay = 0;
    } statistics;

    struct DataSourceTimeStats
    {
        etl::bitset<100> timeTenthMs;
        uint32_t frequency;
    };
    etl::vector<DataSourceTimeStats, 2> dataSourceTimeStats;

    TaskHandle_t taskHandle;
    QueueHandle_t frameConsumerQueue;
    OpenAce::OwnshipPositionInfo ownshipPosition;
    OpenAce::Config::OpenAceConfiguration openAceConfiguration;
    OpenAce::BarometricPressure lastBarometricPressure;
    LDPC_Decoder<OGN_PACKET_LENGTH*8, 48> decoder;
    OpenAce::GpsStatsMsg gpsStats;
    uint16_t distanceIgnore;
public:
    static constexpr const etl::string_view NAME = "ADSL";
    ADSL(etl::imessage_bus& bus, const Configuration &config) :
        BaseModule(bus, NAME),
        taskHandle(nullptr),
        frameConsumerQueue(nullptr),
        ownshipPosition()
    {
        int32_t v = config.valueByPath(25000, "ADSL", "distanceIgnore");
        distanceIgnore = std::max((int32_t)0, std::min(v, MAX_IGNORE_DISTANCE));
        openAceConfiguration = config.openAceConfig();
    }

    virtual ~ADSL() = default;

    virtual OpenAce::PostConstruct postConstruct() override;
    virtual void start() override;
    virtual void stop() override;
    virtual void getData(etl::string_stream &stream, const etl::string_view optional) const override;

private:
    /**
     * Send a FreeRTOS message when a ADSL is received
     * This will release the sender from the task and allow it to continue in a seperate thread
    */
    void on_receive(const OpenAce::RadioRxFrame &msg);
    void on_receive(const OpenAce::OwnshipPositionMsg &msg);
    void on_receive(const OpenAce::RadioTxPositionRequest &msg);
    void on_receive(const OpenAce::GpsStatsMsg &msg);
    void on_receive(const OpenAce::ConfigUpdatedMsg &msg);
    void on_receive_unknown(const etl::imessage& msg)
    {
    }

    // Transform a FLARM addressType to an openAce address type
    OpenAce::AddressType addressMapToAddressType(uint8_t addressMap) const;
    static OpenAce::AircraftCategory mapAircraftCategory(ADSL_Packet::AircraftCategory category) ;

    static uint8_t addressTypeToAddressMap(OpenAce::AddressType addressType) ;
    static ADSL_Packet::AircraftCategory  mapAircraftCategory(OpenAce::AircraftCategory category) ;
    /**
     * Keep track of what timestamp (roughly) we receive flarm frames
    */
    void addReceiveStat(uint32_t frequency);

    int8_t parseFrame(const ADSL_Packet &packet, int16_t rssiDbm);

    /**
     * Parse a flarm frame and send it
     *
     * Based on https://github.com/creaktive/flare/blob/master/flarm_decode.c
    */
//    int8_t ognParseFrame(flarmV7Packet_t *packet, int16_t rssiDbm);
    static void adslReceiveTask(void *arg);


};

