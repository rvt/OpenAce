#include "radiotunerrx.hpp"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "etl/algorithm.h"

#include "pico/rand.h"

OpenAce::PostConstruct RadioTunerRx::postConstruct()
{
    moduleByName(*this, Radio::NAMES[0]);
    uint8_t numRadios = 1;
    if (moduleByName(*this, Radio::NAMES[1], false))
    {
        numRadios++;
    }
    addRadioTasks(numRadios);
    return OpenAce::PostConstruct::OK;
}

void RadioTunerRx::start()
{
    getBus().subscribe(*this);

    Configuration *config = static_cast<Configuration *>(BaseModule::moduleByName(*this, Configuration::NAME, false));
    if (config)
    {
        enableDisableDatasources(config->openAceConfig().protocols);
    }
};

void RadioTunerRx::stop()
{
    getBus().unsubscribe(*this);

    for (auto it = radioTasks.cbegin(); it != radioTasks.cend(); it++)
    {
        xTimerDelete(it->timerHandle, TASK_DELAY_MS(2'000));
        xTaskNotify(it->taskHandle, TaskState::EXIT, eSetBits);
        while (eTaskGetState(it->taskHandle) != eDeleted)
        {
            vTaskDelay(TASK_DELAY_MS(50));
        }
    }
    radioTasks.clear();
};

/**
 * Create a tune task per radio
 */
void RadioTunerRx::addRadioTasks(uint8_t numRadios)
{
    for (uint8_t radioNo = 0; radioNo < numRadios; radioNo++)
    {

        auto radio = static_cast<Radio *>(moduleByName(*this, Radio::NAMES[radioNo], false));
        auto &ref = radioTasks.emplace_back(this, radio);

        ref.timerHandle = xTimerCreate("rxTaskTimer", TASK_DELAY_MS(1'000), pdFALSE /* Must not be autostart */, &ref, timerTuneCallback);
        if (ref.timerHandle == nullptr)
        {
            radioTasks.pop_back();
            puts("RadioTunerRx: Failed to create timer.");
            continue;
        }

        xTaskCreate(radioTuneTask, "rxTask", configMINIMAL_STACK_SIZE + 64, &ref, tskIDLE_PRIORITY, &ref.taskHandle);
        if (ref.taskHandle == nullptr)
        {

            radioTasks.pop_back();
            puts("RadioTunerRx: Failed to create task.");
            continue;
        }
    }
}

void RadioTunerRx::getData(etl::string_stream &stream, const etl::string_view optional) const
{
    stream << "{";
    for (auto it = radioTasks.cbegin(); it != radioTasks.cend(); ++it)
    {
        it->getData(stream);
    }
    stream << ",\"zone\":\"" << CountryRegulations::zoneToString(currentZone) << "\"";
    stream << "}\n";
}

//*********************** Tuner tasks ***********************

/**
 * Call back the task that handle the radio indicating that the wait period is completed and that the radio needs to change to the new datasource
 */
void RadioTunerRx::timerTuneCallback(TimerHandle_t xTimer)
{
    RadioTunerRx::RadioProtocolCtx *taskCtx = (RadioTunerRx::RadioProtocolCtx *)pvTimerGetTimerID(xTimer);
    xTaskNotify(taskCtx->taskHandle, TaskState::TIMER, eSetBits);
}

void RadioTunerRx::radioTuneTask(void *arg)
{
    constexpr uint16_t TIMEOUT_DELAY = 2'000;
    RadioTunerRx::RadioProtocolCtx *taskCtx = static_cast<RadioTunerRx::RadioProtocolCtx *>(arg);
    bool taskBlock = false;
    while (true)
    {
        uint32_t notifyValue = ulTaskNotifyTake(pdTRUE, TASK_DELAY_MS(TIMEOUT_DELAY));
        if (notifyValue & TaskState::EXIT)
        {
            vTaskDelete(nullptr);
            return;
        }
        else if (notifyValue & TaskState::BLOCK)
        {
            taskBlock = true;
        }
        else if (notifyValue & TaskState::UNBLOCK)
        {
            taskBlock = false;
        }

        if (taskBlock)
        {
            continue;
        }
        else if (notifyValue & TaskState::TIMER)
        {
            if (taskCtx->upcomingTimeslot != CountryRegulations::NONE_DATASOURCE.idx)
            {
                // printf("Set frequency to f:%ld ms:%d zone:%d source:%s\n", frequency, CoreUtils::msInSecond(), taskCtx->nextTimeSlot.zone, OpenAce::dataSourceToString(radioTask->nextTimeSlot.source));
                auto nextTimeSlot = CountryRegulations::protocolTimeslotById(taskCtx->upcomingTimeslot);
                auto frequency = CountryRegulations::determineFrequency(nextTimeSlot);

                // Send a message to the radio to indicate to switch and listen to a different protocol
                taskCtx->radio->rxMode(
                    {Radio::RadioParameters{
                        nextTimeSlot.radioConfig,
                        frequency,
                        nextTimeSlot.frequency.powerdBm}});

                // Set timer for the next slot
                auto delay = taskCtx->advanceReceiveSlot(taskCtx->controller->currentZone);
                xTimerChangePeriod(taskCtx->timerHandle, TASK_DELAY_MS(delay), TASK_DELAY_MS(10));
                taskCtx->statistics.rxRequests++;
            }
        }
        else
        {
            // Try to find a next slot
            taskCtx->advanceReceiveSlot(taskCtx->controller->currentZone);
            xTimerChangePeriod(taskCtx->timerHandle, TASK_DELAY_MS(TIMEOUT_DELAY / 2), TASK_DELAY_MS(2));
            taskCtx->statistics.timerMissed++;
        }
    }
}

// ******************** Message bus receive handlers ********************

void RadioTunerRx::on_receive(const OpenAce::OwnshipPositionMsg &msg)
{
    static uint32_t lastTime = CoreUtils::msSinceBoot();
    auto msSinceBoot = CoreUtils::msSinceBoot();
    // Update ZONE every 30 seconds, or when still at ZONE0
    if (currentZone == CountryRegulations::Zone::ZONE0 || CoreUtils::msElapsed(lastTime, msSinceBoot) > 30000)
    {
        lastTime = msSinceBoot;
        currentZone = CountryRegulations::zone(msg.position.lat, msg.position.lon);
    }
}

void RadioTunerRx::on_receive(const OpenAce::AircraftPositionMsg &msg)
{
    static uint32_t lastTime = CoreUtils::msSinceBoot();
    slotReceive[(uint8_t)msg.position.dataSource]++;

    // Update the tasks at least every seconds, but not on each and every aircraft message
    auto msSinceBoot = CoreUtils::msSinceBoot();
    if (CoreUtils::msElapsed(lastTime, msSinceBoot) > 1000)
    {
        lastTime = msSinceBoot;
        for (auto &taskCtx : radioTasks)
        {
            taskCtx.updateSlotReceive(slotReceive);
        }
    }
}

void RadioTunerRx::on_receive_unknown(const etl::imessage &msg)
{
}

void RadioTunerRx::on_receive(const OpenAce::ConfigUpdatedMsg &msg)
{
    if (msg.moduleName == "config")
    {
        const auto aceConfig = msg.config.openAceConfig();
        enableDisableDatasources(aceConfig.protocols);
    }
}

void RadioTunerRx::enableDisableDatasources(const etl::ivector<OpenAce::DataSource> &dataSources)
{
    // Step 1: Block all tasks first
    for (auto &taskCtx : radioTasks)
    {
        xTaskNotify(taskCtx.taskHandle, TaskState::BLOCK, eSetBits);
    }

    // TODO: Synchronize task blocking here instead of fixed delay
    vTaskDelay(TASK_DELAY_MS(50)); // This delay could be replaced with a synchronization mechanism

    // Step 2: Precompute how many protocols each radio should handle
    uint8_t dsPerRadio = dataSources.size() / radioTasks.size();
    uint8_t remainingSources = dataSources.size() % radioTasks.size(); // Handle remainder for last radio
    uint8_t newDsPos = 0;

    // Step 3: Distribute data sources across radios
    for (auto &taskCtx : radioTasks)
    {
        uint8_t sourcesForThisRadio = dsPerRadio;

        // Distribute the remainder to the last radio
        if (&taskCtx == &radioTasks.back())
        {
            sourcesForThisRadio += remainingSources;
        }

        // Copy the assigned data sources for this radio
        etl::vector<OpenAce::DataSource, MAX_SOURCE_PER_RADIO> newDataSources;
        for (uint8_t i = 0; i < sourcesForThisRadio; ++i)
        {
            newDataSources.emplace_back(dataSources[newDsPos]);
            newDsPos++;
        }

        // Step 4: Update task with new data sources
        taskCtx.updateDataSources(newDataSources);

        // Unblock the task
        xTaskNotify(taskCtx.taskHandle, TaskState::UNBLOCK, eSetBits);
    }
}
