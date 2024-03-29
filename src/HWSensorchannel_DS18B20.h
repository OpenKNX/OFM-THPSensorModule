#pragma once

#include <Arduino.h>
#include "HWSensorchannel.h"
#include "one_wire.h"

#define POLL_INTERVALL 2000


class HWSensorchannel_DS18B20 : public HWSensorchannel
{
    private:
        One_wire *m_Wire;
        One_wire *m_Wire2;
        rom_address_t m_address{};
        rom_address_t m_address2{};
        uint8_t m_state = 0;
        uint8_t m_state2 = 0;
        uint32_t m_lastexec = 0;
        uint32_t m_lastexec2 = 0;

    public:
        HWSensorchannel_DS18B20();
        void Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number);
        bool Loop();

    private:
        void init(void);
};