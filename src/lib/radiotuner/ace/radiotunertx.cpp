#include "radiotunertx.hpp"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "etl/algorithm.h"

#include "pico/rand.h"

OpenAce::PostConstruct RadioTunerTx::postConstruct()
{
    moduleByName(*this, Radio::NAMES[0]);
    numRadios = 1;
    if (moduleByName(*this, Radio::NAMES[1], false))
    {
        numRadios++;
    }
    return OpenAce::PostConstruct::OK;
}

void RadioTunerTx::start()
{
    //    addDataSourceToTasks(dataSource, radio);

    Configuration *config = static_cast<Configuration *>(BaseModule::moduleByName(*this, Configuration::NAME, false));
    if (config)
    {
        enableDisableDatasources(config->openAceConfig().protocols);
    }

    getBus().subscribe(*this);
};

void RadioTunerTx::stop()
{
    getBus().unsubscribe(*this);

    // Remove all DataSources that are not required
    for (auto const &it : txTasks)
    {

        xTimerDelete(it.timerHandle, TASK_DELAY_MS(2'000));
        xTaskNotify(it.taskHandle, TaskState::EXIT, eSetBits);
        while (eTaskGetState(it.taskHandle) != eDeleted)
        {
            vTaskDelay(TASK_DELAY_MS(50));
        }
    }
    txTasks.clear();
};

void RadioTunerTx::getData(etl::string_stream &stream, const etl::string_view optional) const
{
    stream << "{";
    stream << "\"protocols\":[";
    for (auto it = txTasks.cbegin(); it != txTasks.cend(); ++it)
    {
        stream << "\"" << dataSourceToString(it->source) << "\"";
        if (std::next(it) != txTasks.cend())
        {
            stream << ",";
        }
    }

    stream << "]";
    for (auto it = txTasks.cbegin(); it != txTasks.cend(); ++it)
    {
        it->getData(stream);
    }
    stream << ",\"zone\":\"" << CountryRegulations::zoneToString(currentZone) << "\"";
    stream << "}\n";
}

void RadioTunerTx::timerTxCallback(TimerHandle_t xTimer)
{
    RadioTunerTx::SendPositionCtx *taskCtx = (RadioTunerTx::SendPositionCtx *)pvTimerGetTimerID(xTimer);
    xTaskNotify(taskCtx->taskHandle, TaskState::TIMER, eSetBits);
}

void RadioTunerTx::radioTxTask(void *arg)
{
    constexpr uint16_t TIMEOUT_DELAY = 3'000; // Cannot be lower than one of the timeouts from CountryRegulations
    RadioTunerTx::SendPositionCtx *taskCtx = static_cast<RadioTunerTx::SendPositionCtx *>(arg);

    while (true)
    {
        uint32_t notifyValue = ulTaskNotifyTake(pdTRUE, TASK_DELAY_MS(TIMEOUT_DELAY));
        if (notifyValue & TaskState::EXIT)
        {
            vTaskDelete(nullptr);
            return;
        }
        else if (notifyValue & TaskState::TIMER)
        {

            if (taskCtx->controller->currentZone == CountryRegulations::Zone::ZONE0)
            {
                continue;
            }

            const auto &protocolTimeSlot = CountryRegulations::protocolTimeslotById(taskCtx->protocolTimingIdx);

            // When zone changed get a new IDX
            if (protocolTimeSlot.zone != taskCtx->controller->currentZone)
            {
                taskCtx->protocolTimingIdx = CountryRegulations::getFirstSlotIdx(taskCtx->controller->currentZone, taskCtx->source);
            }

            // When a valid timingIdx, process
            if (taskCtx->protocolTimingIdx != CountryRegulations::NONE_DATASOURCE.idx)
            {
                // Create a message on the message bus to request positional message for a specific protocol
                uint32_t frequency = CountryRegulations::determineFrequency(protocolTimeSlot);
                taskCtx->controller->getBus().receive(
                    OpenAce::RadioTxPositionRequest
                {
                    Radio::RadioParameters{
                        protocolTimeSlot.radioConfig,
                        frequency,
                        protocolTimeSlot.frequency.powerdBm},
                    taskCtx->radioNo});

                auto currentMs = CoreUtils::msInSecond();
                auto nextTxTime = CountryRegulations::getNextTxTime(currentMs, taskCtx->protocolTimingIdx);

                taskCtx->protocolTimingIdx = nextTxTime.idx;
                // printf("idx:%d expected: %d\n", nextTxTime.idx, (nextTxTime.duration + currentMs) % 1000);
                xTimerChangePeriod(taskCtx->timerHandle, TASK_DELAY_MS(nextTxTime.duration), TASK_DELAY_MS(10));
                taskCtx->statistics.txRequests++;
            }
        }
        else
        {
            // Always restart a timer
            xTimerChangePeriod(taskCtx->timerHandle, TASK_DELAY_MS(TIMEOUT_DELAY / 2), TASK_DELAY_MS(2));
            taskCtx->statistics.timerMissed++;
        }
    }
}

void RadioTunerTx::on_receive(const OpenAce::OwnshipPositionMsg &msg)
{
    static uint16_t checkEvery = 0;
    if (currentZone == CountryRegulations::Zone::ZONE0 || checkEvery++ > (OPENACE_GPS_FREQUENCY * 30))
    {
        checkEvery = 0;
        currentZone = CountryRegulations::zone(msg.position.lat, msg.position.lon);
    }
}

void RadioTunerTx::on_receive(const OpenAce::ConfigUpdatedMsg &msg)
{
    if (msg.moduleName == "config")
    {
        const auto aceConfig = msg.config.openAceConfig();
        enableDisableDatasources(aceConfig.protocols);
    }
}

void RadioTunerTx::enableDisableDatasources(const etl::ivector<OpenAce::DataSource> &datasources)
{

    // Remove all DataSources that are not required
    for (auto it = txTasks.begin(); it != txTasks.end();)
    {
        {
            bool taskSourceInDatasource = etl::find_if(datasources.cbegin(), datasources.cend(), [&it](const OpenAce::DataSource &source)
            {
                return it->source == source;
            }) != datasources.cend();

            if (!taskSourceInDatasource)
            {
                xTimerDelete(it->timerHandle, TASK_DELAY_MS(2'000));
                xTaskNotify(it->taskHandle, TaskState::EXIT, eSetBits);
                while (eTaskGetState(it->taskHandle) != eDeleted)
                {
                    vTaskDelay(TASK_DELAY_MS(50));
                }
                it = txTasks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Add all DataSources that are required, but not yet running
    uint8_t numRadio = 0;
    for (auto dataSource : datasources)
    {
        numRadio++;
        bool isRunning = etl::find_if(txTasks.cbegin(), txTasks.cend(), [dataSource](const SendPositionCtx &ctx)
        {
            return ctx.source == dataSource;
        }) != txTasks.cend();

        if (!isRunning)
        {
            if (!txTasks.full())
            {
                auto &ref = txTasks.emplace_back(dataSource, this, numRadio % numRadios);

                ref.timerHandle = xTimerCreate("txTaskTimer", TASK_DELAY_MS(250), pdFALSE /* Must not be autostart */, &ref, timerTxCallback);
                if (ref.timerHandle == nullptr)
                {
                    txTasks.pop_back();
                    continue;
                }

                xTaskCreate(radioTxTask, "txTask", configMINIMAL_STACK_SIZE + 64, &ref, tskIDLE_PRIORITY, &ref.taskHandle);
                if (ref.taskHandle == nullptr)
                {

                    txTasks.pop_back();
                    continue;
                }
            }
        }
    }
}

void RadioTunerTx::on_receive_unknown(const etl::imessage &msg)
{
}