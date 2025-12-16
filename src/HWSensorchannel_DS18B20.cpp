#include "HWSensorchannel_DS18B20.h"


HWSensorchannel_DS18B20::HWSensorchannel_DS18B20()
{
    
}


void HWSensorchannel_DS18B20::Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number)
{
    HWSensorchannel::Setup(pin0, pin1, channel_number);

    //new (&m_ow) OneWireNg_CurrentPlatform(pin0, false);
    //new (&m_ow2) OneWireNg_CurrentPlatform(pin1, false);

    //DSTherm drv(m_ow);
    //drv.writeScratchpadAll(0, 0, DSTherm::RES_12_BIT);

    //DSTherm drv2(m_ow2);
    //drv2.writeScratchpadAll(0, 0, DSTherm::RES_12_BIT);
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
                    m_ow = new OneWireNg_CurrentPlatform(m_pin0, false);
                    m_drv = new DSTherm(*m_ow);
                    m_drv->convertTempAll(0, false);
                    m_state = 1;
                    m_lastexec = new_millis;
                }
            break;

            case 1:
                if(new_millis - m_lastexec > 750)
                {
                    OneWireNg::ErrorCode ec = m_drv->readScratchpadSingle(m_scrpd);
                    if (ec == OneWireNg::EC_SUCCESS)
                    {
                        long temp = m_scrpd->getTemp2();
                        SetTemperature((float)temp / 16);
                    }
                    else if (ec == OneWireNg::EC_CRC_ERROR)
                    {
                        logDebugP("CRC-Error 0");
                    }
                    delete m_drv;
                    delete m_ow;
                    m_state = 0;
                    m_lastexec = new_millis;
                    m_first_sensor = !m_first_sensor;
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
                    m_ow = new OneWireNg_CurrentPlatform(m_pin1, false);
                    m_drv = new DSTherm(*m_ow);
                    m_drv->convertTempAll(0, false);
                    m_state2 = 1;
                    m_lastexec2 = new_millis;
                }
            break;

            case 1:
                if(new_millis - m_lastexec2 > 750)
                {
                    OneWireNg::ErrorCode ec = m_drv->readScratchpadSingle(m_scrpd);
                    if (ec == OneWireNg::EC_SUCCESS)
                    {
                        long temp = m_scrpd->getTemp2();
                        SetHumidity((float)temp / 16);
                    }
                    else if (ec == OneWireNg::EC_CRC_ERROR)
                    {
                        logDebugP("CRC-Error 0");
                    }
                    delete m_drv;
                    delete m_ow;
                    m_state2 = 0;
                    m_lastexec2 = new_millis;
                    m_first_sensor = !m_first_sensor;
                }
            break;
        }
    }


    return true;
}

