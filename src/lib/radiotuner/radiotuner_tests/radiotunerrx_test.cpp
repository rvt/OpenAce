
#include <catch2/catch_test_macros.hpp>

#define private public

#include "../ace/radiotunerrx.hpp"
#include "ace/messagerouter.hpp"
#include "ace/messages.hpp"
#include "pico/time.h"
#include "etl/vector.h"

using RadioProtocolCtx = RadioTunerRx::RadioProtocolCtx;
OpenAce::ThreadSafeBus<50> bus;
OpenAce::OwnshipPositionMsg ownshipPosition{};

TEST_CASE("RadioProtocolCtx", "[single-file]")
{
    RadioProtocolCtx ctx{nullptr, nullptr};

    etl::array<uint8_t, (uint8_t)OpenAce::DataSource::_MAXRADIO> slotReceived = {};

    ctx.updateDataSources(etl::vector<OpenAce::DataSource, 1> {});
    ctx.prioritizeDatasources();
    REQUIRE(etl::vector<OpenAce::DataSource, 1> {} == ctx.dataSourceTimeSlots);

    SECTION("Prioritice should not crash", "[single-file]")
    {
        ctx.advanceReceiveSlot(CountryRegulations::ZONE0);
        REQUIRE(*ctx.upcomingDataSource == OpenAce::DataSource::FLARM);
    }
    SECTION("extra FLARM Data Source", "[single-file]")
    {
        ctx.updateDataSources(etl::vector{OpenAce::DataSource::FLARM});
        ctx.prioritizeDatasources();
        REQUIRE(etl::vector{OpenAce::DataSource::FLARM} == ctx.dataSourceTimeSlots);
    }

    SECTION("3 Data Source s", "[single-file]")
    {
        ctx.updateDataSources(etl::vector{OpenAce::DataSource::FLARM, OpenAce::DataSource::OGN1, OpenAce::DataSource::ADSL});
        ctx.prioritizeDatasources();
        REQUIRE(etl::vector{OpenAce::DataSource::FLARM, OpenAce::DataSource::OGN1, OpenAce::DataSource::ADSL} == ctx.dataSourceTimeSlots);

        SECTION("FLARM Data Received", "[single-file]")
        {
            slotReceived[(uint8_t)(OpenAce::DataSource::FLARM)]++;
            ctx.updateSlotReceive(slotReceived);
            ctx.prioritizeDatasources();
            REQUIRE(etl::vector{OpenAce::DataSource::FLARM, OpenAce::DataSource::FLARM, OpenAce::DataSource::OGN1, OpenAce::DataSource::ADSL} == ctx.dataSourceTimeSlots);

            SECTION("OGN and FLARM Data Received", "[single-file]")
            {
                slotReceived[(uint8_t)(OpenAce::DataSource::FLARM)]++;
                slotReceived[(uint8_t)(OpenAce::DataSource::OGN1)]++;
                ctx.updateSlotReceive(slotReceived);
                ctx.prioritizeDatasources();
                REQUIRE(etl::vector{OpenAce::DataSource::FLARM, OpenAce::DataSource::FLARM, OpenAce::DataSource::OGN1, OpenAce::DataSource::OGN1, OpenAce::DataSource::ADSL} == ctx.dataSourceTimeSlots);
                REQUIRE(*ctx.upcomingDataSource == OpenAce::DataSource::FLARM);

                SECTION("Should be circular receive slots", "[single-file]")
                {
                    ctx.advanceReceiveSlot(CountryRegulations::Zone::ZONE1);
                    REQUIRE(*ctx.upcomingDataSource == OpenAce::DataSource::FLARM);
                    ctx.advanceReceiveSlot(CountryRegulations::Zone::ZONE1);
                    REQUIRE(*ctx.upcomingDataSource == OpenAce::DataSource::OGN1);
                    ctx.advanceReceiveSlot(CountryRegulations::Zone::ZONE1);
                    REQUIRE(*ctx.upcomingDataSource == OpenAce::DataSource::OGN1);
                    ctx.advanceReceiveSlot(CountryRegulations::Zone::ZONE1);
                    REQUIRE(*ctx.upcomingDataSource == OpenAce::DataSource::ADSL);
                    ctx.advanceReceiveSlot(CountryRegulations::Zone::ZONE1);
                    // Here prioritsation should happen
                    REQUIRE(*ctx.upcomingDataSource == OpenAce::DataSource::FLARM);
                    ctx.advanceReceiveSlot(CountryRegulations::Zone::ZONE1);
                    REQUIRE(*ctx.upcomingDataSource == OpenAce::DataSource::FLARM);
                    ctx.advanceReceiveSlot(CountryRegulations::Zone::ZONE1);
                    REQUIRE(*ctx.upcomingDataSource == OpenAce::DataSource::OGN1);
                }
            }

            SECTION("OGN Data, no FLARM Received", "[single-file]")
            {
                slotReceived[(uint8_t)(OpenAce::DataSource::OGN1)]++;
                ctx.updateSlotReceive(slotReceived);
                ctx.prioritizeDatasources();
                REQUIRE(etl::vector{OpenAce::DataSource::FLARM, OpenAce::DataSource::OGN1, OpenAce::DataSource::OGN1, OpenAce::DataSource::ADSL} == ctx.dataSourceTimeSlots);

                SECTION("NO Data Received", "[single-file]")
                {
                    ctx.updateSlotReceive(slotReceived);
                    ctx.prioritizeDatasources();
                    REQUIRE(etl::vector{OpenAce::DataSource::FLARM, OpenAce::DataSource::OGN1, OpenAce::DataSource::ADSL} == ctx.dataSourceTimeSlots);
                }

                SECTION("Data sources removed", "[single-file]")
                {
                    ctx.updateDataSources(etl::vector<OpenAce::DataSource, 1> {});
                    ctx.prioritizeDatasources();
                    REQUIRE(etl::vector{etl::vector<OpenAce::DataSource, 1>{}} == ctx.dataSourceTimeSlots);
                }
            }
        }
    }
}
// TEST_CASE( "When ownship is received", "[single-file]" )
// {
//     auto radioController = RadioTunerRx{&bus};
//     radioController.numRadios = 2;
//     radioController.addRadioStructures();

