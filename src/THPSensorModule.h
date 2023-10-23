#pragma once
#include "OpenKNX.h"
#include "Sensorchannel.h"
#include "HWSensors.h"

#ifndef OPENKNX_DUALCORE
#pragma message "THPSensorModule needs OPENKNX_DUALCORE"
#endif


class THPSensorModule : public OpenKNX::Module
{

  public:
    THPSensorModule(const uint8_t* gpioPins);
    void loop() override;
    #ifdef OPENKNX_DUALCORE
    void loop1() override;
    #endif
    void setup() override;
    const std::string name() override;
    const std::string version() override;
    void processBeforeRestart() override;
    void processBeforeTablesUnload() override;
    void savePower() override;
    bool restorePower() override;
    void readFlash(const uint8_t* iBuffer, const uint16_t iSize) override;
    void writeFlash() override;
    uint16_t flashSize() override;
    static THPSensorModule *instance();
    bool processCommand(const std::string cmd, bool diagnoseKo) override;

  private:
    static THPSensorModule *_instance;
    void processInputKo(GroupObject &ko);
    uint8_t _curLoopChannel = 0;
    const uint8_t* _gpioPins;

    HWSensors _HWSensors = HWSensors();
    Sensorchannel *_Sensorchannels[THP_ChannelCount];
};