#include "HWSensorchannel_DS18B20.h"
#include <Wire.h>

HWSensorchannel_DS18B20::HWSensorchannel_DS18B20()
{
    
}

void HWSensorchannel_DS18B20::Setup(uint8_t pin0, uint8_t pin1)
{
    HWSensorchannel::Setup(pin0, pin1);
    m_Wire = new One_wire(pin0);
    init();
}

bool HWSensorchannel_DS18B20::Loop()
{
    Serial.print("HWSensorchannel_DS18B20::Loop ");
    Serial.print(m_pin0);
    Serial.print(" ");
    Serial.println(m_state);

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
                Serial.print("step1: ");
                float stemp = m_Wire->temperature(m_address);     // 10ms
                Serial.println(stemp);
                SetTemperature(stemp);
                m_state = 0;
                m_lastexec = millis();
            }
        break;
    }
    //Serial.println("endloop");
    return true;
}

void HWSensorchannel_DS18B20::LoopMax()
{

}

void HWSensorchannel_DS18B20::init(void)
{
  m_Wire->init();
  m_Wire->single_device_read_rom(m_address);
  m_Wire->set_resolution(m_address, 12);
}

