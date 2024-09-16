#include <stdio.h>

#include "gpsdecoder.hpp"
#include "ace/moreutils.hpp"

OpenAce::PostConstruct GpsDecoder::postConstruct()
{
    return OpenAce::PostConstruct::OK;
}

void GpsDecoder::start()
{
    getBus().subscribe(*this);
}

void GpsDecoder::stop()
{
    getBus().unsubscribe(*this);
}

void GpsDecoder::getData(etl::string_stream &stream, const etl::string_view optional) const
{
    constexpr etl::format_spec width2fill0 = etl::format_spec().width(2).fill('0');
    const char *dopValue = OpenAce::DOPInterpretationToString(OpenAce::floatToDOPInterpretation(pDop));
    stream << "{";
    stream << "\"receivedGGA\":" << statistics.receivedGGA;
    stream << ",\"receivedRMC\":" << statistics.receivedRMC;
    stream << ",\"receivedGSA\":" << statistics.receivedGSA;
    stream << ",\"receivedOther\":" << statistics.receivedOther;
    stream << ",\"latitude\":" << etl::format_spec{}.precision(5) << latitude();
    stream << ",\"longitude\":" << longitude() << etl::format_spec{}.precision(1);
    stream << ",\"altitude\":" << altitude();
    stream << ",\"groundspeed\":" << groundSpeed();
    stream << ",\"track\":" << course();
    stream << ",\"pDop\":" << pDop << OpenAce::RESET_FORMAT;
    stream << ",\"dopValue\":\"" << dopValue << "\"";
    stream << ",\"fixQuality\":\"" << fixQuality << "\"";
    stream << ",\"satellitesTracked\":" << satellitesTracked;
    stream << ",\"upTime\":" << CoreUtils::msElapsed(statistics.startTime) / 1000; // ToDo, movr to some better location
    stream << ",\"gpstime\":" << "\""
           << width2fill0 << lastGGATimestamp.hours << OpenAce::RESET_FORMAT << ":"
           << width2fill0 << lastGGATimestamp.minutes << OpenAce::RESET_FORMAT << ":"
           << width2fill0 << lastGGATimestamp.seconds << OpenAce::RESET_FORMAT << "\"";
    stream << "}\n";
}

void GpsDecoder::on_receive(const OpenAce::GPSMessage &msg)
{
    // printf("GpsDecoder: %s\n", msg.sentence.c_str());
    static Every<int8_t, 30, 60> sendGpsTime{0};
    static Every<int8_t, 5, 60> sendValidGps{0};
    switch (minmea_sentence_id(msg.sentence.c_str(), false))
    {
    case MINMEA_SENTENCE_RMC:
    {
        statistics.receivedRMC++;
        struct minmea_sentence_rmc frame;
        if (minmea_parse_rmc(&frame, msg.sentence.c_str()))
        {
            uint16_t millis = frame.time.microseconds / 1000;
            // TODO: Be more intelligent on when sending time messages so a 'fix' is quicker known after (re)start of the whole system
            if (millis == 0 && sendGpsTime.isItTime(frame.time.seconds))
            {
                getBus().receive(
                    OpenAce::GpsTime
                {
                    static_cast<int16_t>(frame.date.year + 2000),
                    static_cast<int8_t>(frame.date.month),
                    static_cast<int8_t>(frame.date.day),
                    static_cast<int8_t>(frame.time.hours),
                    static_cast<int8_t>(frame.time.minutes),
                    static_cast<int8_t>(frame.time.seconds),
                    static_cast<int16_t>(millis)});
            }

            // Update planes position when fix is valid
            if (frame.valid)
            {
                float prevLatitude = latitude();
                float prevLongitude = longitude();

                latitude(minmea_tocoord(&frame.latitude));
                longitude(minmea_tocoord(&frame.longitude));

                auto const relNorthrelEast = CoreUtils::northEastDistance(prevLatitude, prevLongitude, latitude(), longitude());
                velocityNorth = relNorthrelEast.north;
                velocityEast = relNorthrelEast.east;

                groundSpeed(minmea_tofloat(&frame.speed));
                // Course might not always be done and will result in a inf values in the filter
                if (frame.course.scale != 0)
                {
                    course(minmea_tofloat(&frame.course));
                }
                lastRMCTimestamp = frame.time;
                sendMessageWhenGGAisRMC();
            }

            // Send a frame valid message
            if (sendValidGps.isItTime(frame.time.seconds))
            {
                getBus().receive(
                    OpenAce::GpsStatus{frame.valid});
            }
        }
    }
    break;

    case MINMEA_SENTENCE_GGA:
    {
        statistics.receivedGGA++;

        struct minmea_sentence_gga frame;
        if (minmea_parse_gga(&frame, msg.sentence.c_str()))
        {

            float height = convertToMeters(&frame.height, frame.height_units);

            if (height != INVALID_CONVERSION)
            {
                float alt = convertToMeters(&frame.altitude, frame.altitude_units);
                if (alt != INVALID_CONVERSION)
                {
                    alt += height;
                }
                altitude(alt);
            }
            lastGGATimestamp = frame.time;
            satellitesTracked = frame.satellites_tracked;
            // 0: Fix not valid
            // 1: GPS fix
            // 2: Differential GPS fix (DGNSS), SBAS, OmniSTAR VBS, Beacon, RTX in GVBS mode
            // 3: Not applicable
            // 4: RTK Fixed, xFill
            // 5: RTK Float, OmniSTAR XP/HP, Location RTK, RTX
            // 6: INS Dead reckoning
            fixQuality = frame.fix_quality;

            sendMessageWhenGGAisRMC();
        }
    }
    break;

    case MINMEA_SENTENCE_GSA:
    {
        statistics.receivedGSA++;
        struct minmea_sentence_gsa frame;
        if (minmea_parse_gsa(&frame, msg.sentence.c_str()))
        {
            pDop = minmea_tofloat(&frame.pdop);
            getBus().receive(
                OpenAce::GpsStatsMsg
            {
                fixQuality,
                (uint8_t)frame.fix_type,
                satellitesTracked,
                pDop,
                minmea_tofloat(&frame.hdop)});
        }
    }
    break;

    default:
    {
        statistics.receivedOther++;
    }
    }
}

