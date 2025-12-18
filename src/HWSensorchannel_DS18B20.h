#pragma once

#include <Arduino.h>
#include "HWSensorchannel.h"

#define SINGLE_SENSOR

#include <stdio.h>
#include "pico/stdio.h"

#include "OneWireNg_CurrentPlatform.h"
#include "drivers/DSTherm.h"
#include "utils/Placeholder.h"
#include "platform/Platform_Delay.h"
#include "DallasTemperature.h"

#define POLL_INTERVALL 2000


class HWSensorchannel_DS18B20 : public HWSensorchannel
{
    private:
        OneWireNg_CurrentPlatform* m_ow = nullptr;
        DSTherm* m_drv = nullptr;
        Placeholder<DSTherm::Scratchpad> m_scrpd;
        //Placeholder<OneWireNg_CurrentPlatform> m_ow2;
        
        bool m_first_sensor = true;
        uint8_t m_state = 0;
        uint8_t m_state2 = 0;
        uint32_t m_lastexec = 0;
        uint32_t m_lastexec2 = 0;

        void pollSensor(uint8_t pin);
        float readSensor(uint8_t pin);

    public:
        HWSensorchannel_DS18B20();
        void Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number);
        bool Loop();
};