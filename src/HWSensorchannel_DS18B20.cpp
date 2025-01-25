#include "HWSensorchannel_DS18B20.h"


HWSensorchannel_DS18B20::HWSensorchannel_DS18B20()
{
    
}


void HWSensorchannel_DS18B20::Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number)
{
    HWSensorchannel::Setup(pin0, pin1, channel_number);

    new (&m_ow) OneWireNg_CurrentPlatform(pin0, false);
    new (&m_ow2) OneWireNg_CurrentPlatform(pin1, false);

    DSTherm drv(m_ow);
    drv.writeScratchpadAll(0, 0, DSTherm::RES_12_BIT);

    DSTherm drv2(m_ow2);
    drv2.writeScratchpadAll(0, 0, DSTherm::RES_12_BIT);
}

bool HWSensorchannel_DS18B20::Loop()
{
    uint32_t new_millis = millis();
    if(m_first_sensor)
    {
        // sensor 1
        switch(m_state)
        {
            case 0:
                if(new_millis - m_lastexec > POLL_INTERVALL)
                {
                    DSTherm drv(m_ow);
                    drv.convertTempAll(0, false);
                    m_state = 1;
                    m_lastexec = new_millis;
                }
            break;

            case 1:
                if(new_millis - m_lastexec > 750)
                {
                    DSTherm drv(m_ow);
                    static Placeholder<DSTherm::Scratchpad> scrpd;
                    OneWireNg::ErrorCode ec = drv.readScratchpadSingle(scrpd);
                    if (ec == OneWireNg::EC_SUCCESS)
                    {
                        long temp = scrpd->getTemp2();
                        SetTemperature((float)temp / 16);
                    }
                    else if (ec == OneWireNg::EC_CRC_ERROR)
                    {
                        logDebugP("CRC-Error 0");
                    }
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
                    DSTherm drv(m_ow2);
                    drv.convertTempAll(0, false);
                    m_state2 = 1;
                    m_lastexec2 = new_millis;
                }
            break;

            case 1:
                if(new_millis - m_lastexec2 > 750)
                {
                    DSTherm drv(m_ow2);
                    static Placeholder<DSTherm::Scratchpad> scrpd;
                    OneWireNg::ErrorCode ec = drv.readScratchpadSingle(scrpd);
                    if (ec == OneWireNg::EC_SUCCESS)
                    {
                        long temp = scrpd->getTemp2();
                        SetHumidity((float)temp / 16);
                    }
                    else if (ec == OneWireNg::EC_CRC_ERROR)
                    {
                        logDebugP("CRC-Error 1");
                    }
                    m_state2 = 0;
                    m_lastexec2 = new_millis;
                }
            break;
        }
    }
    m_first_sensor = !m_first_sensor;

    return true;
}

