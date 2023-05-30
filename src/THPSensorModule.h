#pragma once
#include "OpenKNX.h"
#include "Helper.h"
#include "KnxHelper.h"
#include "Sensorchannel.h"
#include "HWSensors.h"


class THPSensorModule : public OpenKNX::Module
{

  public:
    THPSensorModule(const uint8_t* gpioPins);
    void loop() override;
    void loop1() override;
    void setup() override;
    const std::string name() override;
    const std::string version() override;
    void processBeforeRestart() override;
    void processBeforeTablesUnload() override;
    void savePower() override;
    bool restorePower() override;
    bool usesDualCore() override;
    void readFlash(const uint8_t* iBuffer, const uint16_t iSize) override;
    void writeFlash() override;
    uint16_t flashSize() override;
    static THPSensorModule *instance();

  private:
    static THPSensorModule *_instance;
    void processInputKo(GroupObject &ko);
    uint8_t _curLoopChannel = 0;
    const uint8_t* _gpioPins;

    HWSensors _HWSensors = HWSensors();
    Sensorchannel *_Sensorchannels[THP_ChannelCount];
};