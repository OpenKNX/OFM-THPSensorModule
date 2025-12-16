#include "HWSensorchannel_DS18B20.h"


HWSensorchannel_DS18B20::HWSensorchannel_DS18B20()
{
    
}


void HWSensorchannel_DS18B20::Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number)
{
    HWSensorchannel::Setup(pin0, pin1, channel_number);

    OneWireNg_PicoRP2040PIO _ow0(pin0, false);
    OneWireNg_PicoRP2040PIO _ow1(pin1, false);

    DSTherm drv0(_ow0);
    drv0.writeScratchpadAll(0, 0, DSTherm::RES_12_BIT);

    DSTherm drv1(_ow1);
    drv1.writeScratchpadAll(0, 0, DSTherm::RES_12_BIT);

    drv0.~DSTherm();
    drv1.~DSTherm();

    _ow0.~OneWireNg_PicoRP2040PIO();
    _ow1.~OneWireNg_PicoRP2040PIO();
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
                    //logDebugP("sensor %d, state %d state2 %d", m_first_sensor, m_state, m_state2);
                    m_ow0 = new OneWireNg_PicoRP2040PIO(m_pin0, false);
                    DSTherm drv0(*m_ow0);

                    drv0.convertTempAll(0, false);

                    m_state = 1;
                    m_lastexec = new_millis;
                }
                return false;
            break;

            case 1:
                if(new_millis - m_lastexec > 750)
                {
                    //logDebugP("sensor %d, state %d state2 %d", m_first_sensor, m_state, m_state2);
                    DSTherm drv0(*m_ow0);

                    static Placeholder<DSTherm::Scratchpad> scrpd;
                    OneWireNg::ErrorCode ec = drv0.readScratchpadSingle(scrpd);
                    if (ec == OneWireNg::EC_SUCCESS)
                    {
                        long temp = scrpd->getTemp2();
                        SetTemperature((float)temp / 16);
                    }
                    else if (ec == OneWireNg::EC_CRC_ERROR)
                    {
                        logDebugP("CRC-Error 0");
                    }
                    else
                    {
                        logDebugP("Onewire Error 0x%x",ec);
                    }

                    drv0.~DSTherm();
                    m_ow0->~OneWireNg_PicoRP2040PIO();

                    m_state = 0;
                    m_lastexec = new_millis;
                    m_first_sensor = false;
                }
                return false;
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
                    OneWireNg_PicoRP2040PIO _ow1(m_pin1, false);
                    DSTherm drv1(_ow1);

                    drv1.convertTempAll(0, false);

                    drv1.~DSTherm();
                    _ow1.~OneWireNg_PicoRP2040PIO();

                    m_state2 = 1;
                    m_lastexec2 = new_millis;
                }
                return false;
            break;

            case 1:
                if(new_millis - m_lastexec2 > 750)
                {
                    OneWireNg_PicoRP2040PIO _ow1(m_pin1, false);
                    DSTherm drv1(_ow1);

                    static Placeholder<DSTherm::Scratchpad> scrpd;
                    OneWireNg::ErrorCode ec = drv1.readScratchpadSingle(scrpd);
                    if (ec == OneWireNg::EC_SUCCESS)
                    {
                        long temp = scrpd->getTemp2();
                        SetHumidity((float)temp / 16);
                    }
                    else if (ec == OneWireNg::EC_CRC_ERROR)
                    {
                        logDebugP("CRC-Error 1");
                    }
                    else
                    {
                        logDebugP("Onewire Error 0x%x",ec);
                    }

                    drv1.~DSTherm();
                    _ow1.~OneWireNg_PicoRP2040PIO();

                    m_state2 = 0;
                    m_lastexec2 = new_millis;
                    m_first_sensor = true;
                    return true;
                }
                else
                {
                    return false;
                }
            break;
        }
    }

    return false;     // should never be reached
}

