#include "HWSensorchannel_DS18B20.h"
#include <Wire.h>

HWSensorchannel_DS18B20::HWSensorchannel_DS18B20()
{
    
}

void HWSensorchannel_DS18B20::Setup(uint8_t pin0, uint8_t pin1)
{
    HWSensorchannel::Setup(pin0, pin1);
    m_Wire = new One_wire(pin0);
    m_Wire2 = new One_wire(pin1);
    init();
}

bool HWSensorchannel_DS18B20::Loop()
{
    // proceed with the "older" one
    if(m_lastexec < m_lastexec2)
    {
        // sensor 1
        switch(m_state)
        {
            case 0:
                if(millis() - m_lastexec > POLL_INTERVALL)
                {
                    m_Wire->convert_temperature(m_address, false, false);   // ~6ms
                    m_state = 1;
                    m_lastexec = millis();
                }
            break;

            case 1:
                if(millis() - m_lastexec > 750)
                {
                    float stemp = m_Wire->temperature(m_address);     // 10ms
                    SetTemperature(stemp);
                    m_state = 0;
                    m_lastexec = millis();
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
                if(millis() - m_lastexec2 > POLL_INTERVALL)
                {
                    m_Wire2->convert_temperature(m_address2, false, false);   // ~6ms
                    m_state2 = 1;
                    m_lastexec2 = millis();
                }
            break;

            case 1:
                if(millis() - m_lastexec2 > 750)
                {
                    float stemp = m_Wire2->temperature(m_address2);     // 10ms
                    SetHumidity(stemp); // For Temp2, the Humidty KO and Params are used
                    m_state2 = 0;
                    m_lastexec2 = millis();
                }
            break;
        }
    }



    return true;
}

void HWSensorchannel_DS18B20::init(void)
{
  m_Wire->init();
  m_Wire->single_device_read_rom(m_address);
  m_Wire->set_resolution(m_address, 12);

  m_Wire2->init();
  m_Wire2->single_device_read_rom(m_address2);
  m_Wire2->set_resolution(m_address2, 12);
}

