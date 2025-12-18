#include "HWSensorchannel_DS18B20.h"


HWSensorchannel_DS18B20::HWSensorchannel_DS18B20()
{
    
}


void HWSensorchannel_DS18B20::Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number)
{
    HWSensorchannel::Setup(pin0, pin1, channel_number);
}

bool HWSensorchannel_DS18B20::Loop()
{
    uint32_t new_millis = millis();
    if(m_first_sensor) // sensor 1 or 2 will be toggled each call !
    {
        // sensor 1
        switch(m_state)
        {
            case 0:
                if(new_millis - m_lastexec > POLL_INTERVALL)
                {
                    pollSensor(m_pin0);
                    m_state = 1;
                    m_lastexec = new_millis;
                }
            break;

            case 1:
                if(new_millis - m_lastexec > 750)
                {
                    SetTemperature(readSensor(m_pin0));
                    m_state = 0;
                    m_lastexec = new_millis;
                }
            break;
        }
    }
    else
    {
        // sensor 2
        switch(m_state2)
        {
            case 0:
                if(new_millis - m_lastexec2 > POLL_INTERVALL)
                {
                    pollSensor(m_pin1);
                    m_state2 = 1;
                    m_lastexec2 = new_millis;
                }
            break;

            case 1:
                if(new_millis - m_lastexec2 > 750)
                {
                    SetHumidity(readSensor(m_pin1));
                    m_state2 = 0;
                    m_lastexec2 = new_millis;
                }
            break;
        }
    }
    m_first_sensor = !m_first_sensor;

    return true; // true = in the hardware sensor loop, go to next sensor
}

void HWSensorchannel_DS18B20::pollSensor(uint8_t pin)
{
    OneWireNg_CurrentPlatform ow(pin, false);
    DSTherm drv(ow);

    drv.convertTempAll(0, false);
}

float HWSensorchannel_DS18B20::readSensor(uint8_t pin)
{
    OneWireNg_CurrentPlatform ow(pin, false);
    DSTherm drv(ow);
    Placeholder<DSTherm::Scratchpad> scrpd;

    OneWireNg::ErrorCode ec = drv.readScratchpadSingle(scrpd);
    if (ec == OneWireNg::EC_SUCCESS)
    {
        long temp = scrpd->getTemp2();
        return ((float)temp / 16);
    }
    else if (ec == OneWireNg::EC_CRC_ERROR)
    {
        logDebugP("OneWire-CRC-Error Pin %d", pin);
    }
    else
    {
        logDebugP("OneWire-Error Pin %d", pin);
    }
    return NAN;
}