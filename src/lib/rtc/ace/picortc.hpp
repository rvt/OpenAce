#pragma once

#include <stdint.h>

#include "ace/constants.hpp"
#include "ace/messagerouter.hpp"
#include "ace/basemodule.hpp"
#include "ace/messages.hpp"
#include "ace/coreutils.hpp"

#include "etl/map.h"
#include "etl/message_bus.h"

#include "hardware/rtc.h"
#include "pico/time.h"
#include "sys/time.h"

// When set, also set's the PICO's rtc


class PicoRtc : public RtcModule, public etl::message_router<PicoRtc, OpenAce::GpsTime>
{
    friend class message_router;

    struct
    {
        uint32_t epochSet=0;
        uint32_t delayUs=0; // Delay between PPS and when we received a time message from the GPS
        uint32_t highElapseTime=0;
        uint32_t ppsEventsReceived=0;
    } statistics;

private:
    /**
     * Set the PICO's hardware clock.
     * See: https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#timestamp
     * Use the convenience function PicoRtc::timeMsSinceBoot() or PicoRtc::timeUsSinceBoot() to get the times;
     * Use gettimeofday(&tv, nullptr); to get the time in seconds and microseconds since the epoch.
     * Only use it when you need absolute time (logging, display, etc) or to calculate time based encryption
     * When SET_PICO_RTC is set to true, also the RTC will be set, but that will only give second accuracy and will always
    */
    void on_receive(const OpenAce::GpsTime& msg);

    void on_receive_unknown(const etl::imessage& msg);

    uint32_t lastPpstime; // uint32_t since we use it for difference calculations
public:
    static constexpr const etl::string_view NAME = "PicoRtc";
    PicoRtc(etl::imessage_bus& bus, const Configuration &config) :  RtcModule(bus), lastPpstime(0)
    {
    }

    virtual ~PicoRtc() = default;

    virtual void ppsEvent() override;

    virtual OpenAce::PostConstruct postConstruct() override;

    virtual void start() override;

    virtual void stop() override;

    virtual void getData(etl::string_stream &stream, const etl::string_view optional) const override;
};
