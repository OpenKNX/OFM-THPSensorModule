#pragma once

#include <Arduino.h>
#include "HWSensorchannel.h"
#include <Wire.h>

#define POLL_INTERVALL 1000

/*
#define SHT31_I2C_ADDR			   0x44
#define SHT31_MEAS_HIGHREP_STRETCH 0x2C06
#define SHT31_MEAS_MEDREP_STRETCH  0x2C0D
#define SHT31_MEAS_LOWREP_STRETCH  0x2C10
#define SHT31_MEAS_HIGHREP         0x2400
#define SHT31_MEAS_MEDREP          0x240B
#define SHT31_MEAS_LOWREP          0x2416
#define SHT31_READSTATUS           0xF32D
#define SHT31_CLEARSTATUS          0x3041
#define SHT31_SOFTRESET            0x30A2
#define SHT31_HEATEREN             0x306D
#define SHT31_HEATERDIS            0x3066
*/




const uint16_t POLYNOMIAL = 0x131;  // P(x)=x^8+x^5+x^4+1 = 100110001

#define SHT21_I2C_ADDR 0x40	// I2C device address
#define SHT21__T_MEASUREMENT_NHM 0XF3  // command trig. temp meas. no hold master
#define SHT21__RH_MEASUREMENT_NHM 0XF5 // command trig. hum. meas. no hold master
#define SHT21_SOFTRESET 0XFE                 // command soft reset





class HWSensorchannel_SHT2x : public HWSensorchannel
{
    private:
        TwoWire *m_Wire;
        uint8_t m_state = 0;
        uint32_t m_lastexec = 0;

    public:
        HWSensorchannel_SHT2x();
        void Setup(uint8_t pin0, uint8_t pin1);
        bool Loop();
        void LoopMax();

    private:
        void writeCommand(uint8_t cmd);
        boolean begin();
        uint8_t crc8(const uint8_t *data, int len);
        void reset(void);
        boolean readTempHumidity1(void);
        boolean readTempHumidity2(void);
        boolean readTempHumidity3(void);
};