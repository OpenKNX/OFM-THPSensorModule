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

    // do nothing when not parameterized
    if (!knx.configured())
        return;

    uint8_t loopedChannels = 0;
    while(openknx.freeLoopTime() && loopedChannels < THP_ChannelCount)
    {
        _Sensorchannels[_curLoopChannel]->loop();
        loopedChannels++;
        _curLoopChannel++;
        _curLoopChannel = _curLoopChannel % THP_ChannelCount;
    }
}

void THPSensorModule::loop1()
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

uint16_t THPSensorModule::flashSize()
{
    // Version + Data (Channel * Inputs * (Dpt + Value))
    return 1 + (THP_ChannelCount * 2 * 4);
}

void THPSensorModule::readFlash(const uint8_t *iBuffer, const uint16_t iSize)
{
    if (iSize == 0) // first call - without data
        return;

    uint8_t lVersion = openknx.flash.readByte();
    if (lVersion != 1) // version unknown
    {
        logDebugP("Wrong version of flash data (%i)", lVersion);
        return;
    }

    logDebugP("Reading channel data from flash (%i)", THP_ChannelCount);
    for (uint8_t lIndex = 0; lIndex < THP_ChannelCount; lIndex++)
    {
        //_Sensorchannels[lIndex]->restore(); ToDo
    }
}

void THPSensorModule::writeFlash()
{
    openknx.flash.writeByte(1); // Version
    for (uint8_t lIndex = 0; lIndex < THP_ChannelCount; lIndex++)
    {
        //_Sensorchannels[lIndex]->save(); ToDo
    }
}