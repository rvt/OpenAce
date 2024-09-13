#pragma once

#include <stdint.h>

#include "ace/constants.hpp"
#include "ace/models.hpp"

#include "ace/messagerouter.hpp"
#include "ace/basemodule.hpp"
#include "ace/messages.hpp"
#include "ace/coreutils.hpp"
#include "ace/EMA.hpp"

#include "minmea.h"



/**
 * This decoder requires that both GGA and GMC sentences are received from the GPS and that each sentences us correct ms resolution
 * When both coralated sentences are sned. It will send out a ownship position message
*/
class GpsDecoder : public BaseModule, public etl::message_router<GpsDecoder, OpenAce::GPSMessage>
{
    friend class message_router;
    struct
    {
        uint32_t receivedGGA = 0;
        uint32_t receivedRMC = 0;
        uint32_t receivedGSA = 0;
        uint32_t receivedOther = 0;
        uint32_t startTime = CoreUtils::msSinceBoot();
    } statistics;

    static constexpr float INVALID_CONVERSION = -9999;

    float velocityNorth;
    float velocityEast;
    RatePerSecond<OPENACE_GPS_FREQUENCY> altitude{OPENACE_EMAFLOAT_K_FACTOR_1S};
    RatePerSecond<OPENACE_GPS_FREQUENCY> groundSpeed{OPENACE_EMAFLOAT_K_FACTOR_1S};
    RatePerSecond<OPENACE_GPS_FREQUENCY> course{OPENACE_EMAFLOAT_K_FACTOR_1S};
    RatePerSecond<OPENACE_GPS_FREQUENCY> latitude{OPENACE_EMAFLOAT_K_FACTOR_1S};
    RatePerSecond<OPENACE_GPS_FREQUENCY> longitude{OPENACE_EMAFLOAT_K_FACTOR_1S};

    uint8_t fixQuality;
    uint8_t satellitesTracked;
    float pDop;

    minmea_time lastRMCTimestamp;
    minmea_time lastGGATimestamp;
private:
    void on_receive(const OpenAce::GPSMessage& msg);

    /**
     * Convert an minmea_float with altitude/height information in meters
    */
    float convertToMeters(const struct minmea_float *value, char unit) const;

    /**
     * Send message when both GGA and RMC sentences are received
    */
    void sendMessageWhenGGAisRMC();

    void on_receive_unknown(const etl::imessage& msg)
    {
    }
public:
    static constexpr const etl::string_view NAME = "GpsDecoder";
    GpsDecoder(etl::imessage_bus& bus, const Configuration &config) : BaseModule(bus, NAME),
        fixQuality(0),
        satellitesTracked(0),
        pDop(255),
        lastRMCTimestamp({0,0,0,0}),
                     lastGGATimestamp({0,0,0,0})
    {
    }

    virtual ~GpsDecoder() = default;

    virtual OpenAce::PostConstruct postConstruct() override;

    virtual void start() override;
    virtual void stop() override;

    virtual void getData(etl::string_stream &stream, const etl::string_view optional) const override;
};
