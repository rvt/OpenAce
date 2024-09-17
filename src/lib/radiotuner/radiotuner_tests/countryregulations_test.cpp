
#include <catch2/catch_test_macros.hpp>

#include "../ace/countryregulations.hpp"
#include "pico/rand.h"
#include "pico/time.h"

auto countryRegulations = CountryRegulations{};

TEST_CASE ( "zone", "[single-file]" )
{
    REQUIRE ( CountryRegulations::ZONE1 == countryRegulations.zone(51, 4));
    REQUIRE ( CountryRegulations::ZONE4 == countryRegulations.zone(51, 120));
    REQUIRE ( CountryRegulations::ZONE1 == countryRegulations.zone(0, 0));
}

TEST_CASE ( "nextProtocolTimeslot", "[single-file]" )
{
    uint8_t idx;
    idx = countryRegulations.nextProtocolTimeslot(100, CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE( 400  == countryRegulations.protocolTimeslotById(idx).slotStartTime );

    idx = countryRegulations.nextProtocolTimeslot(500, CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE( 800  == countryRegulations.protocolTimeslotById(idx).slotStartTime );

    idx = countryRegulations.nextProtocolTimeslot(900, CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE( 400  == countryRegulations.protocolTimeslotById(idx).slotStartTime );

    idx = countryRegulations.nextProtocolTimeslot(300, CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE( 400  == countryRegulations.protocolTimeslotById(idx).slotStartTime );

    idx = countryRegulations.nextProtocolTimeslot(1000, CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE( 400  == countryRegulations.protocolTimeslotById(idx).slotStartTime );

    idx = countryRegulations.nextProtocolTimeslot(0, CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE( 400  == countryRegulations.protocolTimeslotById(idx).slotStartTime );

    // Unconfigured ZONE/Datasource
    idx = countryRegulations.nextProtocolTimeslot(0, CountryRegulations::Zone::ZONE2, OpenAce::DataSource::FLARM);
    REQUIRE( CountryRegulations::Zone::ZONE0  == countryRegulations.protocolTimeslotById(idx).zone );
}

TEST_CASE ( "findFittingTimeslot", "[single-file]" )
{
    uint8_t idx;

    idx = countryRegulations.nextProtocolTimeslot(500, CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE( 0  != countryRegulations.findFittingTimeslot(500, idx) );
    REQUIRE( 0  != countryRegulations.findFittingTimeslot(900, idx) );
    REQUIRE( 0  != countryRegulations.findFittingTimeslot(100, idx) );
    REQUIRE( 0  == countryRegulations.findFittingTimeslot(300, idx) );

}

TEST_CASE ( "findFittingTimeSlot", "[single-file]" )
{
    uint8_t idx;

    idx = countryRegulations.nextProtocolTimeslot(500, CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE( 0  != countryRegulations.findFittingTimeslot(500, idx) );
    REQUIRE( 0  != countryRegulations.findFittingTimeslot(900, idx) );
    REQUIRE( 0  != countryRegulations.findFittingTimeslot(100, idx) );
    REQUIRE( 0  == countryRegulations.findFittingTimeslot(300, idx) );
}

TEST_CASE ( "protocolTimeslotById", "[single-file]" )
{
    uint8_t idx;

    idx = countryRegulations.getFirstSlotIdx(CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE( 1  == countryRegulations.protocolTimeslotById(idx).idx );
}

TEST_CASE ( "getFirstSlotIdx", "[single-file]" )
{
    uint8_t idx;

    idx = countryRegulations.getFirstSlotIdx(CountryRegulations::Zone::ZONE1, OpenAce::DataSource::FLARM);
    REQUIRE ( idx+1 == countryRegulations.nextSlotIdx(CountryRegulations::Zone::ZONE1, idx) );
    // FLARM has two slots for ZONE1, so should warm over
    REQUIRE ( idx == countryRegulations.nextSlotIdx(CountryRegulations::Zone::ZONE1, idx+1) );
}