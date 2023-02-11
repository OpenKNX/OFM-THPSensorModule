#include "HWSensors.h"
#include "HWSensorchannel_SHT3x.h"
#include "HWSensorchannel_SHT2x.h"
#include "HWSensorchannel_DS18B20.h"
#include "HWSensorchannel_BME280.h"


HWSensors::HWSensors()
{
  
}

void HWSensors::Setup(const uint8_t outpins[], uint8_t sensortypes[])
{
    for(int i = 0;i<THP_ChannelCount;i++)
    {
        m_HWSensorchannels[i] = CreateHWSensorchannel(sensortypes[i]);

        if(m_HWSensorchannels[i] != nullptr)
            m_HWSensorchannels[i]->Setup(outpins[i*2],outpins[(i*2)+1]);
    }
}

HWSensorchannel* HWSensors::CreateHWSensorchannel(uint8_t sensortype)
{
    switch(sensortype)  // AddSensor
    {
        case 0:
            return nullptr;
        case 1:
            return new HWSensorchannel_SHT3x();
        case 2:
            return new HWSensorchannel_SHT2x();
        case 3:
            return new HWSensorchannel_BME280();
        case 4:
            return new HWSensorchannel_DS18B20();
        default:
            return nullptr;
    }
}

void HWSensors::Loop()
{
    //SERIAL_DEBUG.print("HWSensors::loop(): ");
    //query one sensor at a time
    bool nextsensor = true;
    if(m_HWSensorchannels[m_Loop_i] != nullptr)
    {
        //SERIAL_DEBUG.println(m_Loop_i);
        nextsensor = m_HWSensorchannels[m_Loop_i]->Loop();
    }
    if(nextsensor)
    {
        //SERIAL_DEBUG.println("HWSensors::loop(): next");
        m_Loop_i++;
        if(m_Loop_i >= THP_ChannelCount)
            m_Loop_i = 0;
    }
}

float HWSensors::GetTemperature(uint8_t channel)
{
    if(m_HWSensorchannels[channel] != nullptr)
    {
        return m_HWSensorchannels[channel]->GetTemperature();
    }
    else
        return NAN;
}

float HWSensors::GetHumidity(uint8_t channel)
{
    if(m_HWSensorchannels[channel] != nullptr)
    {
        return m_HWSensorchannels[channel]->GetHumidity();
    }
    else
        return NAN;
}

float HWSensors::GetPressure(uint8_t channel)
{
    if(m_HWSensorchannels[channel] != nullptr)
    {
        return m_HWSensorchannels[channel]->GetPressure();
    }
    else
        return NAN;
}
