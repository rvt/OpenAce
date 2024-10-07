#include "sx1262.hpp"

/* FreeRTOS. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* OpenACE. */
#include "etl/map.h"
#include "ace/manchester.hpp"
#include "ace/coreutils.hpp"

void Sx1262::start()
{
    getBus().subscribe(*this);

    // startListen();
};

void Sx1262::stop()
{
    getBus().unsubscribe(*this);
    vQueueDelete(commandQueue);
    xTaskNotify(taskHandle, TaskState::DELETE, eSetBits);
};

//  const char* Sx1262::name() const
// {
//     return Sx1262::NAMES[radioNo];
// }

OpenAce::PostConstruct Sx1262::postConstruct()
{
    spiHall = static_cast<SpiModule *>(BaseModule::moduleByName(*this, SpiModule::NAME));

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(csPin);
    gpio_set_dir(csPin, GPIO_OUT);
    gpio_put(csPin, 1);

    // Busy Pin has PullUp from SX1262 so kept as input only
    gpio_init(busyPin);
    gpio_set_dir(busyPin, GPIO_IN);

    // Make the SPI pins available to picotool
    bi_decl(bi_1pin_with_name(static_cast<uint32_t>(csPin), "Sx1262 CS"));

    // Read the device type
    char data[7];
    sx126x_read_register(this, 0x0320, (uint8_t *)data, 6);
    data[6] = 0;

    // Note that even for a SX1262, the version might come back as SX1261
    // https://forum.lora-developers.semtech.com/t/sx126x-device-id/1508
    if (strncmp(data, "SX126", 5) != 0)
    {
        printf("Expected SX126X, but found [%s] ", data);
        return OpenAce::PostConstruct::HARDWARE_NOT_FOUND;
    }
    printf(" found [%s] (Sx1261 is normal for a Sx1262) ", data);

    commandQueue = xQueueCreate(2, sizeof(Sx1262::Command_t));
    if (commandQueue == nullptr)
    {
        return OpenAce::PostConstruct::XQUEUE_ERROR;
    }

    BaseType_t returned = xTaskCreate(sx1262Task, "sx1262Task", configMINIMAL_STACK_SIZE + 512, this, tskIDLE_PRIORITY, &taskHandle);
    if (returned != pdPASS)
    {
        return OpenAce::PostConstruct::TASK_ERROR;
    }
    registerPinInterupt(dio1Pin, GPIO_IRQ_EDGE_RISE, taskHandle, TASK_VALUE_DIO1_INTERRUPT);

    radioInit();

    printf("Initialised on cs:%d busy:%d dio1:%d ", csPin, busyPin, dio1Pin);
    return OpenAce::PostConstruct::OK;
}

void Sx1262::getData(etl::string_stream &stream, const etl::string_view optional) const
{
    stream << "{";
    stream << "\"deviceErrors\":" << statistics.deviceErrors;
    stream << ",\"waitPacketTimeout\":" << statistics.waitPacketTimeout;
    stream << ",\"receivedPackets\":" << statistics.receivedPackets;
    stream << ",\"buzyWaitsTimeout\":" << statistics.buzyWaitsTimeout;
    stream << ",\"queueFull\":" << statistics.queueFull;
    stream << ",\"txTimeout\":" << statistics.txTimeout;
    stream << ",\"txOk\":" << statistics.txOk;
    stream << ",\"mode\":" << "\"" << Radio::modeString(statistics.mode) << "\"";
    stream << ",\"dataSource\":" << "\"" << OpenAce::dataSourceToString(statistics.dataSource) << "\"";
    stream << ",\"frequency\":" << statistics.frequency;
    stream << ",\"powerdBm\":" << statistics.powerdBm;
    stream << ",\"txEnabled\":" << txEnabled;
    stream << "}\n";
}

void Sx1262::on_receive(const OpenAce::RadioTxFrame &msg)
{
    if (msg.radioNo == radioNo)
    {
        txPacket(msg.txPacket);
    }
}

void Sx1262::on_receive(const OpenAce::ConfigUpdatedMsg &msg)
{
    if (msg.moduleName == NAMES[radioNo])
    {
        txEnabled = msg.config.valueByPath(true, NAMES[radioNo], "txEnabled");
        offset = msg.config.valueByPath(true, NAMES[radioNo], "offset");
    }
}

