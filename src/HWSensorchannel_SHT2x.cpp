#include "HWSensorchannel_SHT2x.h"
#include <Wire.h>

HWSensorchannel_SHT2x::HWSensorchannel_SHT2x()
{
    
}

void HWSensorchannel_SHT2x::Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number)
{
    HWSensorchannel::Setup(pin0, pin1, channel_number);
    if(m_pin0 == 13 || m_pin0 == 17 || m_pin0 == 21 || m_pin0 == 25 || m_pin0 == 29)
    {
        m_Wire = &Wire;
    }
    else
    {
        m_Wire = &Wire1;
    }
}

bool HWSensorchannel_SHT2x::Loop()
{
    //Serial.print("HWSensorchannel_SHT2x::Loop ");
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
                begin();
                m_state = 1;
                m_lastexec = millis();
            }
            return false;
        break;

        case 1:
            if(millis() - m_lastexec > 15)      // 15ms after reset
            {
                readTempHumidity1();
                m_state = 2;
                m_lastexec = millis();
            }
            return false;
        break;

        case 2:
            if(millis() - m_lastexec > 30)      // 30ms for hum measurement
            {
                readTempHumidity2();
                m_state = 3;
                m_lastexec = millis();
            }
            return false;
        break;

        case 3:
            if(millis() - m_lastexec > 90)      // 90ms for temp measurement
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

void HWSensorchannel_SHT2x::LoopMax()
{

}


boolean HWSensorchannel_SHT2x::begin()
{
    m_Wire->begin();
    reset();
    return true;
}

void HWSensorchannel_SHT2x::writeCommand(uint8_t cmd)
{
    m_Wire->beginTransmission(SHT21_I2C_ADDR);
    m_Wire->write(cmd);
    m_Wire->endTransmission();  
}

uint8_t HWSensorchannel_SHT2x::crc8(const uint8_t *data, int len)
{
    const uint16_t POLYNOMIAL = 0x131;  // P(x)=x^8+x^5+x^4+1 = 100110001
    uint8_t crc = 0;	
  	uint8_t byteCtr;

 	 //calculates 8-Bit checksum with given polynomial
  	for (byteCtr = 0; byteCtr < len; ++byteCtr)
 	{
        crc ^= (data[byteCtr]);
 	    for (uint8_t bit = 8; bit > 0; --bit)
 	    {
            if (crc & 0x80)
                crc = (crc << 1) ^ POLYNOMIAL;
 	        else
                crc = (crc << 1);
 	    }
 	}
    return crc;
}


boolean HWSensorchannel_SHT2x::readTempHumidity1(void)
{
    //Serial.println("HWSensorchannel_SHT2x::readTempHumidity1");
    writeCommand(SHT21__RH_MEASUREMENT_NHM);
    return true;
}

boolean HWSensorchannel_SHT2x::readTempHumidity2(void)
{
    //Serial.println("HWSensorchannel_SHT2x::readTempHumidity2");
    uint8_t readbuffer[3];

    m_Wire->requestFrom(SHT21_I2C_ADDR, (uint8_t)3);
    if (m_Wire->available() != 3) 
        return false;
    for (uint8_t i=0; i<3; i++)
    {
        readbuffer[i] = m_Wire->read();
    }

	uint16_t result = (readbuffer[0] << 8);
	result += readbuffer[1];

    if (readbuffer[2] != crc8(readbuffer, 2))
    {
        Serial.println("HWSensorchannel_SHT2x::readTempHumidity2: CRC Error");
        return false;
    }

    double shum = result;
    shum *= 100;
    shum /= 0xFFFF;
    Serial.println(shum);
    SetHumidity(shum);

    writeCommand(SHT21__T_MEASUREMENT_NHM);

    return true;
}


boolean HWSensorchannel_SHT2x::readTempHumidity3(void)
{
    //Serial.println("HWSensorchannel_SHT2x::readTempHumidity3: ");
    uint8_t readbuffer[3];

    m_Wire->requestFrom(SHT21_I2C_ADDR, (uint8_t)3);
    if (m_Wire->available() != 3)
    {
        Serial.println("HWSensorchannel_SHT2x::readTempHumidity3: no data");
        return false;
    }
    for (uint8_t i=0; i<3; i++)
    {
        readbuffer[i] = m_Wire->read();
    }

	uint16_t result = (readbuffer[0] << 8);
	result += readbuffer[1];

        if (readbuffer[2] != crc8(readbuffer, 2))
    {
        Serial.println("HWSensorchannel_SHT2x::readTempHumidity3: CRC Error");
        return false;
    }

    double stemp = result;
    stemp *= 175;
    stemp /= 0xffff;
    stemp = -45 + stemp;
    SetTemperature(stemp);
    //Serial.print("#");
    //Serial.print(stemp);
    //Serial.println("#");


    return true;
}

void HWSensorchannel_SHT2x::reset(void)
{
    writeCommand(SHT21_SOFTRESET);
}

