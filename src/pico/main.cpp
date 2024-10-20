#include "main.h"
#include "build_time.hpp"
#include "default_config.hpp"

/* System. */
#include <stdio.h>
#include <malloc.h>

/* FreeRTOS. */
#include "FreeRTOS.h"
#include "croutine.h"
#include "task.h"

/* pico */
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

/* Vendor. */
#include "etl/list.h"
#include "etl/error_handler.h"
#include "etl/exception.h"

/* OpenAce. */
#include "ace/serialadsb.hpp"
#include "ace/dump1090client.hpp"

#include "ace/messagerouter.hpp"
#include "ace/constants.hpp"
#include "ace/aircrafttracker.hpp"
#include "ace/basemodule.hpp"
#include "ace/config.hpp"
#include "ace/inmemorystore.hpp"
#include "ace/flashstore.hpp"
#include "ace/ubloxm8n.hpp"
#include "ace/adsbdecoder.hpp"
#include "ace/picortc.hpp"
#include "ace/wifiservice.hpp"
#include "ace/webserver.hpp"
#include "ace/gpsdecoder.hpp"
#include "ace/acespi.hpp"
#include "ace/bmp280.hpp"
#include "ace/sx1262.hpp"
#include "ace/radiotunerrx.hpp"
#include "ace/radiotunertx.hpp"
#include "ace/flarm2024.hpp"
#include "ace/ogn1.hpp"
#include "ace/adsl.hpp"
#include "ace/gdl90service.hpp"
#include "ace/gdloverudp.hpp"

const char* buildTime = BUILD_TIMESTAMP;

/* Prototypes for the standard FreeRTOS callback/hook functions implemented
within this file. */
void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationTickHook(void);
void vAssertCalled(const char *pcFile, uint32_t ulLine);

void etlcpp_receive_error(const etl::exception &e)
{
    printf("ETLCPP error was %s in file %s at line %d\n", e.what(), e.file_name(), e.line_number());
}

uint32_t getTotalHeap(void)
{
    extern char __StackLimit, __bss_end__;
    return &__StackLimit - &__bss_end__;
}

uint32_t getFreeHeap(void)
{
    auto m = mallinfo();
    return getTotalHeap() - m.uordblks;
}

/**
 * Module Manager will be responsible at a later stage to re-load modules during runtime
 */
class ModuleManager : public BaseModule, public etl::message_router<ModuleManager>
{

public:
    static constexpr const etl::string_view NAME = "ModuleManager";
    ModuleManager(etl::imessage_bus &bus, const Configuration &config) : BaseModule(bus, NAME)
    {
        (void)config;
    }
    virtual ~ModuleManager() = default;
    virtual OpenAce::PostConstruct postConstruct() override
    {
        return OpenAce::PostConstruct::OK;
    }
    virtual void start() override {}
    virtual void stop() override {}
    void on_receive_unknown(const etl::imessage &msg)
    {
        (void)msg;
    }
};

void registerModules()
{
    // // *INDENT-OFF*
    BaseModule::registerModule(AceSpi::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new AceSpi(bus, config); });
    BaseModule::registerModule(Bmp280::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new Bmp280(bus, config); });
    // BaseModule::registerModule(Config::NAME, [] (etl::imessage_bus &bus, const Configuration &config) -> BaseModule* { return new Config(bus, FlashStore, DEFAULT_OPENACE_CONFIG);});
    BaseModule::registerModule(Gdl90Service::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new Gdl90Service(bus, config); });
    BaseModule::registerModule(WifiService::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new WifiService(bus, config); });
    BaseModule::registerModule(Webserver::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new Webserver(bus, config); });
    BaseModule::registerModule(PicoRtc::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new PicoRtc(bus, config); });
    BaseModule::registerModule(Sx1262::NAMES[0], [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new Sx1262(bus, config, 0); });
    BaseModule::registerModule(Sx1262::NAMES[1], [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new Sx1262(bus, config, 1); });
    BaseModule::registerModule(RadioTunerTx::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new RadioTunerTx(bus, config); });
    BaseModule::registerModule(RadioTunerRx::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new RadioTunerRx(bus, config); });
    BaseModule::registerModule(ADSBDecoder::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new ADSBDecoder(bus, config); });
    BaseModule::registerModule(Flarm2024::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new Flarm2024(bus, config); });
    BaseModule::registerModule(Ogn1::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new Ogn1(bus, config); });
    BaseModule::registerModule(ADSL::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new ADSL(bus, config); });
    BaseModule::registerModule(GDLoverUDP::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new GDLoverUDP(bus, config); });
    BaseModule::registerModule(GpsDecoder::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new GpsDecoder(bus, config); });
    BaseModule::registerModule(UbloxM8N::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new UbloxM8N(bus, config); });
    BaseModule::registerModule(SerialADSB::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new SerialADSB(bus, config); });
    BaseModule::registerModule(Dump1090Client::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new Dump1090Client(bus, config); });
    BaseModule::registerModule(ModuleManager::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new ModuleManager(bus, config); });
    BaseModule::registerModule(AircraftTracker::NAME, [](etl::imessage_bus &bus, const Configuration &config) -> BaseModule *
                               { return new AircraftTracker(bus, config); });
    // // *INDENT-ON*

    for (auto a : BaseModule::registeredModules())
    {
        printf("Registered %s\n", a.first.cbegin());
    }
}

static InMemoryStore volatileStore;
static FlashStore permanentStore{4096, 0};
static OpenAce::ThreadSafeBus<25> bus;
static Config config(bus, volatileStore, permanentStore, DEFAULT_OPENACE_CONFIG);

