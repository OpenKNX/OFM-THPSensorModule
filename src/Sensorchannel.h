#pragma once

#include <Arduino.h>
#include "HWSensors.h"
#include "knx.h"

#define GetTempKO()  knx.getGroupObject(THP_KoSensorTemp_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetTempAlarmLKO()  knx.getGroupObject(THP_KoSensorTempAlarmL_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetTempAlarmHKO()  knx.getGroupObject(THP_KoSensorTempAlarmH_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetTempMinValueKO()  knx.getGroupObject(THP_KoSensorTempMinValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetTempMaxValueKO()  knx.getGroupObject(THP_KoSensorTempMaxValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetTempMinMaxResetKO()  knx.getGroupObject(THP_KoSensorTempMinMaxReset_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)

#define GetHumKO()  knx.getGroupObject(THP_KoSensorHum_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetHumAlarmLKO()  knx.getGroupObject(THP_KoSensorHumAlarmL_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetHumAlarmHKO()  knx.getGroupObject(THP_KoSensorHumAlarmH_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetHumMinValueKO()  knx.getGroupObject(THP_KoSensorHumMinValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetHumMaxValueKO()  knx.getGroupObject(THP_KoSensorHumMaxValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetHumMinMaxResetKO()  knx.getGroupObject(THP_KoSensorHumMinMaxReset_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)

#define GetAbsHumKO()  knx.getGroupObject(THP_KoSensorAbsHum_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetAbsHumAlarmLKO()  knx.getGroupObject(THP_KoSensorAbsHumAlarmL_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetAbsHumAlarmHKO()  knx.getGroupObject(THP_KoSensorAbsHumAlarmH_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetAbsHumMinValueKO()  knx.getGroupObject(THP_KoSensorAbsHumMinValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetAbsHumMaxValueKO()  knx.getGroupObject(THP_KoSensorAbsHumMaxValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetAbsHumMinMaxResetKO()  knx.getGroupObject(THP_KoSensorAbsHumMinMaxReset_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)

#define GetDewPointKO()  knx.getGroupObject(THP_KoSensorDewPoint_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetDewPointAlarmLKO()  knx.getGroupObject(THP_KoSensorDewPointAlarmL_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetDewPointAlarmHKO()  knx.getGroupObject(THP_KoSensorDewPointAlarmH_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetDewPointMinValueKO()  knx.getGroupObject(THP_KoSensorDewPointMinValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetDewPointMaxValueKO()  knx.getGroupObject(THP_KoSensorDewPointMaxValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetDewPointMinMaxResetKO()  knx.getGroupObject(THP_KoSensorDewPointMinMaxReset_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)

#define GetPressKO()  knx.getGroupObject(THP_KoSensorPress_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetPressAlarmLKO()  knx.getGroupObject(THP_KoSensorPressAlarmL_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetPressAlarmHKO()  knx.getGroupObject(THP_KoSensorPressAlarmH_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetPressMinValueKO()  knx.getGroupObject(THP_KoSensorPressMinValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetPressMaxValueKO()  knx.getGroupObject(THP_KoSensorPressMaxValue_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)
#define GetPressMinMaxResetKO()  knx.getGroupObject(THP_KoSensorPressMinMaxReset_ + (THP_KoBlockSize * m_channelnumber) + THP_KoOffset)




#define ParamTempSendChangeAmount() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorTemperatureSendChangeAmount_, Float_Enc_IEEE754Single)
#define ParamTempSendCycle() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorTemperatureSendCycle_)
#define ParamTempAlign() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorTemperatureAlign_, Float_Enc_IEEE754Single)
#define ParamTempWarnL() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorTemperatureWarnL_, Float_Enc_IEEE754Single)
#define ParamTempWarnH() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorTemperatureWarnH_, Float_Enc_IEEE754Single)
#define ParamTempMinMax() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorTemperatureMinMax_)

#define ParamHumSendChangeAmount() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorHumiditySendChangeAmount_, Float_Enc_IEEE754Single)
#define ParamHumSendCycle() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorHumiditySendCycle_)
#define ParamHumAlign() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorHumidityAlign_, Float_Enc_IEEE754Single)
#define ParamHumWarnL() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorHumidityWarnL_, Float_Enc_IEEE754Single)
#define ParamHumWarnH() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorHumidityWarnH_, Float_Enc_IEEE754Single)
#define ParamHumMinMax() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorHumidityMinMax_)
#define ParamHumDPT() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorHumiditySend_)

