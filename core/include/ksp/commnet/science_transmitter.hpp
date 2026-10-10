// WP-39: the interface seam between CommNet and the science transmission code (IScienceDataTransmitter.cs, implemented by
// ModuleDataTransmitter and consumed by CommNetVessel.GetBestTransmitter / ScienceUtil.GetBestTransmitter).
//
// ScienceData, RnDCommsStream and ResearchAndDevelopment belong to WP-35. Until that package lands, the data a transmitter queues is the
// plain struct below (the fields ModuleDataTransmitter.transmitQueuedData reads from a ScienceData); WP-35 replaces it with / adapts it to
// its ScienceData and implements ICommsStream with RnDCommsStream.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/core/delegate.hpp"

namespace ksp {
class ProtoVessel;
}

namespace ksp::CommNet {

// The fields of ScienceData that the transmit coroutine uses (ScienceData.cs: dataAmount, triggered, subjectID, baseTransmitValue,
// transmitBonus, scienceValueRatio, container)
struct TransmissionItem {
    float dataAmount = 0.0f;
    bool triggered = false;
    std::string subjectID;
    float baseTransmitValue = 0.0f;
    float transmitBonus = 0.0f;
    float scienceValueRatio = 0.0f;
    uint32_t container = 0;  // flightID of the part module that held the data (ReturnDataToContainer)
    // ScienceData.cs:46 Load / :76 Save (the title lookup through ResearchAndDevelopment is WP-35's)
    void Load(const ConfigNode& node);
    void Save(ConfigNode& node) const;
};

// RnDCommsStream.cs (WP-35): StreamData(dataAmount, protoVessel) and the xmitIncomplete flag the transmitter toggles
class ICommsStream {
public:
    virtual ~ICommsStream() = default;
    bool xmitIncomplete = false;
    virtual void StreamData(float dataAmount, ProtoVessel* protoVessel) = 0;
};

// IScienceDataTransmitter.cs
class IScienceDataTransmitter {
public:
    virtual ~IScienceDataTransmitter() = default;
    virtual float DataRate() const = 0;
    virtual double DataResourceCost() const = 0;
    virtual bool CanTransmit() = 0;
    virtual bool IsBusy() = 0;
    virtual void TransmitData(std::vector<TransmissionItem> dataQueue) = 0;
    virtual void TransmitData(std::vector<TransmissionItem> dataQueue, Callback<> callback) = 0;
};

}  // namespace ksp::CommNet
