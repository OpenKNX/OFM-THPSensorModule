#pragma once

#include <Arduino.h>
#include "HWSensorchannel.h"

#define SINGLE_SENSOR

#include <stdio.h>
#include "pico/stdio.h"

#include "platform/OneWireNg_PicoRP2040PIO.h"
#include "drivers/DSTherm.h"
#include "utils/Placeholder.h"
#include "platform/Platform_Delay.h"
#include "DallasTemperature.h"

#define POLL_INTERVALL 2000


class HWSensorchannel_DS18B20 : public HWSensorchannel
{
    private:
        OneWireNg_PicoRP2040PIO* m_ow0 = nullptr;
        OneWireNg_PicoRP2040PIO* m_ow1 = nullptr;
        bool m_first_sensor = true;
        uint8_t m_state = 0;
        uint8_t m_state2 = 0;
        uint32_t m_lastexec = 0;
        uint32_t m_lastexec2 = 0;

    public:
        HWSensorchannel_DS18B20();
        void Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number);
        bool Loop();
};