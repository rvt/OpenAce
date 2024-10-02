#pragma once

#include "ace/coreutils.hpp"
#include "ace/constants.hpp"
#include "ace/models.hpp"

#include "etl/flat_set.h"
#include "etl/vector.h"
#include "etl/map.h"
#include "etl/flat_map.h"
#include "etl/flat_map.h"
#include "etl/unordered_map.h"

#include "cpr.hpp"

struct AdsbCombinedDataStatus
{
    uint32_t icao; // ICAO address
    OpenAce::IcaoAddress icaoAddress;
    uint8_t messageStatus; // How complete this message is
    uint32_t lastSeen;

    // Data from ADSB
    uint16_t velocity;          // Non decoded Ground speed in knots
    uint8_t category;           // category
    int16_t heading;            // Heading in degrees
    int32_t gnsAltitude;        // Altitude in meter
    int32_t raw_even_latitude;  // Non decoded latitude  even
    int32_t raw_even_longitude; // Non decoded longitude even
    int32_t raw_odd_latitude;   // Non decoded latitude   odd
    int32_t raw_odd_longitude;  // Non decoded longitude  odd
    int16_t baro_gnss_diff;     // Difference between barometric altitude and GNSS altitude
    float lat;                  // lat
    float lon;                  // lon
    uint8_t vert_rate_sign;     // Vert Rate Sign
    int16_t vert_rate;          // Non decoded longitude
    bool airborne;              // Non decoded longitude
    bool evict;                 // When set to true, the aircraft needs to be removed from cache
    bool operator<(const AdsbCombinedDataStatus &other) const
    {
        return icao < other.icao;
    }

    ~AdsbCombinedDataStatus() = default;

    // Constructor with icao and lastSeen parameters
    AdsbCombinedDataStatus()
        : icao(0), icaoAddress("-"), messageStatus(0), lastSeen(0),
          velocity(0.0f), category(0), heading(0), gnsAltitude(0), raw_even_latitude(0),
          raw_even_longitude(0), raw_odd_latitude(0), raw_odd_longitude(0), baro_gnss_diff(0),
          lat(0.0f), lon(0.0f), vert_rate(0.0f), airborne(false), evict(false)
    {
    }

    // Constructor with icao for search functions.
    AdsbCombinedDataStatus(uint32_t icao_)
        : icao(icao_)
    {
    }

    // Constructor with icao and lastSeen parameters
    AdsbCombinedDataStatus(uint32_t icao_, uint32_t lastSeen_)
        : icao(icao_), icaoAddress("-"), messageStatus(0), lastSeen(lastSeen_),
          velocity(0.0f), category(0), heading(0), gnsAltitude(0), raw_even_latitude(0),
          raw_even_longitude(0), raw_odd_latitude(0), raw_odd_longitude(0), baro_gnss_diff(0),
          lat(0.0f), lon(0.0f), vert_rate(0.0f), airborne(false), evict(false)
    {
    }
};

/**
 * Performance measurements calling the start method:
 * flat map took 16us
 * unordered_map takes 5us
 */
template <size_t SIZE, uint32_t EVICT_TIME_MS>
class AdsbDataCollector
{
    static constexpr uint8_t ADSBDECODER_MS_DELAY_SERIAL_AND_OVERHEAD = 5; // Estimate time to send 24characters + some additional overhead
    static constexpr uint8_t HAS_POSITION_ODD = 1 << 0;                    //
    static constexpr uint8_t HAS_POSITION_EVEN = 1 << 1;                   //
    static constexpr uint8_t HAS_HEADING = 1 << 2;                         //
    static constexpr uint8_t HAS_VELOCITY = 1 << 3;
    static constexpr uint8_t HAS_ALTITUDE = 1 << 4; //
    static constexpr uint8_t HAS_POSITION_UPDATED = 1 << 5;
    static constexpr uint8_t CHECK_HAS_ICAOADDRESS = 1 << 6;
    static constexpr uint8_t VALID_MASK = HAS_POSITION_ODD | HAS_POSITION_EVEN | HAS_HEADING | HAS_VELOCITY | HAS_ALTITUDE | HAS_POSITION_UPDATED;

    static constexpr uint32_t CLEAR_UP_SIZE = (SIZE * 90) / 100;

private:
    struct AdsbCombinedDataStatusEq
    {
        bool operator()(uint32_t lhs, uint32_t rhs) const
        {
            return lhs == rhs;
        }
    };

    struct AdsbCombinedDataStatusHash
    {
        size_t operator()(uint32_t e) const
        {
            return size_t(e);
        }
    };

    etl::unordered_map<uint32_t, AdsbCombinedDataStatus, SIZE, 8, AdsbCombinedDataStatusHash, AdsbCombinedDataStatusEq> cache;
    AdsbCombinedDataStatus defaultStatus;

    // Declare a reference to the defaultStatus
    AdsbCombinedDataStatus *currentDataStatus = &defaultStatus;

