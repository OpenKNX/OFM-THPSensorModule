#pragma once

#include "Sensorchannel.h"
#include <knx.h>
#include "HWSensors.h"
#include "knxprod.h"


class Sensors
{
    //members
    private:
        Sensorchannel *m_Sensorchannels[THP_ChannelCount];
        uint8_t m_Sensorchannel_next;
        HWSensors *m_hwsensors;


    public:    
        void processReadRequests();
        void processInputKo(GroupObject &iKo);
        bool processDiagnoseCommand(char *iBuffer);
        void debug();
        void setup(const uint8_t outpins[], HWSensors *HWSensors);
        void loop();
};