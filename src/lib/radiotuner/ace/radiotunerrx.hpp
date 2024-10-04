#pragma once

#include <stdint.h>

#include "ace/constants.hpp"
#include "ace/messagerouter.hpp"
#include "ace/basemodule.hpp"
#include "ace/messages.hpp"
#include "ace/coreutils.hpp"
#include "ace/models.hpp"

#include "countryregulations.hpp"

#include "etl/map.h"
#include "etl/message_bus.h"
#include "etl/list.h"
#include "etl/array.h"
#include <etl/array_view.h>
#include "etl/string.h"
#include "etl/bitset.h"
#include "etl/circular_iterator.h"

#include "timers.h"

// When set, also set's the PICO's rtc

/**
 * This class is responsible for controlling the radio's
 */
/**
 * Note: Currently using array's indexed on datasource to give fast lookups on statistics but will cost a bit more memory
 * The downside is that only one protocol can be configured on one radio so listening on the same protocol on two radios would not be possible
 */
class RadioTunerRx : public BaseModule, public etl::message_router<RadioTunerRx, OpenAce::OwnshipPositionMsg, OpenAce::AircraftPositionMsg, OpenAce::ConfigUpdatedMsg>
{
    using  SlotReceive = etl::array<uint8_t, static_cast<uint8_t>(OpenAce::DataSource::_ITEMS)>;
public:
    static constexpr const uint32_t UPDATE_ZONE_REGULATION_EVERY = 30000; // Get new regulatory dataset every XXms
    static constexpr const size_t   MAX_SLOTS_PER_SOURCE = 3;             // Maximum slots a datasource can get while receiving
    static constexpr const size_t   MAX_SOURCE_PER_RADIO = 3;             // Maximum source per radio, eg maximum protocols it will search for
    static constexpr const size_t   TIME_SLOT_SIZE = MAX_SLOTS_PER_SOURCE * MAX_SLOTS_PER_SOURCE;

private:
    // Each radio will get one task Context to handle
    struct RadioProtocolCtx
    {
        mutable struct
        {
            uint32_t rxRequests = 0;
            uint32_t timerMissed = 0;
        } statistics;

        // Pointers needed to control the radio
        RadioTunerRx *controller;
        Radio *radio;
        TaskHandle_t taskHandle;
        TimerHandle_t timerHandle;

        // The DataSources this radio will handle
        etl::vector<OpenAce::DataSource, MAX_SOURCE_PER_RADIO> dataSources;

        // datasources processed by this task and used by prioritizeDatasources
        using DataSources = etl::vector<OpenAce::DataSource, TIME_SLOT_SIZE>;
        using CircularDataSourceIterator = etl::circular_iterator<DataSources::iterator>;
        DataSources dataSourceTimeSlots;
        CircularDataSourceIterator upcomingDataSource;

        // Data of the upcoming timeslot set by advanceReceiveSlot
        uint8_t upcomingTimeslot = CountryRegulations::NONE_DATASOURCE.idx;

        SlotReceive slotReceive = {};
        SlotReceive previousReceived = {};

        // Constructor
        RadioProtocolCtx(RadioTunerRx *controller_, Radio *radio_) : controller(controller_), radio(radio_), taskHandle(nullptr), timerHandle(nullptr)
        {
            prioritizeDatasources();
        }

        // Ensure we never copy
        RadioProtocolCtx(const RadioProtocolCtx &) = delete;
        RadioProtocolCtx &operator=(RadioProtocolCtx const &) = delete;

        void getData(etl::string_stream &stream) const
        {

            stream << "\"radio_" << radio->radio() << "\":[";
            for (auto it = dataSourceTimeSlots.cbegin(); it != dataSourceTimeSlots.cend(); ++it)
            {
                stream << "\"" << OpenAce::dataSourceToString(*it) << "\"";
                if (std::next(it) != dataSourceTimeSlots.cend())
                {
                    stream << ",";
                }
            }
            stream << "]";
            stream << ",\"nextRx_" << radio->radio() << "\":" << "\"" << OpenAce::dataSourceToString(*upcomingDataSource) << "\"";
            stream << ",\"timerMissedRadio_" << radio->radio() << "\":" << statistics.timerMissed;
            stream << ",\"rxRequestsRadio_" << radio->radio() << "\":" << statistics.rxRequests;
        }

        /**
         * Update the receive counts, this works as an incremental counter (8 bit) where the differences between the last received and the current
         * will be calculate to know if any data was received
         */
        void updateSlotReceive(const SlotReceive &sr)
        {
            // Update slotReceive with the difference between sr and previousReceived
            for (size_t i = 0; i < slotReceive.size(); ++i)
            {
                // Subtract the previous value from the new value to get the difference
                slotReceive[i] = sr[i] - previousReceived[i];
            }
            previousReceived = sr;
        }

        /**
         * Set the datasources to be received
         */
        void updateDataSources(const etl::ivector<OpenAce::DataSource> &ds)
        {
            dataSources.clear();
            dataSources.assign(ds.cbegin(), ds.cend());
            prioritizeDatasources();
        }