#define ParamAbsHumSendChangeAmount() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorAbsHumiditySendChangeAmount_, Float_Enc_IEEE754Single)
#define ParamAbsHumSendCycle() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorAbsHumiditySendCycle_)
#define ParamAbsHumAlign() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorAbsHumidityAlign_, Float_Enc_IEEE754Single)
#define ParamAbsHumWarnL() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorAbsHumidityWarnL_, Float_Enc_IEEE754Single)
#define ParamAbsHumWarnH() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorAbsHumidityWarnH_, Float_Enc_IEEE754Single)
#define ParamAbsHumMinMax() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorAbsHumidityMinMax_)
#define ParamAbsHumDPT() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorAbsHumiditySend_)

#define ParamDewPointSendChangeAmount() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorDewPointSendChangeAmount_, Float_Enc_IEEE754Single)
#define ParamDewPointSendCycle() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorDewPointSendCycle_)
#define ParamDewPointAlign() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorDewPointAlign_, Float_Enc_IEEE754Single)
#define ParamDewPointWarnL() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorDewPointWarnL_, Float_Enc_IEEE754Single)
#define ParamDewPointWarnH() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorDewPointWarnH_, Float_Enc_IEEE754Single)
#define ParamDewPointMinMax() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorDewPointMinMax_)
#define ParamDewPointDPT() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorDewPointSend_)

#define ParamPressSendChangeAmount() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorPressureSendChangeAmount_, Float_Enc_IEEE754Single)
#define ParamPressSendCycle() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorPressureSendCycle_)
#define ParamPressAlign() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorPressureAlign_, Float_Enc_IEEE754Single)
#define ParamPressWarnL() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorPressureWarnL_, Float_Enc_IEEE754Single)
#define ParamPressWarnH() knx.paramFloat(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorPressureWarnH_, Float_Enc_IEEE754Single)
#define ParamPressMinMax() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorPressureMinMax_)
#define ParamPressDPT() knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize * m_channelnumber + THP_SensorPressureSend_)

// converts a relative (to the channel start) KO number to an absolute KO number of the device
#define AbsKO(asap)    (THP_KoOffset + THP_KoBlockSize * m_channelnumber + asap)

// converts a absolute KO number to a relative KO number (offset to the starting KO number of the channel)
#define RelKO(asap)    (asap - THP_KoOffset - THP_KoBlockSize * m_channelnumber)


#define TempKODPT       (Dpt(9,1))
#define HumKODPT        (ParamHumDPT()==5?Dpt(5,1):Dpt(9,7))
#define AbsHumKODPT     Dpt(9,29)
#define DewPointKODPT   Dpt(9,1)
#define PressKODPT      Dpt(9,6)


class Sensorchannel
{
    private:
        uint8_t m_channelnumber;
        HWSensors *m_hwSensors;

        uint32_t m_temperature_last_send_millis = 0;
        float m_temperature_last_send_value = -1000;
        uint32_t m_temperature_alarmH_last_send_millis = 0;
        uint32_t m_temperature_alarmL_last_send_millis = 0;

        uint32_t m_humidity_last_send_millis = 0;
        float m_humidity_last_send_value = -1000;
        uint32_t m_humidity_alarmH_last_send_millis = 0;
        uint32_t m_humidity_alarmL_last_send_millis = 0;

        uint32_t m_abshumidity_last_send_millis = 0;
        float m_abshumidity_last_send_value = -1000;
        uint32_t m_abshumidity_alarmH_last_send_millis = 0;
        uint32_t m_abshumidity_alarmL_last_send_millis = 0;

        uint32_t m_dewpoint_last_send_millis = 0;
        float m_dewpoint_last_send_value = -1000;
        uint32_t m_dewpoint_alarmH_last_send_millis = 0;
        uint32_t m_dewpoint_alarmL_last_send_millis = 0;

        uint32_t m_pressure_last_send_millis = 0;
        float m_pressure_last_send_value = -1000;
        uint32_t m_pressure_alarmH_last_send_millis = 0;
        uint32_t m_pressure_alarmL_last_send_millis = 0;

        float CalcDewPoint(float relative_humidity, float temperature);
        float CalcAbsHumidity(float relative_humidity, float temperature);
    
    public:
        Sensorchannel();
        void Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number, HWSensors *HWSensors);
	    void loop();
        void processInputKo(GroupObject& ko);
};