static void load(const etl::string_view str, etl::imessage_bus &bus, const Configuration &config, bool force = false)
{

    if (!(config.isModuleEnabled(str) || force))
    {
        printf("Module %s disabled\n", str.cbegin());
    }
    else
    {
        printf("Loading %s ...", str.cbegin());
        if (BaseModule::registeredModules().contains(str))
        {
            auto registeredModules = BaseModule::registeredModules();
            auto *client = registeredModules[str].loadFunction(bus, config);
            if (client)
            {
                printf(" %s::PostConstruct()...", str.cbegin());
                auto result = client->postConstruct();
                if (result == OpenAce::PostConstruct::OK)
                {
                    BaseModule::setModuleStatus(str, client, result);
                    printf(" %s::start()...", client->name().cbegin());
                    client->start();
                }
                else
                {
                    BaseModule::setModuleStatus(str, nullptr, result);
                    printf(" Unloading... reason [%s]", postConstructToString(result));
                    delete client;
                }
            }
            else
            {
                printf(" -> out of memory %s...", str.cbegin());
            }
            printf(" Free: %d\n", xPortGetFreeHeapSize());
        }
        else
        {
            printf(" -> not Found %s... ", str.cbegin());
        }
    }

    printf("\n");
}

static void loadModules(void *arch)
{
    (void)arch;
    load(WifiService::NAME, bus, config, true);
    load(ModuleManager::NAME, bus, config);

    WifiService *client = (WifiService *)(config.moduleByName(config, WifiService::NAME, false));
    if (client != nullptr)
    {
        load(Webserver::NAME, bus, config, true);
    }
    load(AircraftTracker::NAME, bus, config, true);
    load(AceSpi::NAME, bus, config, true);
    load(PicoRtc::NAME, bus, config, true);

    for (uint8_t i = 0; i < OPEN_ACE_MAX_RADIOS; i++)
    {
        load(Sx1262::NAMES[i], bus, config);
    }
    load(RadioTunerRx::NAME, bus, config);
    load(RadioTunerTx::NAME, bus, config);

    load(Bmp280::NAME, bus, config);
    load(Gdl90Service::NAME, bus, config);
    load(ADSBDecoder::NAME, bus, config);
    load(ADSL::NAME, bus, config);
    load(Flarm2024::NAME, bus, config);
    load(Ogn1::NAME, bus, config);
    load(GDLoverUDP::NAME, bus, config);
    load(GpsDecoder::NAME, bus, config);
    load(UbloxM8N::NAME, bus, config);

    // SerialADSB messes up the serial terminal, but it will load beyond this point
    // load(SerialADSB::NAME, bus, config);
    load(Dump1090Client::NAME, bus, config);
    // puts("\033[2J\033[H");
    puts("All modules loaded!\n");

    while (true)
    {
        // printf("Free: %ld\n", xPortGetFreeHeapSize()); vTaskDelay(10);
        if (cyw43_arch_async_context())
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            vTaskDelay(TASK_DELAY_MS(10));
            // printf("1\n");

            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            vTaskDelay(TASK_DELAY_MS(990));
        }
        else
        {
            puts("Wifi module not enabled");
            vTaskDelay(TASK_DELAY_MS(5000));
        }
    }
}

void vLaunch(void)
{

    // Bootstap
    BaseModule::initBase();
    registerModules();
    puts("--");
    auto status = config.postConstruct();
    BaseModule::setModuleStatus(Configuration::NAME, &config, status);
    BaseModule::setModuleStatus(Config::NAME, &config, status);
    config.start();
    // Bootstap

    // + 1024 because we run the message bus in this task
    TaskHandle_t taskpublish_handle;
    UBaseType_t uxCoreAffinityMask;
    xTaskCreate(loadModules, "LoadModulesTask", configMINIMAL_STACK_SIZE + 2048 /* 96 */, NULL, tskIDLE_PRIORITY, &(taskpublish_handle));
    uxCoreAffinityMask = ((1 << 0));
    vTaskCoreAffinitySet(taskpublish_handle, uxCoreAffinityMask);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUM_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif
    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

void overflowTest()
{
    uint32_t time = 0x000000001;  // Time overflows
    uint32_t start = 0xfffffff0;  // 'old' time
    uint32_t diff = time - start; // Calculate difference
    if (diff != 17)               // Should be 17 if overflow is handled correctly
    {
        panic("Compiler or CPU does not handle overflow correctly");
    }
}

int main()
{
    overflowTest();
    stdio_init_all();
#ifndef NDEBUG
    etl::error_handler::set_callback<etlcpp_receive_error>();
#endif

    printf(
        R"=(

         ██████╗ ██████╗ ███████╗███╗   ██╗ █████╗  ██████╗███████╗
        ██╔═══██╗██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔════╝
        ██║   ██║██████╔╝█████╗  ██╔██╗ ██║███████║██║     █████╗
        ██║   ██║██╔═══╝ ██╔══╝  ██║╚██╗██║██╔══██║██║     ██╔══╝
        ╚██████╔╝██║     ███████╗██║ ╚████║██║  ██║╚██████╗███████╗
        ╚═════╝ ╚═╝     ╚══════╝╚═╝  ╚═══╝╚═╝  ╚═╝ ╚═════╝╚══════╝

    )=");

      /* Configure the hardware ready to run the demo. */
      const char *rtos_name;
#if (portSUPPORT_SMP == 1)
    rtos_name = "SMP";
#else
    rtos_name = "Single Core";
#endif

#if (portSUPPORT_SMP == 1) && (configNUM_CORES == 2)
    printf("Starting %s\n\n", rtos_name);
    vLaunch();
#elif (RUN_FREERTOS_ON_CORE == 1)
    printf("Starting %s on core 1:\n\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true)
        ;
#else
    printf("Starting %s on core 0:\n\n", rtos_name);
    vLaunch();
#endif

    return 0;
}