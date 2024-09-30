
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "pico/rand.h"
#include "pico/time.h"
#include "coreutils.hpp"


TEST_CASE( "msSinceEpoch", "[single-file]" )
{
    time_us_64Value = 0;
    CoreUtils::setOffsetMsSinceEpoch(0);
    time_us_64Value = 1000000;
    REQUIRE( (CoreUtils::msSinceEpoch() == 1000) );

    time_us_64Value = 1200'000;
    CoreUtils::setOffsetMsSinceEpoch(1210);
    REQUIRE( (CoreUtils::msSinceEpoch() == 1210) );

    time_us_64Value = 2500'000;
    REQUIRE( (CoreUtils::msSinceEpoch() == 2510) );
}


TEST_CASE( "msInSecond", "[single-file]" )
{
    time_us_64Value = 0;
    CoreUtils::setOffsetMsSinceEpoch(1698800584411);
    REQUIRE( (CoreUtils::msInSecond() == 411) );

    CoreUtils::setOffsetMsSinceEpoch(1698800584000);
    REQUIRE( (CoreUtils::msInSecond() == 0) );
}


TEST_CASE( "msDelayToReference", "[single-file]" )
{
    time_us_64Value = 0;
    REQUIRE( (CoreUtils::msDelayToReference(411, 123) == 288) ); // 123ms into whole second

    REQUIRE( (CoreUtils::msDelayToReference(123, 411) == 712) );  // 411ms into whole second

    CoreUtils::setOffsetMsSinceEpoch(1698800584010); // 10ms into whole second
    REQUIRE( (CoreUtils::msDelayToReference(123) == 113) );
}


TEST_CASE( "secondsSinceEpoch", "[single-file]" )
{
    time_us_64Value = 0;
    CoreUtils::setOffsetMsSinceEpoch(1698800584010);
    REQUIRE( (CoreUtils::secondsSinceEpoch() == 1698800584) );

    // ms is always round down by design so 999ms in second is still considered teh previous second
    CoreUtils::setOffsetMsSinceEpoch(1698800584510);
    REQUIRE( (CoreUtils::secondsSinceEpoch() == 1698800584) );
}

TEST_CASE( "distanceAccurate", "[single-file]" )
{
    using namespace Catch::literals;
    time_us_64Value = 0;
    REQUIRE( CoreUtils::distanceAccurate(54, 4, 54, 4) == 0 );

    REQUIRE( CoreUtils::distanceAccurate(0, 4, 0, 5) == 111206_a );
    REQUIRE( CoreUtils::distanceAccurate(1, 4, 0, 4) == 111206_a );

    Catch::Approx target = Catch::Approx(23370).margin(18);
    for (int lat = -50; lat <= 50; lat=lat + 100)
    {
        for (int lon = -4; lon <= 4; lon=lon + 8)
        {
            REQUIRE( CoreUtils::distanceAccurate(lat+0.1, lon + .6, lat+.3, lon+.7) == target );
        }
    }
}

TEST_CASE( "distanceFast", "[single-file]" )
{
    using namespace Catch::literals;
    time_us_64Value = 0;
    REQUIRE( CoreUtils::distanceFast(54, 4, 54, 4) == 0 );

    REQUIRE( CoreUtils::distanceFast(0, 4, 0, 5) == 111321_a );
    REQUIRE( CoreUtils::distanceFast(1, 4, 0, 4) == 111139_a );

    Catch::Approx target = Catch::Approx(23370).margin(24);
    for (int lat = -50; lat <= 50; lat=lat + 100)
    {
        for (int lon = -4; lon <= 4; lon=lon + 8)
        {
            REQUIRE( CoreUtils::distanceFast(lat+0.1, lon + .6, lat+.3, lon+.7) == target );
        }
    }
}



