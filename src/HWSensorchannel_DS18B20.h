#pragma once

#include <Arduino.h>
#include "HWSensorchannel.h"
#include "one_wire.h"

#define POLL_INTERVALL 1000


class HWSensorchannel_DS18B20 : public HWSensorchannel
{
    private:
        One_wire *m_Wire;
        rom_address_t m_address{};
        uint8_t m_state = 0;
        uint32_t m_lastexec = 0;

    public:
        HWSensorchannel_DS18B20();
        void Setup(uint8_t pin0, uint8_t pin1);
        bool Loop();
        void LoopMax();

    private:
        void init(void);
};