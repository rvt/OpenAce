#pragma once

#include "ace/coreutils.hpp"
#include "ace/constants.hpp"
#include "ace/models.hpp"

#include "etl/flat_set.h"

template <size_t SIZE, uint32_t EVICT_TIME_MS>
class AddressCache
{
    static constexpr uint32_t CLEAR_UP_SIZE = (SIZE * 90) / 100;

    struct AddressStatus
    {
        OpenAce::AircraftAddress icao;
        uint32_t lastSeen;
        AddressStatus(OpenAce::AircraftAddress icao_, uint32_t lastSeen_) : icao(icao_), lastSeen(lastSeen_)
        {
        }
        AddressStatus(const AddressStatus &other)
            : icao(other.icao), lastSeen(other.lastSeen)
        {
        }
        // copy assignment operator
        AddressStatus &operator=(const AddressStatus &other)
        {
            icao = other.icao;
            lastSeen = other.lastSeen;
            return *this;
        }
    };

    struct AddressComparator
    {
        constexpr bool operator()(const AddressStatus &lhs, const AddressStatus &rhs) const
        {
            return lhs.icao < rhs.icao;
        }
    };

    struct FindByIcao
    {
        FindByIcao(const OpenAce::AircraftAddress &icao) : icao(icao) {}
        bool operator()(const AddressStatus &i)
        {
            return i.icao == icao;
        }

    private:
        OpenAce::AircraftAddress icao;
    };

    etl::flat_set<AddressStatus, SIZE, AddressComparator> cache;

public:
    void clear() {
        cache.clear();
    }
    size_t size() const
    {
        return cache.size();
    }

    bool contains(uint32_t icao, uint32_t msSinceBoot)
    {
        auto it = etl::find_if(cache.begin(), cache.end(), FindByIcao(icao));

        if (it != cache.end())
        {
            it->lastSeen = msSinceBoot;
            return true;
        }

        return false;
    }

    bool insert(uint32_t address, uint32_t msSinceBoot)
    {
        if (cache.full())
        {
            evictOldEntries(msSinceBoot);
        }

        cache.insert(AddressStatus{address, msSinceBoot});
        return true;
    }

    void evictOldEntries(uint32_t msSinceBoot)
    {
        // Always ensure there is room for new cache entries be reducing evictTime untill there is room again
        auto evictTime = EVICT_TIME_MS;
        while ((cache.size() > CLEAR_UP_SIZE) && evictTime > 2000)
        {
            cache.erase(etl::remove_if(cache.begin(), cache.end(), [msSinceBoot, evictTime](const auto &it)
            {
                return CoreUtils::msElapsed(it.lastSeen, msSinceBoot) > evictTime;
            }),
            cache.end());
            evictTime -= 1000;
        }
    }
};
