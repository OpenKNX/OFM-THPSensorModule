#include "Sensors.h"

/*
HWSensors *Sensors::m_hwsensors;
uint8_t Sensors::m_NoOfSensorchannels = 0;
uint8_t Sensors::m_Sensorchannel_next = 0;
Sensorchannel *Sensors::m_Sensorchannels[NO_OF_SENSORCHANNELS];


void Sensors::Setup(const uint8_t outpins[], uint8_t num_channels, HWSensors *HWSensors)
{
    // Setup the channels here
    Sensors::m_NoOfSensorchannels = num_channels;
    Sensors::m_hwsensors = HWSensors;

    for(int i = 0;i<Sensors::m_NoOfSensorchannels;i++)
    {
        Sensors::m_Sensorchannels[i] = new Sensorchannel();
        Sensors::m_Sensorchannels[i]->Setup(outpins[i*2],outpins[(i*2)+1], i, Sensors::m_hwsensors);
    }
}

void Sensors::Loop(uint8_t mode)
{
    //Serial.println("Sensors::Loop()");

    if(mode == 1)
    {
        Sensors::m_Sensorchannels[Sensors::m_Sensorchannel_next]->Loop();
        Sensors::m_Sensorchannel_next++;
        if(Sensors::m_Sensorchannel_next == Sensors::m_NoOfSensorchannels)
            Sensors::m_Sensorchannel_next = 0;
    }
    else
        for(int i = 0;i<Sensors::m_NoOfSensorchannels;i++)
            Sensors::m_Sensorchannels[i]->Loop();
}

void Sensors::LoopMax()
{
    for(int i = 0;i<Sensors::m_NoOfSensorchannels;i++)
    {
        Sensors::m_Sensorchannels[i]->LoopMax();
    }
}


void Sensors::MinMaxResetKOCallback(GroupObject& ko)
{
    Serial.println(" Sensors::MinMaxResetKOCallback");
    uint16_t channelnumber = (ko.asap() - THP_KoOffset ) / THP_KoBlockSize;     // calculated channelnumber is correct for all MinMaxResets
    if(channelnumber < THP_ChannelCount)
        if(m_Sensorchannels[channelnumber] != nullptr)
            m_Sensorchannels[channelnumber]->MinMaxResetKOCallback(ko);
}
*/

void Sensors::processReadRequests()
{
    // this method is called after startup delay and executes read requests, which should just happen once after startup
    static bool sCalledProcessReadRequests = false;
    if (!sCalledProcessReadRequests)
    {
        /*
        // we go through all IO devices defined as outputs and check for initial read requests
        if (knx.paramByte(PM_ReadLed) & PM_ReadLedMask)
        {
            if ((knx.paramByte(PM_LEDPresence) & PM_LEDPresenceMask) >> PM_LEDPresenceShift == VAL_PM_LedKnx)
                knx.getGroupObject(PM_KoLEDPresence).requestObjectRead();
            if ((knx.paramByte(PM_LEDMove) & PM_LEDMoveMask) >> PM_LEDMoveShift == VAL_PM_LedKnx)
                knx.getGroupObject(PM_KoLEDMove).requestObjectRead();
        }
        */ // ToDo Waldemar fragen
        sCalledProcessReadRequests = true;
    }
}

void Sensors::processInputKo(GroupObject &iKo)
{
    uint16_t lAsap = iKo.asap();

    // process module-global KOs here (there are none for Sensors)

    int16_t channelnumber = (lAsap - THP_KoOffset ) / THP_KoBlockSize;
    if(channelnumber < THP_ChannelCount)
        if(m_Sensorchannels[channelnumber] != nullptr)
            m_Sensorchannels[channelnumber]->processInputKo(iKo);

}

bool Sensors::processDiagnoseCommand(char *iBuffer)
{
    return false;
}

void Sensors::debug()
{
    return;
}

void Sensors::setup(const uint8_t outpins[], HWSensors *HWSensors)
{
    for(int i = 0;i<THP_ChannelCount;i++)
    {
        m_Sensorchannels[i] = new Sensorchannel();
        m_Sensorchannels[i]->Setup(outpins[i*2], outpins[(i*2)+1], i, HWSensors);
        
    }
}

void Sensors::loop()
{

}