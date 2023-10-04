#include "HWSensorchannel_SHT3x.h"
#include <Wire.h>

HWSensorchannel_SHT3x::HWSensorchannel_SHT3x()
{
    
}

void HWSensorchannel_SHT3x::Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number)
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

bool HWSensorchannel_SHT3x::Loop()
{
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
            if(millis() - m_lastexec > 10)
            {
                readTempHumidity1();
                m_state = 2;
                m_lastexec = millis();
            }
            return false;
        break;

        case 2:
            if(millis() - m_lastexec > 20)
            {
                readTempHumidity2();
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

void HWSensorchannel_SHT3x::LoopMax()
{

}


boolean HWSensorchannel_SHT3x::begin()
{
    m_Wire->begin();
    reset();
    return true;
}

void HWSensorchannel_SHT3x::writeCommand(uint16_t cmd)
{
    m_Wire->beginTransmission(SHT31_I2C_ADDR);
    m_Wire->write(cmd >> 8);
    m_Wire->write(cmd & 0xFF);
    m_Wire->endTransmission();  
}

uint8_t HWSensorchannel_SHT3x::crc8(const uint8_t *data, int len)
{
    const uint8_t POLYNOMIAL(0x31);
    uint8_t crc(0xFF);

    for ( int c = len; c; --c ) {
        crc ^= *data++;

        for ( int i = 8; i; --i ) {
            crc = ( crc & 0x80 )
            ? (crc << 1) ^ POLYNOMIAL
            : (crc << 1);
        }
    }
    return crc;
}

void HWSensorchannel_SHT3x::readTempHumidity1(void)
{
    writeCommand(SHT31_MEAS_HIGHREP);
}

boolean HWSensorchannel_SHT3x::readTempHumidity2(void)
{
    uint8_t readbuffer[6];

    m_Wire->requestFrom(SHT31_I2C_ADDR, (uint8_t)6);
    if (m_Wire->available() != 6) 
        return false;
    for (uint8_t i=0; i<6; i++)
    {
        readbuffer[i] = m_Wire->read();
    }
    uint16_t ST, SRH;
    ST = readbuffer[0];
    ST <<= 8;
    ST |= readbuffer[1];

    if (readbuffer[2] != crc8(readbuffer, 2)) return false;

    SRH = readbuffer[3];
    SRH <<= 8;
    SRH |= readbuffer[4];

    if (readbuffer[5] != crc8(readbuffer+3, 2)) return false;

    double stemp = ST;
    stemp *= 175;
    stemp /= 0xffff;
    stemp = -45 + stemp;
    SetTemperature(stemp);

    double shum = SRH;
    shum *= 100;
    shum /= 0xFFFF;

    SetHumidity(shum);

    return true;
}

void HWSensorchannel_SHT3x::reset(void)
{
    writeCommand(SHT31_SOFTRESET);
}

