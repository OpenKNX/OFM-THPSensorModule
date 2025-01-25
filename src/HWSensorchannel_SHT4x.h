#pragma once

#include <Arduino.h>
#include "HWSensorchannel.h"
#include <Wire.h>

#define POLL_INTERVALL 1000


#define SHT41_I2C_ADDR			   0x44


#define SHT4x_NOHEAT_HIGHPRECISION                                             \
  0xFD /**< High precision measurement, no heater */
#define SHT4x_NOHEAT_MEDPRECISION                                              \
  0xF6 /**< Medium precision measurement, no heater */
#define SHT4x_NOHEAT_LOWPRECISION                                              \
  0xE0 /**< Low precision measurement, no heater */

#define SHT4x_HIGHHEAT_1S                                                      \
  0x39 /**< High precision measurement, high heat for 1 sec */
#define SHT4x_HIGHHEAT_100MS                                                   \
  0x32 /**< High precision measurement, high heat for 0.1 sec */
#define SHT4x_MEDHEAT_1S                                                       \
  0x2F /**< High precision measurement, med heat for 1 sec */
#define SHT4x_MEDHEAT_100MS                                                    \
  0x24 /**< High precision measurement, med heat for 0.1 sec */
#define SHT4x_LOWHEAT_1S                                                       \
  0x1E /**< High precision measurement, low heat for 1 sec */
#define SHT4x_LOWHEAT_100MS                                                    \
  0x15 /**< High precision measurement, low heat for 0.1 sec */

#define SHT4x_READSERIAL 0x89 /**< Read Out of Serial Register */
#define SHT4x_SOFTRESET 0x94  /**< Soft Reset */



class HWSensorchannel_SHT4x : public HWSensorchannel
{
    private:
        TwoWire *m_Wire;
        uint8_t m_state = 0;
        uint32_t m_lastexec = 0;

    public:
        HWSensorchannel_SHT4x();
        void Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number);
        bool Loop();
        void LoopMax();

    private:
        void writeCommand(uint8_t cmd);
        boolean begin();
        uint8_t crc8(const uint8_t *data, int len);
        void reset(void);
        void readTempHumidity1(void);
        boolean readTempHumidity2(void);
};