    inline void decodePCR(bool fflag)
    {
        // only when all positions are in AND updated the decodeCPR is ran
        if ((currentDataStatus->messageStatus & (HAS_POSITION_ODD | HAS_POSITION_EVEN)) == (HAS_POSITION_ODD | HAS_POSITION_EVEN))
        {
            currentDataStatus->messageStatus |= HAS_POSITION_UPDATED;
            decodeCPR(fflag,
                      currentDataStatus->raw_even_latitude, currentDataStatus->raw_even_longitude,
                      currentDataStatus->raw_odd_latitude, currentDataStatus->raw_odd_longitude,
                      &currentDataStatus->lat, &currentDataStatus->lon);
        }
    }

public:
    void clear() {
        cache.clear();
    }
    bool start(uint32_t address, uint32_t msSinceBoot)
    {
        auto it = cache.find(address);
        if (it != cache.end())
        {
            currentDataStatus = &it->second;
            currentDataStatus->lastSeen = msSinceBoot;
            return true;
        }

        static uint8_t evictCycle = 0;
        evictCycle++;
        if (evictCycle == 25 || cache.full())
        {
            evictCycle = 0;
            evictOldEntries(msSinceBoot);
        }

        if (!cache.full())
        {
            cache[address] = AdsbCombinedDataStatus{address, msSinceBoot};
            auto it = cache.find(address);
            currentDataStatus = &it->second;
            return true;
        }

        currentDataStatus = &defaultStatus;
        return false;
    }

    void evictOldEntries(uint32_t msSinceBoot)
    {

        // Always ensure there is room for new cache entries be reducing evictTime untill there is room again
        // auto evictTime = EVICT_TIME_MS;
        // while (cache.full() && evictTime > 2000)
        // {
        //     cache.erase(etl::remove_if(cache.cbegin(), cache.cend(), [msSinceBoot, evictTime](const auto &it)
        //                                { return it.second.evict; }),
        //                 cache.end());
        //     evictTime -= 1000;
        // }

        auto evictTime = EVICT_TIME_MS;
        while ((cache.size() > CLEAR_UP_SIZE) && evictTime > 2000)
        {
            for (auto it = cache.cbegin(); it != cache.cend();)
            {
                if (it->second.evict || (CoreUtils::msElapsed(it->second.lastSeen, msSinceBoot) > evictTime))
                {
                    it = cache.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            evictTime -= 1000;
        }
    }

    void dump()
    {
        auto sinceBoot = CoreUtils::msSinceBoot();
        for (const auto &entry : cache)
        {
            const auto &data = entry.second; // Access the value part of the pair
            printf("%06X %02X %06d %s gnssAltitude: %d\n", data.icao, data.messageStatus, CoreUtils::msElapsed(data.lastSeen, sinceBoot), data.icaoAddress.c_str(), data.gnsAltitude);
        }
    }

    size_t size() const
    {
        return cache.size();
    }

    inline AdsbCombinedDataStatus &current()
    {
        return *currentDataStatus;
    }

    inline void updateAltitude(int32_t altitude)
    {
        currentDataStatus->messageStatus |= HAS_ALTITUDE;
        currentDataStatus->gnsAltitude = altitude + currentDataStatus->baro_gnss_diff;
    }

    inline void updateGnssAltitude(int32_t altitude)
    {
        currentDataStatus->messageStatus |= HAS_ALTITUDE;
        currentDataStatus->gnsAltitude = altitude;
    }

    inline void updateIcaoAddress(const OpenAce::IcaoAddress &icaoAddress, uint8_t aircraft_type)
    {
        if (!(currentDataStatus->messageStatus & CHECK_HAS_ICAOADDRESS))
        {
            currentDataStatus->messageStatus |= CHECK_HAS_ICAOADDRESS;
            currentDataStatus->icaoAddress = icaoAddress;
            currentDataStatus->category = aircraft_type;
        }
    }

    inline void updateRawOdd(uint32_t raw_latitude, uint32_t raw_longitude)
    {
        currentDataStatus->messageStatus |= (HAS_POSITION_ODD);
        currentDataStatus->raw_odd_latitude = raw_latitude;
        currentDataStatus->raw_odd_longitude = raw_longitude;
        decodePCR(true);
    }

    inline void updateRawEven(uint32_t raw_latitude, uint32_t raw_longitude)
    {
        currentDataStatus->messageStatus |= (HAS_POSITION_EVEN);
        currentDataStatus->raw_even_latitude = raw_latitude;
        currentDataStatus->raw_even_longitude = raw_longitude;
        decodePCR(false);
    }

    inline void updateAirborne(bool airborne)
    {
        currentDataStatus->airborne = airborne;
    }

    inline void updateHeading(int16_t heading)
    {
        currentDataStatus->messageStatus |= HAS_HEADING;
        currentDataStatus->heading = heading;
    }
    inline void updateVelocityHeadingBaroDiff(uint16_t velocity, int16_t vert_rate,uint8_t vert_rate_sign, int16_t heading, int16_t baro_gnss_diff)
    {
        currentDataStatus->messageStatus |= HAS_HEADING;
        currentDataStatus->messageStatus |= HAS_VELOCITY;
        currentDataStatus->velocity = velocity;
        currentDataStatus->vert_rate = vert_rate;
        currentDataStatus->vert_rate_sign = vert_rate_sign;
        currentDataStatus->heading = heading;
        currentDataStatus->baro_gnss_diff = baro_gnss_diff;
    }

    inline bool positionUpdatedAndValid()
    {
        if ((currentDataStatus->messageStatus & VALID_MASK) == VALID_MASK)
        {
            currentDataStatus->messageStatus &= ~HAS_POSITION_UPDATED;
            return true;
        }

        return false;
    }
};