/**
 * Apply methods
 */

void Sx1262::radioInit()
{
    // .365
    sx126x_buzy_wait(busyPin, 100000);

    sx126x_set_rx_tx_fallback_mode(this, SX126X_FALLBACK_STDBY_XOSC);
    sx126x_clear_irq_status(this, SX126X_IRQ_ALL);
    sx126x_set_dio_irq_params(this, 0, 0, 0, 0);

    sx126x_set_dio3_as_tcxo_ctrl(this, SX126X_TCXO_CTRL_1_6V, 5000.f / 15.625);
    sx126x_set_dio2_as_rf_sw_ctrl(this, true);

    // 9.4 Standby (STDBY) Mode DCDC must be set in _RC mode
    // 13.1.12 Calibrate Function must happen in SX126X_STANDBY_CFG_RC
    sx126x_set_standby(this, SX126X_STANDBY_CFG_RC);
    sx126x_set_reg_mode(this, SX126X_REG_MODE_DCDC);
    sx126x_cal(this, SX126X_CAL_ALL);
    vTaskDelay(35); // Seems it's really needed to wait a bit
    checkAndClearDeviceErrors();

    sx126x_cal_img_in_mhz(this, 863, 870); // 13.1.13 CalibrateImage
    checkAndClearDeviceErrors();
    // Calibation can take 90ms, here we wait for it to finish to continue setting up the device
    if (sx126x_buzy_wait(busyPin, 150000))
    {
        statistics.buzyWaitsTimeout++;
    }
    vTaskDelay(100); // Seems it's really needed to wait 100ms before other commands can be send
    sx126x_set_standby(this, SX126X_STANDBY_CFG_XOSC);
    sx126x_set_pa_cfg(this, &DEFAULT_HIGH_POWER_PA_CFG);
    // OCP (Over Current Protection) Configuration must be set after sx126x_set_pa_cfg
    sx126x_set_ocp_value(this, (uint8_t)(60.0 / 2.5));

    checkAndClearDeviceErrors();

    // Better Resistance of the SX1262 Tx to Antenna Mismatch
    uint8_t clamp;
    sx126x_read_register(this, 0x08D8, &clamp, 1);
    clamp |= 0x1E;
    sx126x_write_register(this, 0x08D8, &clamp, 1);

    standBy();
}

void Sx1262::standBy()
{
    if (sx126x_buzy_wait(busyPin, 150000))
    {
        statistics.buzyWaitsTimeout++;
        return;
    }
    // .715
    sx126x_set_standby(this, SX126X_STANDBY_CFG_RC);
}

uint8_t Sx1262::receivedPacketLength() const
{
    sx126x_rx_buffer_status_t rx_buffer_status;
    sx126x_get_rx_buffer_status(this, &rx_buffer_status);
    return rx_buffer_status.pld_len_in_bytes;
}

bool Sx1262::applyNewLoraParameters(const Radio::ProtocolConfig &config)
{
    // // Set the Sync Word
    // LoRa Sync Word, Differentiate the LoRa® signal for Public or Private Network
    uint8_t data[] = {0xF4, 0x14};
    sx126x_write_register(this, 0x0740, data, 2); // SX126X_REG_LR_SYNCWORD

    // Optimizing the Inverted IQ Operation
    sx126x_read_register(this, 0x0736, data, 1); // SX126X_REG_IQ_POLARITY
    data[0] |= 0x04;
    sx126x_write_register(this, 0x0736, data, 1);

    //     if (sx126x_buzy_wait(busyPin, 150000)) {
    //         statistics.buzyWaitsTimeout++;
    //         printf("Wait busy timeout (switchToGFSK) ");
    //     }
    //     // 13.1.8 SetCAD (Only Lora Mode)
    //     sx126x_set_cad_params(this, &cad_params_lora);
    //     sx126x_set_cad(this);

    return true;
}

void Sx1262::checkAndClearDeviceErrors()
{
    sx126x_errors_mask_t errors;
    sx126x_status_t status = sx126x_get_device_errors(this, &errors);
    if (status != SX126X_STATUS_OK)
    {
        statistics.deviceErrors++;
        sx126x_clear_device_errors(this);
        // printf("Device Error: %d\n", status);
    }
}

