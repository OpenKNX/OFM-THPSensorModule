#include "Sensors.h"


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
    //SERIAL_DEBUG.print("Sensors::loop(): ");
    //SERIAL_DEBUG.println(m_Sensorchannel_next);
    m_Sensorchannels[m_Sensorchannel_next]->Loop();
    m_Sensorchannel_next++;
    if(m_Sensorchannel_next == THP_ChannelCount)
        m_Sensorchannel_next = 0;
}