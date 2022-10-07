#include "HWSensorchannel_BME280.h"
#include <Wire.h>

HWSensorchannel_BME280::HWSensorchannel_BME280()
{
    
}

void HWSensorchannel_BME280::Setup(uint8_t pin0, uint8_t pin1)
{
    HWSensorchannel::Setup(pin0, pin1);
    if(m_pin0 == 13 || m_pin0 == 17 || m_pin0 == 21 || m_pin0 == 25 || m_pin0 == 29)
    {
        m_Wire = &Wire;
    }
    else
    {
        m_Wire = &Wire1;
    }

    m_bme280 = new ForcedClimate(*m_Wire);
}

bool HWSensorchannel_BME280::Loop()
{
    //Serial.print("HWSensorchannel_BME280::Loop ");
    //Serial.print(m_pin0);
    //Serial.print(" ");
    //Serial.println(m_state);


    switch(m_state)
    {
        case 0:
            if(millis() - m_lastexec > POLL_INTERVALL)
            {
                m_Wire->setSDA(m_pin1);
                m_Wire->setSCL(m_pin0);
                m_bme280->begin();
                m_state = 1;
                m_lastexec = millis();
            }
            return false;
        break;

        case 1:
            if(millis() - m_lastexec > 0)      // no waiting
            {
                readTempHumidity1();
                m_state = 2;
                m_lastexec = millis();
            }
            return false;
        break;

        case 2:
            if(millis() - m_lastexec > 0)      // no waiting
            {
                readTempHumidity2();
                m_state = 3;
                m_lastexec = millis();
            }
            return false;
        break;

        case 3:
            if(millis() - m_lastexec > 0)      // no waiting
            {
                readTempHumidity3();
                m_Wire->end();
                m_state = 0;
                m_lastexec = millis();
                return true;
            }
            return false;
        break;
    }

    return false;
}

void HWSensorchannel_BME280::LoopMax()
{

}


boolean HWSensorchannel_BME280::readTempHumidity1(void)
{
    //Serial.println("HWSensorchannel_BME280::readTempHumidity1");

    SetTemperature(m_bme280->getTemperatureCelcius(true));
    return true;
}

boolean HWSensorchannel_BME280::readTempHumidity2(void)
{
    //Serial.println("HWSensorchannel_BME280::readTempHumidity2");

    SetHumidity(m_bme280->getRelativeHumidity());
    return true;
}


boolean HWSensorchannel_BME280::readTempHumidity3(void)
{
    //Serial.println("HWSensorchannel_BME280::readTempHumidity3: ");

    SetPressure(m_bme280->getPressure());
    return true;
}