//     // Should Update the current regulation
//     get_absolute_time_value = 100000;
//     ownshipPosition.position.lat = 51;
//     ownshipPosition.position.lon = 4;
//     radioController.on_receive(ownshipPosition);
//     REQUIRE( (radioController.currentZone == CountryRegulations::ZONE1) );

//     SECTION( "Should not update" )
//     {
//         get_absolute_time_value = 200000;

//         ownshipPosition.position.lat = 51;
//         ownshipPosition.position.lon = 170;
//         REQUIRE( (radioController.currentZone == CountryRegulations::ZONE1) );

//         SECTION( "should update to ZONE3 after time passed" )
//         {
//             get_absolute_time_value =300000;
//             for (int i=0; i<100; i++)
//             {
//                 radioController.on_receive(ownshipPosition);
//             }
//             REQUIRE( (radioController.currentZone == CountryRegulations::Zone::ZONE3) );
//         }
//     }
// }

// TEST_CASE( "Should find least occupied radio", "[single-file]" )
// {
//     auto radioController = RadioTunerRx{&bus};
//     radioController.numRadios = 2;
//     radioController.addRadioStructures();

//     REQUIRE( (radioController.leastOccupiedRadio() == 0) );
//     radioController.startListen(OpenAce::DataSource::OGN1, 0);
//     REQUIRE( (radioController.leastOccupiedRadio() == 1) );
//     radioController.startListen(OpenAce::DataSource::FLARM, 1);
//     REQUIRE( (radioController.leastOccupiedRadio() == 0) );
//     radioController.startListen(OpenAce::DataSource::PAW, 0);
//     REQUIRE( (radioController.leastOccupiedRadio() == 1) );

//     SECTION( "When one radio" )
//     {
//         auto radioController = RadioTunerRx{&bus};
//         radioController.startListen(OpenAce::DataSource::OGN1);
//         REQUIRE( (radioController.leastOccupiedRadio() == 0) );
//     }
// }

// TEST_CASE( "Listen on datasource", "[single-file]" )
// {
//     auto radioController = RadioTunerRx{&bus};
//     radioController.numRadios = 2;
//     radioController.addRadioStructures();