void Sx1262::configureSx1262(const RadioParameters &lastParameters, const RadioParameters &newParameters)
{
    standBy();
    if (newParameters.config.mode == Radio::Mode::GFSK)
    {
        // if (lastParameters.config.mode != newParameters.config.mode || true)
        // {
        sx126x_set_pkt_type(this, SX126X_PKT_TYPE_GFSK);
        sx126x_clear_irq_status(this, SX126X_IRQ_ALL);
        sx126x_set_gfsk_mod_params(this, &DEFAULT_MOD_PARAMS_GFSK);
        // }

        if (lastParameters.config.dataSource != newParameters.config.dataSource || true)
        {
            //    printf("DataSource:%s\n",  OpenAce::dataSourceToString(newParameters.config.dataSource));
            auto pkt_params_gfsk = DEFAULT_PKG_PARAMS_GFSK;
            pkt_params_gfsk.preamble_len_in_bits = 1 * 8;
            pkt_params_gfsk.sync_word_len_in_bits = (newParameters.config.syncLength) * 8;
            pkt_params_gfsk.pld_len_in_bytes = newParameters.config.packetLength * MANCHESTER;

            sx126x_set_gfsk_pkt_params(this, &pkt_params_gfsk);

            sx126x_set_ocp_value(this, (uint8_t)(60.0 / 2.5));
            sx126x_set_gfsk_sync_word(this, newParameters.config.syncWord.data(), newParameters.config.syncLength);
        }
        statistics.mode = newParameters.config.mode;
    }
    else if (newParameters.config.mode == Radio::Mode::LORA)
    {
        // to implement
        applyNewLoraParameters(newParameters.config);
        statistics.mode = newParameters.config.mode;

        printf("Lora not yet supported");
    }

    if (lastParameters.frequency != newParameters.frequency)
    {
        //        printf("Freq:%ld\n", newParameters.frequency);
        sx126x_set_rf_freq(this, newParameters.frequency + offset);
        statistics.frequency = newParameters.frequency + offset;
    }

    checkAndClearDeviceErrors();

    statistics.dataSource = newParameters.config.dataSource;
}

void Sx1262::Listen()
{
    sx126x_set_dio_irq_params(this,
                              SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_SYNC_WORD_VALID | SX126X_IRQ_PREAMBLE_DETECTED,
                              SX126X_IRQ_RX_DONE, // Dio1
                              SX126X_IRQ_NONE,    // Dio2
                              SX126X_IRQ_NONE     // Dio3
                             );

    sx126x_clear_irq_status(this, SX126X_IRQ_ALL);
    sx126x_set_buffer_base_address(this, 0x00, 0x00);

    // https://forum.lora-developers.semtech.com/t/sx1262-reduced-rx-sensitivity-packet-reception-fails/162/12
    // Need to call SetFs() and then RxBoosted() periodically to fix a issue with receiver gain
    sx126x_cfg_rx_boosted(this, true);
    // Device is out into listen with timeout because there where reports
    // that sensetivity goes down at some point
    sx126x_set_rx(this, OPENACE_SX126X_MAX_RX_TIME);
}

void Sx1262::sendGFSKPacket(const RadioParameters &parameters, const uint8_t *data, uint8_t length)
{
    sx126x_set_dio_irq_params(this,
                              SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_CRC_ERROR | SX126X_IRQ_HEADER_ERROR | SX126X_IRQ_HEADER_VALID | SX126X_IRQ_SYNC_WORD_VALID | SX126X_IRQ_PREAMBLE_DETECTED,
                              SX126X_IRQ_RX_DONE | SX126X_IRQ_TX_DONE, // Dio1
                              SX126X_IRQ_NONE,                         // Dio2
                              SX126X_IRQ_NONE                          // Dio3
                             );
    sx126x_clear_irq_status(this, SX126X_IRQ_ALL);
    statistics.powerdBm = parameters.powerdBm;
    sx126x_set_tx_params(this, parameters.powerdBm, SX126X_RAMP_200_US);
    sx126x_write_buffer(this, 0, data, length);

    sx126x_set_tx(this, SX126X_MAX_TIMEOUT_IN_MS);
}

