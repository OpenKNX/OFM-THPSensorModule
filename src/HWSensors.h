#pragma once

#include "HWSensorchannel.h"



class HWSensors
{
    //members
    HWSensorchannel *m_HWSensorchannels[THP_ChannelCount];
    uint8_t m_Loop_i = 0;


    //constructors
    public: HWSensors();

	
    public: void Setup(const uint8_t pins[], uint8_t sensortypes[]);
	
	public: void Loop();

    public: void LoopMax();

    public: float GetTemperature(uint8_t channel);
    public: float GetHumidity(uint8_t channel);
    public: float GetPressure(uint8_t channel);

    HWSensorchannel* CreateHWSensorchannel(uint8_t sensortype);
};