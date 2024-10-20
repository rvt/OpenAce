#pragma once

#include <stdio.h>

#include "configstore.hpp"

#include "ace/coreutils.hpp"
#include "ace/constants.hpp"
#include "ace/basemodule.hpp"
#include "ace/messages.hpp"

#include "etl/message_bus.h"
#include "etl/string.h"
#include "etl/to_arithmetic.h"

#include "ArduinoJson.hpp"

// #include "lwip/inet.h"

using namespace ArduinoJson;

typedef std::function<void(const char *NAME, const OpenAce::PinTypeMap &map)> LoadModuleCallback;

class Config : public Configuration, public etl::message_router<Config>
{
private:
    enum LoadLocation : uint8_t
    {
        // Message to indicate next period needs to be decided, send from a timer
        NA = 0,
        VOLATILE = 1,
        PERSISTENT = 2,
        DEFAULT = 3
    };

    struct
    {
        LoadLocation location = NA;
        uint16_t persistentStoreSize = 0;
    } statistics;

    etl::string_view loadLocationToString(LoadLocation location) const;

    using ccharptr = const char *;

    template <typename T>
    T configValueBypathImpl(Config &config, const etl::ivector<OpenAce::Modulename> &pathParsed) const
    {
        T variant;
        if (pathParsed.size() == 0)
        {
            return variant;
        }

        variant = config.doc[pathParsed[0]];
        for (uint8_t depth = 1; depth < pathParsed.size(); depth++)
        {
            auto const &key = pathParsed[depth];
            // TODO: Check if we can do without the PATH index check
            auto index = etl::to_arithmetic<int>(key);
            if (index.has_value())
            {
                variant = variant[(int)index.value()];
            }
            else
            {
                variant = variant[key];
            }
        }

        return variant;
    }

    template <typename T>
    T configValueBypath(const etl::ivector<OpenAce::Modulename> &pathParsed)
    {
        return configValueBypathImpl<T>(*this, pathParsed);
    }

    template <typename T>
    const T configValueBypath(const etl::ivector<OpenAce::Modulename> &pathParsed) const
    {
        return configValueBypathImpl<T>(const_cast<Config &>(*this), pathParsed);
    }

    /**
     * Get a configuration value from the configuration
     * The second and third parameter can be an integer to access an array type
     *
     * Example: auto value = configValue("api/_configuration/GDLoverUDP/ips/1.json");
     * Example: auto value = configValue("api/_configuration/GDLoverUDP/ips.json");
     * Example: auto value = configValue("api/_configuration/GDLoverUDP.json");
     */
    // template <typename T>
    // T configValueBypath(const etl::string_view path) const
    // {
    //     const etl::vector<OpenAce::Modulename, 7> vpath = CoreUtils::parsePath(path);
    //     return configValueBypath<T>(vpath);
    // }

    /**
     * Prepare the path into the configuration and return a possible index that is the last item of the path
     * Example  : api/modulefoo/bar/bas/1.json  => idx 1  bar/bas
     * Example  : api/modulefoo/bar/bas/beer.json  => idx <no value>  bar/bas/beer
     * When local = true, the json extension and /apt/_Configuration is not part of the path and points directly into configuration
     * returns: Result object
     */
    auto preparePath(const etl::string_view path, bool fullPath = true) const
    {
        struct Result
        {
            etl::to_arithmetic_result<int> idx;
            etl::vector<OpenAce::Modulename, 7> path;
        };

        auto pathParsed = CoreUtils::parsePath(path);
        if (fullPath)
        {
            pathParsed.erase(pathParsed.begin(), pathParsed.begin() + 2); // Removes /api/_Configuration path
            pathParsed.pop_back();                                        // Removes the 'json' extension
        }

        if (pathParsed.size() == 0)
        {
            return Result
            {
                etl::to_arithmetic_result<int>{},
                pathParsed};
        }

        // When the last last item is a number, it's expected to be an entry in an array
        // return the index and pop it from the path
        auto index = etl::to_arithmetic<int>(pathParsed.back());
        if (index.has_value())
        {
            pathParsed.pop_back();
        }

        return Result
        {
            index,
            pathParsed};
    }

    void serializeToVolatile();
    void serializeToPersistent();

private:
    friend class message_router;
    JsonDocument doc; // There is properly some room for optimalisation if we store the doc itself in volatile memory  eg __uninitialized_ram
    ConfigStore &volatileStore;
    ConfigStore &permanentStore;
    const uint8_t *defaultConfig;

public:
    static constexpr const etl::string_view NAME = "Config";
    Config(etl::imessage_bus &bus, ConfigStore &volatileStore_, ConfigStore &permanentStore_, const uint8_t *defaultConfig_) : Configuration(bus), volatileStore(volatileStore_), permanentStore(permanentStore_), defaultConfig(defaultConfig_)
    {
    }

    virtual ~Config() = default;

    virtual OpenAce::PostConstruct postConstruct() override;

    virtual void start() override;

    virtual void stop() override;

    virtual void getData(etl::string_stream &stream, const etl::string_view fullPath) const override;

    virtual bool setData(const etl::string_view data, const etl::string_view fullPath) override;

    virtual bool deleteData(const etl::string_view fullPath) override;

    void on_receive_unknown(const etl::imessage &msg);

    /**
     * Get the name of the current configured hardware type
     */
    virtual const OpenAce::ConfigString hardware() const
    {
        return (ccharptr)doc["hardware"];
    };

    /**
     * Returns the hardware configuration for this station or Aircraft
     *
     */
    virtual const OpenAce::Config::OpenAceConfiguration openAceConfig() const override;


    virtual const OpenAce::Config::WifiServiceData wifiService() const override;

    /**
     * Retreives the pin mapping for a given module, usually used for hardware configurations
     */
    virtual const OpenAce::PinTypeMap pinMap(const etl::string_view moduleName, OpenAce::PinTypeMap map) const override;

    virtual int valueByPath(int defaultValue, const etl::string_view pathToValue, const etl::string_view key) const override;

    virtual const OpenAce::ConfigString strValueByPath(const etl::string_view defaultValue, const etl::string_view pathToValue, const etl::string_view key) const override;

    virtual const OpenAce::Config::IpPort ipPortBypath(const etl::string_view pathToValue, const etl::string_view key) const override;

    virtual bool isModuleEnabled(const etl::string_view moduleName) const override;
};
