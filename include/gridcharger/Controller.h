// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <ArduinoJson.h>
#include <TaskSchedulerDeclarations.h>
#include <gridcharger/Provider.h>
#include <gridcharger/huawei/HardwareInterface.h>
#include <gridcharger/huawei/DataPoints.h>

namespace GridChargers {

class Controller {
public:
    void init(Scheduler&);
    void updateSettings();

    void setFan(bool online, bool fullSpeed);
    void setParameter(float val, Huawei::HardwareInterface::Setting setting);
    void setProduction(bool enable);
    void setMode(uint8_t mode);

    std::optional<float> getInputPower() const;
    uint32_t getLastUpdate() const;

    Huawei::DataPointContainer const& getDataPoints() const;
    void getJsonData(JsonVariant& root) const;
    bool getAutoPowerStatus() const;
    uint8_t getMode() const;

private:
    void loop();

    Task _loopTask;
    mutable std::mutex _mutex;
    std::unique_ptr<Provider> _upProvider = nullptr;
};

} // namespace GridChargers

extern GridChargers::Controller GridCharger;
