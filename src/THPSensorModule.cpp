#include "THPSensorModule.h"

THPSensorModule *THPSensorModule::_instance = nullptr;

THPSensorModule::THPSensorModule()
{
    THPSensorModule::_instance = this;
}

THPSensorModule *THPSensorModule::instance()
{
    return THPSensorModule::_instance;
}

const char *THPSensorModule::name()
{
    return "THPSensor";
}

const char *THPSensorModule::version()
{
    return "0.0dev";
}

void THPSensorModule::setup()
{
    // Debug
    //log("paramScenes: %i", ParamSOM_Scenes);
    //log("paramExternal: %i", ParamSOM_External);
    //log("paramLock: %i", ParamSOM_Lock);
    //log("paramDayNight: %i", ParamSOM_DayNight);
    //log("paramVolumeDay: %i", ParamSOM_VolumeDay);
    //log("paramVolumeNight: %i", ParamSOM_VolumeNight);
    //log("paramVolumeDay: %i", ParamSOM_VolumeDay);

    const uint8_t pins[] = {
            THPCHANNEL_A_SCL,THPCHANNEL_A_SDA,
            THPCHANNEL_B_SCL,THPCHANNEL_B_SDA,
            THPCHANNEL_C_SCL,THPCHANNEL_C_SDA,
            THPCHANNEL_D_SCL,THPCHANNEL_D_SDA,
            THPCHANNEL_E_SCL,THPCHANNEL_E_SDA,
            THPCHANNEL_F_SCL,THPCHANNEL_F_SDA,
            THPCHANNEL_G_SCL,THPCHANNEL_G_SDA,
            THPCHANNEL_H_SCL,THPCHANNEL_H_SDA};

    uint8_t sensortypes[THP_ChannelCount];
        for(int i=0;i<THP_ChannelCount;i++)
            sensortypes[i] = knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize*i+THP_Sensortype_);

    _HWSensors.Setup(pins, sensortypes);

    for (uint8_t i = 0; i < THP_ChannelCount; i++)
    {
        _Sensorchannels[i] = new Sensorchannel();
        _Sensorchannels[i]->Setup(pins[i*2], pins[(i*2)+1], i, &_HWSensors);
    }
}



void THPSensorModule::loop()
{   
    // do nothing while StartupDelay
    if(!openknx.afterStartupDelay())
        return;

    if (knx.configured())
    {
        //_knx_configured = true;
    }
    else
    {
        //_knx_configured = false;
        multicore_reset_core1();
        return;
    }

    uint8_t loopedChannels = 0;
    while(openknx.freeLoopTime() && loopedChannels < THP_ChannelCount)
    {
        _Sensorchannels[_curLoopChannel]->loop();
        loopedChannels++;
        _curLoopChannel++;
        _curLoopChannel = _curLoopChannel % THP_ChannelCount;
    }
}

void THPSensorModule::loop2()
{   
    // second core
    _HWSensors.Loop();
}



void THPSensorModule::processInputKo(GroupObject &ko)
{
    // there are no global KO, so all ko belong to a channel

    uint16_t asap = ko.asap();
    int16_t channelnumber = (asap - THP_KoOffset ) / THP_KoBlockSize;
    if(channelnumber < THP_ChannelCount)
        if(_Sensorchannels[channelnumber] != nullptr)
            _Sensorchannels[channelnumber]->processInputKo(ko);

}

void THPSensorModule::processBeforeRestart()
{
    
}

void THPSensorModule::processBeforeTablesUnload()
{
    
}

void THPSensorModule::savePower()
{
    
}

bool THPSensorModule::restorePower()
{
    return true;
}

bool THPSensorModule::usesSecCore()
{
    return true;
}