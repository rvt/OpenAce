
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "math.h"
#include <fstream>
#include <filesystem>
#include <stdio.h>
#include "mockutils.h"
#include "mockconfig.h"
#include "ace/coreutils.hpp"

#define private public

#include "adsbdecoder.hpp"

class Test : public etl::message_router<Test, OpenAce::AircraftPositionMsg>
{

public:
    OpenAce::AircraftPositionInfo position;
    etl::imessage_bus *bus;
    bool received = false;
    Test(etl::imessage_bus  *bus_) : bus(bus_)
    {
        bus->subscribe(*this);
    }
    ~Test()
    {
        bus->unsubscribe(*this);
    }

    void on_receive(const OpenAce::AircraftPositionMsg &msg)
    {
        // printf("AircraftPosition Received\n");
        received = true;
        position = msg.position;
    }
    void on_receive_unknown(const etl::imessage& msg)
    {
    }
};

OpenAce::ThreadSafeBus<50> bus;
MockConfig mockConfig{bus};

// TEST_CASE( "ignoredAirplane", "[single-file]" )
// {
//     // test if ignoredAirplane is configured as a true set
//     ADSBDecoder adsbDecoder{bus, mockConfig};
//     adsbDecoder.ignoredAirplanes.insert(ADSBDecoder::IgnoredArplaneStatus{0x00234567, CoreUtils::msSinceBoot()});
//     adsbDecoder.ignoredAirplanes.insert(ADSBDecoder::IgnoredArplaneStatus{0x00234567, CoreUtils::msSinceBoot()});

//     REQUIRE( (adsbDecoder.ignoredAirplanes.size() == 1));
//     adsbDecoder.ignoredAirplanes.insert(ADSBDecoder::IgnoredArplaneStatus{0x00234568, CoreUtils::msSinceBoot()});

//     REQUIRE( (adsbDecoder.ignoredAirplanes.size() == 2));
//     adsbDecoder.ignoredAirplanes.erase(ADSBDecoder::IgnoredArplaneStatus{0x00234567});
//     adsbDecoder.ignoredAirplanes.erase(ADSBDecoder::IgnoredArplaneStatus{0x00234568});
//     REQUIRE( (adsbDecoder.ignoredAirplanes.size() == 0));
// }

// TEST_CASE( "Test filter below and above", "[single-file]" )
// {
//     ADSBDecoder adsbDecoder{&bus};
//     adsbDecoder.OpenAce::PostConstruct();
//     Test test{&bus};

//     namespace fs = std::filesystem;
//     std::string filename = "../adsb.txt";
//     auto path = fs::current_path();
//     printf("->-> Path: %s\n", path.c_str());
//     std::ifstream infile(filename);
//     if (!infile.is_open())
//     {
//         printf("File not found %s\n", filename.c_str());
//         REQUIRE(( false ));
//     }

//     int higestPlane = 0;
//     int lowestPlane = 50000;

//     std::string line;
//     adsbDecoder.ownshipPosition.altitudeWgs84 = 10000;
//     adsbDecoder.filterAbove=50000;
//     adsbDecoder.filterBelow=50000;
//     while (std::getline(infile, line))
//     {
//         test.received = false;
//         get_absolute_timeValue++;
//         OpenAce::ADSBString data = line.c_str();
//         adsbDecoder.on_receive(OpenAce::ADSBMessage{data});

//         if (test.received == false)
//         {
//             continue;
//         }

//         REQUIRE( (test.position.address != 0) );

//         if (test.position.altitudeWgs84 == 0)
//         {
//         }

//         if (test.position.altitudeWgs84 > higestPlane)
//         {
//             higestPlane = test.position.altitudeWgs84;
//         }
//         if (test.position.altitudeWgs84 < lowestPlane)
//         {
//             lowestPlane = test.position.altitudeWgs84;
//         }
//     }
//     printf("Highest Plane: %d lowestPlane: %d\n", higestPlane, lowestPlane);

