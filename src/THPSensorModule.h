#pragma once
#include "OpenKNX.h"
#include "Helper.h"
#include "KnxHelper.h"
#include "Sensorchannel.h"
#include "HWSensors.h"


class THPSensorModule : public OpenKNX::Module
{

  public:
    THPSensorModule();
    void loop() override;
    void loop2() override;
    void setup() override;
    const char *name() override;
    const char *version() override;
    void processBeforeRestart() override;
    void processBeforeTablesUnload() override;
    void savePower() override;
    bool restorePower() override;
    bool usesSecCore() override;
    static THPSensorModule *instance();

  private:
    static THPSensorModule *_instance;
    void processInputKo(GroupObject &ko);
    uint8_t _curLoopChannel = 0;

    HWSensors _HWSensors = HWSensors();
    Sensorchannel *_Sensorchannels[THP_ChannelCount];
};