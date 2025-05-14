// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <gridcharger/huawei/HardwareInterface.h>
#include <gridcharger/Stats.h>

namespace GridChargers {

class Provider {
public:
    virtual bool init() = 0;
    virtual void deinit() = 0;
    virtual void loop() = 0;
    virtual std::shared_ptr<Stats> getStats() const = 0;

    virtual void setFan(bool online, bool fullSpeed) = 0;
    virtual void setParameter(float val, Huawei::HardwareInterface::Setting setting) = 0;
    virtual void setProduction(bool enable) = 0;
    virtual void setMode(uint8_t mode) = 0;

    virtual bool getAutoPowerStatus() const = 0;
    virtual uint8_t getMode() const = 0;
};

} // namespace GridChargers
