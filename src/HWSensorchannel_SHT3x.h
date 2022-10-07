#pragma once

#include <Arduino.h>
#include "HWSensorchannel.h"
#include <Wire.h>

#define POLL_INTERVALL 1000


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



class HWSensorchannel_SHT3x : public HWSensorchannel
{
    private:
        TwoWire *m_Wire;
        uint8_t m_state = 0;
        uint32_t m_lastexec = 0;

    public:
        HWSensorchannel_SHT3x();
        void Setup(uint8_t pin0, uint8_t pin1);
        bool Loop();
        void LoopMax();

    private:
        void writeCommand(uint16_t cmd);
        boolean begin();
        uint8_t crc8(const uint8_t *data, int len);
        void reset(void);
        void readTempHumidity1(void);
        boolean readTempHumidity2(void);
};