void Sx1262::receiveGFSKPacket(Radio::RadioParameters const &parameters)
{
    // 13.5.3 GetPacketStatus
    sx126x_pkt_status_gfsk_t pkt_status;
    sx126x_get_gfsk_pkt_status(this, &pkt_status);
    if (pkt_status.rx_status.pkt_received && pkt_status.rx_status.abort_error == 0)
    {
        statistics.receivedPackets++;
        uint8_t receivedFrameLength = receivedPacketLength();
        constexpr uint8_t maxFrameLength = OpenAce::RADIO_MAX_FRAME_LENGTH * MANCHESTER;
        if (receivedFrameLength > 0 && receivedFrameLength <= maxFrameLength)
        {
            uint8_t data[maxFrameLength];
            sx126x_read_buffer(this, 0x00, data, receivedFrameLength);

            OpenAce::RadioRxFrame RadioRxFrame{(uint8_t)(receivedFrameLength / MANCHESTER), CoreUtils::secondsSinceEpoch(), (int8_t)(-pkt_status.rssi_sync / 2), parameters.frequency, parameters.config.dataSource};

            // Seems like all GFSK packets are Manchester encoded.
            manchesterDecode((uint8_t *)RadioRxFrame.frame, (uint8_t *)RadioRxFrame.err, data, receivedFrameLength);
            sendToBus(RadioRxFrame);
            // dumpBuffer((uint8_t *)RadioRxFrame.frame, RadioRxFrame.length);
        }
        else
        {
            // Hapens rarely, if at all.
            // printf("maxFrameLength exceeded: expected:%d got:%d", maxFrameLength, receivedFrameLength);
        }
    }
    // printf("\n");
}

sx126x_irq_mask_t Sx1262::getIrqStatus()
{
    sx126x_irq_mask_t mask;
    sx126x_get_irq_status(this, &mask);
    return mask;
}

void Sx1262::rxMode(const RxMode &rxMode)
{
    auto command = Command_t{rxMode};

    if (xQueueSend(commandQueue, &command, TASK_DELAY_MS(5)) == pdFALSE)
    {
        statistics.queueFull++;
    }

    xTaskNotify(taskHandle, TaskState::NEW_COMMAND, eSetBits);
}

void Sx1262::txPacket(const TxPacket &txpacket)
{
    auto command = Command_t{txpacket};
    if (xQueueSend(commandQueue, &command, TASK_DELAY_MS(5)) == pdFALSE)
    {
        statistics.queueFull++;
    }
    xTaskNotify(taskHandle, TaskState::NEW_COMMAND, eSetBits);
}

void Sx1262::clearTXCallback(TimerHandle_t xTimer)
{
    TaskHandle_t handle = (TaskHandle_t)pvTimerGetTimerID(xTimer);
    xTaskNotify(handle, TaskState::CLEAR_TX, eSetBits);
}