        /**
         * Sets the next receive slot based on the current time and the specified zone.
         *
         * This method advances the iterator `upcomingDataSource` to point to the next data source
         * in the `dataSourceTimeSlots` vector. If the iterator reaches the beginning of the vector,
         * it calls `prioritizeDatasources()` to regenerate the dataSourceTimeSlots.
         *
         * After selecting the next data source, the method calculates the appropriate time slot
         * for that data source using the provided zone and the current time.
         *
         * @param zone The geographical zone that influences the timing regulations.
         * @return The delay in milliseconds until the next protocol timeslot starts, minimum delay is 1 ms
         */

        int16_t advanceReceiveSlot(CountryRegulations::Zone zone)
        {
            // Can we do better than this??
            if (dataSources.empty())
            {
                upcomingTimeslot = CountryRegulations::NONE_DATASOURCE.idx;
                return 500;
            }

            // Once full circle, prioritizeDatasources to see if priority needs to be re evaluated
            if (upcomingDataSource == dataSourceTimeSlots.begin())
            {
                prioritizeDatasources();
            }

            auto currentMs = CoreUtils::msInSecond();
            upcomingTimeslot = CountryRegulations::nextProtocolTimeslot(currentMs, zone, *upcomingDataSource);
            ++upcomingDataSource;
            const auto &next = CountryRegulations::protocolTimeslotById(upcomingTimeslot);
            return CoreUtils::msDelayToReference(next.slotStartTime, currentMs) + 1;
        }

        /**
         * Prioritizes and organizes data sources based on their slot receive configuration.
         *
         * This method processes the `dataSources` vector and populates the `dataSourceTimeSlots`
         * vector based on the priority specified in the `slotReceive` array. Each `DataSource`
         * in `dataSources` is added to `dataSourceTimeSlots` one or more times, depending on
         * its corresponding value in `slotReceive`.
         *
         * - If `slotReceive` value is 0: The `DataSource` is added once.
         * - If `slotReceive` value is 1: The `DataSource` is added twice.
         * - If `slotReceive` value is 2 or more: The `DataSource` is added three times.
         *
         * After constructing the `dataSourceTimeSlots`, the method re-initializes the circular
         * iterator `upcomingDataSource` to iterate over the updated `dataSourceTimeSlots` vector.
         */
        void prioritizeDatasources()
        {
            // Clear the dataSourceTimeSlots vector to start fresh
            dataSourceTimeSlots.clear();

            for (const auto &ds : dataSources)
            {
                uint8_t slotCount;

                // Check the slotReceive array to see if more entries are needed
                if (slotReceive[(uint8_t)ds] == 0)
                {
                    slotCount = 1;
                }
                else if (slotReceive[(uint8_t)ds] == 1)
                {
                    slotCount = 2;
                }
                else
                {
                    slotCount = MAX_SLOTS_PER_SOURCE;
                }

                dataSourceTimeSlots.insert(dataSourceTimeSlots.end(), slotCount, ds);
            }

            // Re-initialize the circular iterator after modifying the dataSourceTimeSlots
            upcomingDataSource = CircularDataSourceIterator{dataSourceTimeSlots.begin(), dataSourceTimeSlots.end()};
        }
    };

    // Keep track if there is any traffic on the datasources
    SlotReceive slotReceive;

    // Keep track of one task per each radio
    etl::list<RadioProtocolCtx, OPEN_ACE_MAX_RADIOS> radioTasks;

    enum TaskState : uint32_t
    {
        EXIT = 1 << 0,
        TIMER = 1 << 1,
        BLOCK = 1 << 2,
        UNBLOCK = 1 << 3,
    };

private:
    friend class message_router;

    // Current zone we are flying in
    volatile CountryRegulations::Zone currentZone;


private:
    static void timerTuneCallback(TimerHandle_t xTimer);
    static void radioTuneTask(void *arg);

    // ******************** Message bus receive handlers ********************
    void on_receive(const OpenAce::OwnshipPositionMsg &msg);
    void on_receive(const OpenAce::AircraftPositionMsg &msg);
    void on_receive(const OpenAce::ConfigUpdatedMsg &msg);
    void on_receive_unknown(const etl::imessage &msg);

public:
    static constexpr const etl::string_view NAME = "RadioTunerRx";

    RadioTunerRx(etl::imessage_bus &bus, const Configuration &config) : BaseModule(bus, NAME),
        currentZone(CountryRegulations::Zone::ZONE0)
    {
    }
    virtual ~RadioTunerRx() = default;

    virtual OpenAce::PostConstruct postConstruct() override;
    virtual void start() override;
    virtual void stop() override;
    virtual void getData(etl::string_stream &stream, const etl::string_view optional) const override;
    void addRadioTasks(uint8_t numRadios);

private:
    void enableDisableDatasources(const etl::ivector<OpenAce::DataSource> &datasources);
};