TEST_CASE( "bearingFromInRad", "[single-file]" )
{
    using namespace Catch::literals;
    time_us_64Value = 0;

    int lat, lon;

    lat=50;
    lon=4;
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat+1, lon+1) * RADS_TO_DEG == 32.07481_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat-1, lon+1) * RADS_TO_DEG == 146.6172_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat-1, lon-1) * RADS_TO_DEG == 213.3828_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat+1, lon-1) * RADS_TO_DEG == 327.9252_a );

    lat=-50;
    lon=4;
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat+1, lon+1) * RADS_TO_DEG == 33.38282_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat-1, lon+1) * RADS_TO_DEG == 147.9252_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat-1, lon-1) * RADS_TO_DEG == 212.0748_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat+1, lon-1) * RADS_TO_DEG == 326.6171_a );

    lat=50;
    lon=-4;
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat+1, lon+1) * RADS_TO_DEG == 32.07481_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat-1, lon+1) * RADS_TO_DEG == 146.6172_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat-1, lon-1) * RADS_TO_DEG == 213.3828_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat+1, lon-1) * RADS_TO_DEG == 327.9252_a );

    lat=-50;
    lon=-4;
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat+1, lon+1) * RADS_TO_DEG == 33.38282_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat-1, lon+1) * RADS_TO_DEG == 147.9252_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat-1, lon-1) * RADS_TO_DEG == 212.0748_a );
    REQUIRE( CoreUtils::bearingFromInRad( lat,  lon, lat+1, lon-1) * RADS_TO_DEG == 326.6171_a );
}

TEST_CASE( "getDistanceRelNorthRelEastFloat", "[single-file]" )
{
    using namespace Catch::literals;
    time_us_64Value = 0;

    int lat, lon;

    lat=50;
    lon=4;
    auto result = CoreUtils::getDistanceRelNorthRelEastFloat( lat,  lon, lat+0.1, lon+0.1);
    REQUIRE( result.relNorthMeter == 11127.22363_a );
    REQUIRE( result.relEastMeter == 7134.56885_a );

    lat=50;
    lon=4;
    result = CoreUtils::getDistanceRelNorthRelEastFloat( lat,  lon, lat-0.1, lon-0.1);
    REQUIRE( result.relNorthMeter == -11110.86914_a );
    REQUIRE( result.relEastMeter == -7160.01123_a );
}

TEST_CASE( "getRadialSection", "[single-file]" )
{
    REQUIRE( CoreUtils::getRadialSection<8>(10) == 0 );
    REQUIRE( CoreUtils::getRadialSection<8>(180) == 4 );
    REQUIRE( CoreUtils::getRadialSection<8>(179) == 4 );
    REQUIRE( CoreUtils::getRadialSection<8>(181) == 4 );
}

TEST_CASE( "getPath", "[single-file]" )
{
    etl::string<32> pathToParse = "/foo/bar/bash.sh";
    auto path = CoreUtils::parsePath(pathToParse);
    REQUIRE( path.size() == 4 );
    REQUIRE( path.back() == "sh" );
    REQUIRE( path[0] == "foo" );
    REQUIRE( path[1] == "bar" );
    REQUIRE( path[2] == "bash" );
}

TEST_CASE( "addChecksumToNMEA", "[single-file]" )
{
    // Empty string stays empty
    OpenAce::NMEAString pflau="$";
    CoreUtils::addChecksumToNMEA(pflau);
    REQUIRE( (pflau.size() == 6 ) );
    REQUIRE( (pflau.compare("$*00")) );

    // Should start from 0
    pflau="1234";
    CoreUtils::addChecksumToNMEA(pflau);
    REQUIRE( (pflau.size() == 9 ) );
    REQUIRE( (pflau.compare("1234*35\r\n") == 0 ) );

    // Add checksum
    pflau="$1234";
    CoreUtils::addChecksumToNMEA(pflau);
    REQUIRE( (pflau.size() == 10 ) );
    REQUIRE( (pflau.compare("$1234*04\r\n") == 0) );

    // Replace existing checksum
    pflau="$1234*35";
    CoreUtils::addChecksumToNMEA(pflau);
    REQUIRE( (pflau.size() == 10 ) );
    REQUIRE( (pflau.compare("$1234*04\r\n") == 0) );
}