void Sx1262::sx1262Task(void *arg)
{
    constexpr uint8_t GFSK_PACKET_INTERRUPT_STATUS = SX126X_IRQ_RX_DONE | SX126X_IRQ_PREAMBLE_DETECTED | SX126X_IRQ_SYNC_WORD_VALID;

    Sx1262 *sx1262 = static_cast<Sx1262 *>(arg);
    SpiModule *aceSpi = static_cast<SpiModule *>(BaseModule::moduleByName(*sx1262, SpiModule::NAME));
    TaskHandle_t taskHandle = xTaskGetCurrentTaskHandle();
    TimerHandle_t txClearTimerHandle = xTimerCreate("txClearTimerHandle", TASK_DELAY_MS(12), pdFALSE, taskHandle, clearTXCallback); // TX takes about 5ms, 8ms to clear should be fine

    Radio::RadioParameters lastRadioParameters{DEFAULT_PROTOCOL_CONFIG, 868'000'000, -100};
    Radio::RadioParameters beforeSendConfig{DEFAULT_PROTOCOL_CONFIG, 868'200'000, -100};

    aceSpi->aquireSlot(OPENOPENACE_SPI_DEFAULT_BUS_FREQUENCY, taskHandle);
    bool txMode = false;
    while (true)
    {
        if (uint32_t notifyValue = ulTaskNotifyTake(pdTRUE, TASK_DELAY_MS(OPENACE_SX126X_MAX_RX_TIME)))
        {

            if ((notifyValue & SpiModule::SPI_BUS_READY) == SpiModule::SPI_BUS_READY)
            {

                // Read device status to validate if there is anything to do
                auto irqStatus = sx1262->getIrqStatus();
                // printf("IRQ Status: %d %ld\n", irqStatus, notifyValue);
                if ((irqStatus & GFSK_PACKET_INTERRUPT_STATUS) == GFSK_PACKET_INTERRUPT_STATUS)
                {
                    // printf("Packet RX: %s %d\n", OpenAce::dataSourceToString(lastRadioParameters.config.dataSource), CoreUtils::msInSecond());
                    sx1262->receiveGFSKPacket(lastRadioParameters);
                    sx1262->Listen();
                }

                if (notifyValue & TaskState::CLEAR_TX)
                {
                    //                    printf("TX_CLEAR %d\n", CoreUtils::msInSecond());
                    sx1262->statistics.txTimeout++;
                }
                if (irqStatus & SX126X_IRQ_TX_DONE)
                {
                    //                    printf("TX_DONE %d\n", CoreUtils::msInSecond());
                    sx1262->statistics.txOk++;
                }

                if (irqStatus & SX126X_IRQ_TX_DONE || notifyValue & TaskState::CLEAR_TX)
                {
                    xTimerStop(txClearTimerHandle, TASK_DELAY_MS(5));
                    sx1262->configureSx1262(beforeSendConfig, lastRadioParameters);
                    sx1262->Listen();
                    txMode = false;
                    // printf("TX_HANDLE %d\n", CoreUtils::msInSecond());
                }

                if (notifyValue & TaskState::FAILSAVE_LISTEN_MODE)
                {
                    sx1262->configureSx1262(beforeSendConfig, lastRadioParameters);
                    sx1262->Listen();
                    txMode = false;
                }

                if (!txMode)
                {
                    // Read the next command if available
                    Command_t command;
                    BaseType_t hasData = xQueueReceive(sx1262->commandQueue, &command, 0);
                    if (hasData == pdTRUE)
                    {
                        switch (command.commandType)
                        {
                        case CommandType::RXMODE:
                        {
                            // printf("Set RX: %s %d\n", OpenAce::dataSourceToString(lastRadioParameters.config.dataSource), CoreUtils::msInSecond());
                            sx1262->configureSx1262(lastRadioParameters, command.rxMode.radioParameters);
                            lastRadioParameters = command.rxMode.radioParameters;
                            sx1262->Listen();
                            break;
                        }
                        case CommandType::TXPACKET:
                        {
                            if (sx1262->txEnabled)
                            {
                                beforeSendConfig = lastRadioParameters;
                                sx1262->configureSx1262(lastRadioParameters, command.txPacket.radioParameters);

                                uint8_t frame[OpenAce::RADIO_MAX_FRAME_LENGTH * 2];
                                manchechesterEncode(frame, command.txPacket.data.data(), command.txPacket.length);

                                sx1262->sendGFSKPacket(command.txPacket.radioParameters, frame, command.txPacket.length * 2);
                                txMode = true;
                                xTimerStart(txClearTimerHandle, TASK_DELAY_MS(5));
                            }
                            // printf("TX REQ %s %d\n",OpenAce::dataSourceToString(command.txPacket.radioParameters.config.dataSource), CoreUtils::msInSecond());
                            break;
                        }
                        }
                    }
                }
                // Always releasing the slot is sub-optmial, specially when sending is quickly followed by receiving
                // TODO: design some way to keep the slot aquired for a short time after sending?
                aceSpi->releaseSlot();
            }
            else
            {
                if (notifyValue & TaskState::DELETE)
                {
                    xTimerStop(txClearTimerHandle, TASK_DELAY_MS(5));
                    vTaskDelete(nullptr);
                    return;
                }
                else if (notifyValue & ~SpiModule::SPI_BUS_READY)
                {
                    aceSpi->aquireSlot(OPENOPENACE_SPI_DEFAULT_BUS_FREQUENCY, taskHandle, notifyValue);
                }
            }
        }
        else
        {
            // Endup here when timeout, only count in statistics if there was actually active
            if (lastRadioParameters.config.dataSource != OpenAce::DataSource::NONE)
            {
                sx1262->statistics.waitPacketTimeout++;
            }
            aceSpi->aquireSlot(OPENOPENACE_SPI_DEFAULT_BUS_FREQUENCY, taskHandle, TaskState::FAILSAVE_LISTEN_MODE);
        }
    }
}
