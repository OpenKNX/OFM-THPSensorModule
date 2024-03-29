#pragma once

#include <Arduino.h>
#include <pico/sync.h>
#include "OpenKNX.h"



class HWSensorchannel: public OpenKNX::Channel
{
    protected:
        uint8_t m_pin0 = 0;
        uint8_t m_pin1 = 0;
        mutex_t mx1;
        mutex_t mx2;
        mutex_t mx3;

        float m_temperature = NAN;
        float m_humidity = NAN;
        float m_pressure = NAN;

    public:
        HWSensorchannel();
        virtual void Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number);
	    virtual bool Loop() = 0;

        float GetTemperature();
        float GetHumidity();
        float GetPressure();

        void SetTemperature(float temperature);
        void SetHumidity(float humidity);
        void SetPressure(float pressure);

        const std::string name() override;
};