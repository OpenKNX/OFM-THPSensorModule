#include "HWSensorchannel.h"
#include <Wire.h>

HWSensorchannel::HWSensorchannel()
{
    mutex_init(&mx1);
    mutex_init(&mx2);
    mutex_init(&mx3);
}

void HWSensorchannel::Setup(uint8_t pin0, uint8_t pin1)
{
    m_pin0 = pin0;
    m_pin1 = pin1;
}

float HWSensorchannel::GetTemperature()
{
    float retval;
    if(mutex_try_enter(&mx1, nullptr))
    {
        retval = m_temperature;
        mutex_exit(&mx1);
        return retval;
    }
    else
    {
        return NAN;
    }
}

float HWSensorchannel::GetHumidity()
{
    float retval;
    if(mutex_try_enter(&mx2, nullptr))
    {
        retval = m_humidity;
        mutex_exit(&mx2);
        return retval;
    }
    else
    {
        return NAN;
    }
}

float HWSensorchannel::GetPressure()
{
    float retval;
    if(mutex_try_enter(&mx3, nullptr))
    {
        retval = m_pressure;
        mutex_exit(&mx3);
        return retval;
    }
    else
    {
        return NAN;
    }
}

void HWSensorchannel::SetTemperature(float temperature)
{
    mutex_enter_blocking(&mx1);
    m_temperature = temperature;
    mutex_exit(&mx1);
}

void HWSensorchannel::SetHumidity(float humidity)
{
    mutex_enter_blocking(&mx2);
    m_humidity = humidity;
    mutex_exit(&mx2);
}

void HWSensorchannel::SetPressure(float pressure)
{
    mutex_enter_blocking(&mx3);
    m_pressure = pressure;
    mutex_exit(&mx3);
}