//     REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::OGN1) == 0) );
//     REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::FLARM) == 0) );
//     radioController.startListen(OpenAce::DataSource::OGN1);
//     REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::FLARM) == 0) );
//     radioController.startListen(OpenAce::DataSource::FLARM);
//     REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::OGN1) == 1) );
//     REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::FLARM) == 1) );

//     SECTION( "When stop listening" )
//     {
//         auto radioController = RadioTunerRx{&bus};
//         radioController.numRadios = 2;
//         radioController.addRadioStructures();

//         radioController.startListen(OpenAce::DataSource::OGN1);
//         radioController.startListen(OpenAce::DataSource::FLARM);
//         REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::OGN1) == 1) );
//         REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::FLARM) == 1) );
//         radioController.stopListen(OpenAce::DataSource::OGN1);
//         REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::OGN1) == 0) );
//         REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::FLARM) == 1) );
//     }

//     SECTION( "When startListen is done twice" )
//     {
//         auto radioController = RadioTunerRx{&bus};
//         radioController.numRadios = 2;
//         radioController.addRadioStructures();

//         radioController.startListen(OpenAce::DataSource::OGN1);
//         radioController.startListen(OpenAce::DataSource::OGN1);
//         REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::OGN1) == 1) );

//         radioController.startListen(OpenAce::DataSource::OGN1, -2);
//         REQUIRE( (radioController.numberOfSlots(OpenAce::DataSource::OGN1) == 1) );
//     }
// }

// TEST_CASE ("Timer slots", "[single-file]")
// {
//     auto radioController = RadioTunerRx{&bus};
//     radioController.numRadios = 2;
//     radioController.addRadioStructures();
//     radioController.startListen(OpenAce::DataSource::OGN1);
// //    printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str());
//     REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str(), "l1c0:O") == 0 ) );

//     radioController.startListen(OpenAce::DataSource::FLARM);
//     radioController.startListen(OpenAce::DataSource::OGN1);
//     // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str());
//     // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str());
//     REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str(), "l1c0:O") == 0 ) );
//     REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str(), "l1c0:F") == 0 ) );

//     radioController.startListen(OpenAce::DataSource::ADSL);
//     // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str());
//     // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str());
//     REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str(), "l2c0:OA") == 0 ) );
//     REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str(), "l1c0:F") == 0 ) );

//     SECTION( "When OGN Datasource Received", "[single-file]")
//     {
//         OpenAce::AircraftPositionInfo position= {};
//         position.dataSource = OpenAce::DataSource::OGN1;
//         radioController.on_receive(OpenAce::AircraftPositionMsg{position});
//         radioController.prioritizeDatasources();
//         // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str());
//         // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str());
//         REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str(), "l3c0:OOA") == 0 ) );
//         REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str(), "l1c0:F") == 0 ) );

//         SECTION( "When only FLARM is received Received and not OGN", "[single-file]")
//         {
//             position.dataSource = OpenAce::DataSource::FLARM;
//             radioController.on_receive(OpenAce::AircraftPositionMsg{position});
//             radioController.prioritizeDatasources();
//             // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str());
//             // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str());
//             REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str(), "l2c0:OA") == 0 ) );
//             REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str(), "l2c0:FF") == 0 ) );

//             // Nothing received anymore
//             radioController.prioritizeDatasources();
//             // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str());
//             // printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str());
//             REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str(), "l2c0:OA") == 0 ) );
//             REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(1)).c_str(), "l1c0:F") == 0 ) );
//         }

//         SECTION( "When in middle of cycle, should not prioritize datasources", "[single-file]")
//         {
//             REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str(), "l3c0:OOA") == 0 ) );
//             position.dataSource = OpenAce::DataSource::OGN1;
//             radioController.on_receive(OpenAce::AircraftPositionMsg{position});
//             radioController.radioTasks.at(0).slotIdx = 1;
//             radioController.prioritizeDatasources();
//             printf("%s-\n", radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str());
//             REQUIRE( (strcmp(radioController.visualizeTimeSlots(radioController.radioTasks.at(0)).c_str(), "l3c1:OOA") == 0 ) );
//         }
//     }

// }