//     // Test Filtering planes above me
//     infile.clear();                 // clear fail and eof bits
//     infile.seekg(0, std::ios::beg); // back to the start!
//     adsbDecoder.filterAbove=1000;
//     adsbDecoder.filterBelow=1000;
//     adsbDecoder.ownshipPosition.altitudeWgs84 = lowestPlane-adsbDecoder.filterAbove;
//     get_absolute_timeValue += 1000000;
//     int totalPlanes = 0;
//     while (std::getline(infile, line))
//     {
//         test.received = false;
//         get_absolute_timeValue++;
//         OpenAce::ADSBString data = line.c_str();
//         adsbDecoder.on_receive(OpenAce::ADSBMessage{data});
//         if (test.received)
//         {
//             totalPlanes++;
//         }
//     }
//     printf("Total Planes above: %d\n", totalPlanes);
//     REQUIRE( (totalPlanes == 5) );

//     // Test Filtering planes below me
//     infile.clear();                 // clear fail and eof bits
//     infile.seekg(0, std::ios::beg); // back to the start!
//     adsbDecoder.filterAbove=1000;
//     adsbDecoder.filterBelow=1000;
//     adsbDecoder.ownshipPosition.altitudeWgs84 = higestPlane+adsbDecoder.filterBelow;
//     get_absolute_timeValue += 1000000;
//     totalPlanes = 0;
//     while (std::getline(infile, line))
//     {
//         test.received = false;
//         get_absolute_timeValue++;
//         OpenAce::ADSBString data = line.c_str();
//         adsbDecoder.on_receive(OpenAce::ADSBMessage{data});
//         if (test.received)
//         {
//             totalPlanes++;
//         }
//     }
//     printf("Total Planes below: %d\n", totalPlanes);
//     REQUIRE( (totalPlanes == 394) );
//     infile.close();
// }


TEST_CASE( "Test heading and direction received aircraft", "[single-file]" )
{
    ADSBDecoder adsbDecoder{bus, mockConfig};
    adsbDecoder.postConstruct();
    Test test{&bus};
    adsbDecoder.filterAbove=50000;
    adsbDecoder.filterBelow=50000;

    test.received = false;
    adsbDecoder.ownshipPosition.altitudeWgs84 = 10000;
    adsbDecoder.ownshipPosition.lat = 52.1;
    adsbDecoder.ownshipPosition.lon = 4.8;

    uint8_t data[14];
    hexStrToByteArray("8d502cd1589992ecbaf1a4140b65", data);
    adsbDecoder.receiveBinary(data, 14);
    hexStrToByteArray("8d502cd15899965802eb001f31d8", data);
    adsbDecoder.receiveBinary(data, 14);
    hexStrToByteArray("8d502cd19908c532903c9cced691", data);
    adsbDecoder.receiveBinary(data, 14);

    REQUIRE( test.received == true);
    REQUIRE( test.position.address == 0x502CD1);
    REQUIRE( test.position.addressType == OpenAce::AddressType::ICAO);
    REQUIRE( test.position.dataSource == OpenAce::DataSource::ADSB);
    REQUIRE( test.position.aircraftType == OpenAce::AircraftCategory::Unknown);
    REQUIRE( test.position.altitudeWgs84 == 9029 );
    REQUIRE( test.position.groundSpeed == Catch::Approx(230.98).margin(0.1)); // in m/s
    REQUIRE( test.position.course == 25 ); // ADSB data shows 25.94.. Should we use floats instead of int?
    REQUIRE( test.position.lat == Catch::Approx(52.3888).margin(0.005));
    REQUIRE( test.position.lon == Catch::Approx(4.7209).margin(0.005));
    REQUIRE( test.position.verticalSpeed == Catch::Approx(4.552).margin(0.001));

    REQUIRE( test.position.distanceFromOwn == Catch::Approx(32551).margin(1));
    REQUIRE( test.position.relNorthFromOwn == Catch::Approx(32107).margin(1));
    REQUIRE( test.position.relEastFromOwn == Catch::Approx(-5359).margin(1));
    REQUIRE( test.position.bearingFromOwn == Catch::Approx(351).margin(0.5));
}
