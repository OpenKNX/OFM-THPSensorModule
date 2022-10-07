#pragma once

#include <Arduino.h>
#include "HWSensorchannel.h"
#include <Wire.h>
#include "forcedClimate.h"

#define POLL_INTERVALL 1000











class HWSensorchannel_BME280 : public HWSensorchannel
{
    private:
        TwoWire *m_Wire;
        uint8_t m_state = 0;
        uint32_t m_lastexec = 0;
        ForcedClimate *m_bme280;

    public:
        HWSensorchannel_BME280();
        void Setup(uint8_t pin0, uint8_t pin1);
        bool Loop();
        void LoopMax();

    private:
        boolean readTempHumidity1(void);
        boolean readTempHumidity2(void);
        boolean readTempHumidity3(void);
};