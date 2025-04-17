// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <gridcharger/huawei/DataPoints.h>
#include <gridcharger/huawei/HardwareInterface.h>

namespace GridChargers {

class Provider {
public:
    virtual bool init() = 0;
    virtual void deinit() = 0;
    virtual void loop() = 0;

    virtual void setFan(bool online, bool fullSpeed) = 0;
    virtual void setParameter(float val, Huawei::HardwareInterface::Setting setting) = 0;
    virtual void setProduction(bool enable) = 0;
    virtual void setMode(uint8_t mode) = 0;

    std::optional<float> getInputPower() const  {
        return getDataPoints().get<Huawei::DataPointLabel::InputPower>();
    }

    uint32_t getLastUpdate() const {
        return getDataPoints().getLastUpdate();
    }

    virtual Huawei::DataPointContainer const& getDataPoints() const = 0;
    virtual void getJsonData(JsonVariant& root) const = 0;
    virtual bool getAutoPowerStatus() const = 0;
    virtual uint8_t getMode() const = 0;
};

} // namespace GridChargers
