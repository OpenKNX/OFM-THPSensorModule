#include "THPSensorModule.h"

THPSensorModule *THPSensorModule::_instance = nullptr;

THPSensorModule::THPSensorModule(const uint8_t* gpioPins)
{
    THPSensorModule::_instance = this;
    _gpioPins = gpioPins;
}

THPSensorModule *THPSensorModule::instance()
{
    return THPSensorModule::_instance;
}

const std::string THPSensorModule::name()
{
    return "THPSensor";
}

const std::string THPSensorModule::version()
{
    return "0.1dev";
}

void THPSensorModule::setup()
{
    logTraceP("setup");

    uint8_t sensortypes[THP_ChannelCount];
        for(int i=0;i<THP_ChannelCount;i++)
            sensortypes[i] = knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize*i+THP_Sensortype_);

    _HWSensors.Setup(_gpioPins, sensortypes);

    for (uint8_t i = 0; i < THP_ChannelCount; i++)
    {
        _Sensorchannels[i] = new Sensorchannel();
        _Sensorchannels[i]->Setup(_gpioPins[i*2], _gpioPins[(i*2)+1], i, &_HWSensors);
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

bool THPSensorModule::usesDualCore()
{
    return true;
}