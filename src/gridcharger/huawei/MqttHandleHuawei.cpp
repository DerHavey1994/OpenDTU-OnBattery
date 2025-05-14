// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include <gridcharger/huawei/MqttHandleHuawei.h>
#include "MqttSettings.h"
#include <gridcharger/Controller.h>
#include <gridcharger/huawei/Provider.h>
#include <ctime>
#include <LogHelper.h>

static const char* TAG = "gridCharger";
static const char* SUBTAG = "MQTT";

MqttHandleHuaweiClass MqttHandleHuawei;

void MqttHandleHuaweiClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleHuaweiClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    subscribeTopics();

    _lastPublish = millis();
}

void MqttHandleHuaweiClass::forceUpdate()
{
    _lastPublish = 0;
}

void MqttHandleHuaweiClass::subscribeTopics()
{
    String const& prefix = MqttSettings.getPrefix();

    auto subscribe = [&prefix, this](char const* subTopic, Topic t) {
        String fullTopic(prefix + _cmdtopic.data() + subTopic);
        MqttSettings.subscribe(fullTopic.c_str(), 0,
                std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, t,
                    std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4));
    };

    for (auto const& s : _subscriptions) {
        subscribe(s.first.data(), s.second);
    }
}

void MqttHandleHuaweiClass::unsubscribeTopics()
{
    String const prefix = MqttSettings.getPrefix() + _cmdtopic.data();
    for (auto const& s : _subscriptions) {
        MqttSettings.unsubscribe(prefix + s.first.data());
    }
}

void MqttHandleHuaweiClass::loop()
{
    const CONFIG_T& config = Configuration.get();

    std::unique_lock<std::mutex> mqttLock(_mqttMutex);

    if (!config.GridCharger.Enabled) {
        _mqttCallbacks.clear();
        return;
    }

    for (auto& callback : _mqttCallbacks) { callback(); }
    _mqttCallbacks.clear();

    mqttLock.unlock();
}


void MqttHandleHuaweiClass::onMqttMessage(Topic enumTopic,
        const espMqttClientTypes::MessageProperties& properties,
        const char* topic, const uint8_t* payload, size_t len)
{
    std::string strValue(reinterpret_cast<const char*>(payload), len);
    float payload_val = -1;
    try {
        payload_val = std::stof(strValue);
    }
    catch (std::invalid_argument const& e) {
        DTU_LOGE("Huawei MQTT handler: cannot parse payload of topic '%s' as float: %s",
                topic, strValue.c_str());
        return;
    }

    std::lock_guard<std::mutex> mqttLock(_mqttMutex);
    using Controller = GridChargers::Controller;
    using Provider = GridChargers::Huawei::Provider;
    using Setting = GridChargers::Huawei::HardwareInterface::Setting;

    auto validateAndSetParameter = [this, payload_val](float min, float max,
            Setting setting, const char* paramName, const char* unit) -> bool {
        if (payload_val < min || payload_val > max) {
            DTU_LOGE("Invalid %s %.2f %s (valid range: %.2f-%.2f %s)",
                paramName, payload_val, unit, min, max, unit);
            return false;
        }
        DTU_LOGI("Limit %s: %.2f %s", paramName, payload_val, unit);
        _mqttCallbacks.push_back(std::bind(&Controller::setParameter, &GridCharger, payload_val, setting));
        return true;
    };

    switch (enumTopic) {
        case Topic::LimitOnlineVoltage:
            validateAndSetParameter(Provider::MIN_ONLINE_VOLTAGE, Provider::MAX_ONLINE_VOLTAGE,
                Setting::OnlineVoltage, "online voltage", "V");
            break;

        case Topic::LimitOfflineVoltage:
            validateAndSetParameter(Provider::MIN_OFFLINE_VOLTAGE, Provider::MAX_OFFLINE_VOLTAGE,
                Setting::OfflineVoltage, "offline voltage", "V");
            break;

        case Topic::LimitOnlineCurrent:
            validateAndSetParameter(Provider::MIN_ONLINE_CURRENT, Provider::MAX_ONLINE_CURRENT,
                Setting::OnlineCurrent, "online current", "A");
            break;

        case Topic::LimitOfflineCurrent:
            validateAndSetParameter(Provider::MIN_OFFLINE_CURRENT, Provider::MAX_OFFLINE_CURRENT,
                Setting::OfflineCurrent, "offline current", "A");
            break;

        case Topic::Mode:
            switch (static_cast<int>(payload_val)) {
                case 3:
                    DTU_LOGI("Received MQTT msg. New mode: Full internal control");
                    _mqttCallbacks.push_back(std::bind(&Controller::setMode, &GridCharger, HUAWEI_MODE_AUTO_INT));
                    break;

                case 2:
                    DTU_LOGI("Received MQTT msg. New mode: Internal on/off control, external power limit");
                    _mqttCallbacks.push_back(std::bind(&Controller::setMode, &GridCharger, HUAWEI_MODE_AUTO_EXT));
                    break;

                case 1:
                    DTU_LOGI("Received MQTT msg. New mode: Turned ON");
                    _mqttCallbacks.push_back(std::bind(&Controller::setMode, &GridCharger, HUAWEI_MODE_ON));
                    break;

                case 0:
                    DTU_LOGI("Received MQTT msg. New mode: Turned OFF");
                    _mqttCallbacks.push_back(std::bind(&Controller::setMode, &GridCharger, HUAWEI_MODE_OFF));
                    break;

                default:
                    DTU_LOGE("Invalid mode %.0f", payload_val);
                    break;
            }
            break;

        case Topic::Production:
        {
            bool enable = payload_val > 0;
            DTU_LOGI("Production to be %sabled", (enable?"en":"dis"));
            _mqttCallbacks.push_back(std::bind(&Controller::setProduction, &GridCharger, enable));
            break;
        }

        case Topic::LimitInputCurrent:
            validateAndSetParameter(Provider::MIN_INPUT_CURRENT_LIMIT, Provider::MAX_INPUT_CURRENT_LIMIT,
                Setting::InputCurrentLimit, "input current", "A");
            break;

        case Topic::FanOnlineFullSpeed:
        case Topic::FanOfflineFullSpeed:
        {
            bool online = (Topic::FanOnlineFullSpeed == enumTopic);
            bool fullSpeed = payload_val > 0;
            DTU_LOGI("%sline fan %s speed", (online?"On":"Off"), (fullSpeed?"full":"auto"));
            _mqttCallbacks.push_back(std::bind(&Controller::setFan, &GridCharger, online, fullSpeed));
            break;
        }
    }
}
