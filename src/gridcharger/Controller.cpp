// SPDX-License-Identifier: GPL-2.0-or-later
#include <Configuration.h>
#include <gridcharger/Controller.h>
#include <gridcharger/huawei/Provider.h>
#include <MqttSettings.h>
#include <LogHelper.h>

static const char* TAG = "gridCharger";
static const char* SUBTAG = "Controller";

GridChargers::Controller GridCharger;

namespace GridChargers {

void Controller::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&Controller::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    this->updateSettings();
}

void Controller::updateSettings()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_upProvider) {
        _upProvider->deinit();
        _upProvider = nullptr;
    }

    auto const& config = Configuration.get();
    if (!config.GridCharger.Enabled) { return; }

    switch (config.GridCharger.Provider) {
        case GridChargerProviderType::HUAWEI:
            _upProvider = std::make_unique<::GridChargers::Huawei::Provider>();
            break;
        default:
            DTU_LOGW("Unknown provider: %d\r\n", config.GridCharger.Provider);
            return;
    }

    if (!_upProvider->init()) { _upProvider = nullptr; }
}

void Controller::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->loop();
}

void Controller::setParameter(float val, Huawei::HardwareInterface::Setting setting)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->setParameter(val, setting);
}

void Controller::setProduction(bool enable)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->setProduction(enable);
}

void Controller::setFan(bool online, bool fullSpeed)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->setFan(online, fullSpeed);
}

void Controller::setMode(uint8_t mode)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->setMode(mode);
}

uint32_t Controller::getLastUpdate() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return 0; }

    return _upProvider->getLastUpdate();
}

std::optional<float> Controller::getInputPower() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return std::nullopt; }

    return _upProvider->getInputPower();
}

Huawei::DataPointContainer const& Controller::getDataPoints() const
{
    if (!_upProvider) { return Huawei::DataPointContainer(); }

    return _upProvider->getDataPoints();
}

uint8_t Controller::getMode() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return 0; }

    return _upProvider->getMode();
}

bool Controller::getAutoPowerStatus() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return false; }

    return _upProvider->getAutoPowerStatus();
}

void Controller::getJsonData(JsonVariant& root) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->getJsonData(root);
}

} // namespace GridChargers
