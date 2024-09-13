#pragma once

#include <stdint.h>
#include <sys/time.h>

/* FreeRTOS. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

/* OpenACE. */
#include "ace/constants.hpp"
#include "ace/messagerouter.hpp"
#include "ace/basemodule.hpp"
#include "ace/messages.hpp"

/* ETL CPP */
// #include "etl/map.h"
// #include "etl/message_bus.h"

#include "GDL90.h"

/**
 * Client that can connect to a host and a port and expect to receive line terminated NMEA Messages
 * TODO: de-Couple UDP from the GDL90 so this service send GDL Message over the messagebus which can then be send over UDP or Serial or BlueToolh
 */
class Gdl90Service : public BaseModule, public etl::message_router<Gdl90Service, OpenAce::TrackedAircraftPositionMsg, OpenAce::OwnshipPositionMsg, OpenAce::ConfigUpdatedMsg>
{
    friend class message_router;

private:
    mutable struct
    {
        uint32_t heartbeatTx = 0;
        uint32_t ownshipPosTx = 0;
        uint32_t otherPosTx = 0;
        uint32_t encodingFailureErr = 0;
    } statistics;

    enum TaskState : uint8_t
    {
        // Message to indicate next period needs to be decided, send from a timer
        HEARTBEAT = 1,
        SHUTDOWN = 2,
    };

    TaskHandle_t taskHandle;
    GDL90 gdl90;
    OpenAce::Config::OpenAceConfiguration openAceConfiguration;
    SemaphoreHandle_t configMutex;

private:
    void static heartbeatTimerCallBack(TimerHandle_t xTimer);

    static void gdl90ServiceTask(void *arg);

    void on_receive(const OpenAce::TrackedAircraftPositionMsg &msg);

    void on_receive(const OpenAce::OwnshipPositionMsg &msg);

    void on_receive(const OpenAce::ConfigUpdatedMsg &msg);

    void packAndSend(const GDL90::RawBytes &unpacked);

    void on_receive_unknown(const etl::imessage &msg)
    {
    }

    static void sendHeartBeat(Gdl90Service &gdl90Service);

public:
    static constexpr const etl::string_view NAME = "Gdl90Service";
    Gdl90Service(etl::imessage_bus &bus, const Configuration &config) : BaseModule(bus, NAME), taskHandle(nullptr)
    {
        openAceConfiguration = config.openAceConfig();
    }

    virtual ~Gdl90Service() = default;

    virtual OpenAce::PostConstruct postConstruct() override;

    virtual void start() override;

    virtual void stop() override;

    virtual void getData(etl::string_stream &stream, const etl::string_view optional) const override;
};