/**
 * Convert an minmea_float with altitude/height information in meters
 */
float GpsDecoder::convertToMeters(const struct minmea_float *value, char unit) const
{
    float meters = minmea_tofloat(value);
    switch (unit)
    {
    case 'M':
    case 'm':
        return meters;
    case 'F':
    case 'f':
        return meters * FT_TO_M;
    default:
        return INVALID_CONVERSION;
    }
}

/**
 * Send message when both GGA and RMC sentences are received
 */
void GpsDecoder::sendMessageWhenGGAisRMC()
{
    //  if ((lastGGATimestamp <=> lastRMCTimestamp) == 0) {

    // Send message over bus when both GGA and GMC sentences are received at the same time
    // It's required that both GGA and GMC sentences have the same timestamp in these cases
    // If this in practise is not happening, due to newer GPS systems position should be taken from latest RMC
    // so we take position acuracy over altitude/course
    if (lastGGATimestamp.microseconds == lastRMCTimestamp.microseconds && lastGGATimestamp.seconds == lastRMCTimestamp.seconds)
    {
        // Can we get bank angle from turnrate?? https://aviation.stackexchange.com/questions/65628/what-is-the-formula-for-the-bank-angle-required-for-a-turn-in-line-abreast-forma
        getBus().receive(
            OpenAce::OwnshipPositionMsg
        {
            OpenAce::OwnshipPositionInfo{
                .timestamp = CoreUtils::getPositionTs(),
                .airborne = groundSpeed() > OpenAce::GROUNDSPEED_CONSIDERING_AIRBORN ? true : false, // airborne
                .lat = latitude(),
                .lon = longitude(),
                .altitudeWgs84 = static_cast<int16_t>(altitude()),
                .verticalSpeed = altitude.perSecond(), // vertical speed
                                         .groundSpeed = groundSpeed(),          // Ground Speed
                                         .course = course(),
                                         .hTurnRate = course.perSecond(), // hTurnRate   // degrees per second
                                         .velocityNorth = velocityNorth,
                                         .velocityEast = velocityEast}});

        getBus().receive(
            OpenAce::GpsPositionMsg
        {
            CoreUtils::getPositionTs(),
            latitude(),
            longitude(),
            altitude(),
            course(),
            groundSpeed()});
    }
}
