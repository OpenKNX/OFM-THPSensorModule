#include "Sensorchannel.h"
#include <knx.h>



Sensorchannel::Sensorchannel()
{
  
}

void Sensorchannel::Setup(uint8_t pin0, uint8_t pin1, uint8_t channel_number, HWSensors *HWSensors)
{
    _channelIndex = channel_number;
    m_hwSensors = HWSensors;

    m_pin0 = pin0;
    m_pin1 = pin1;

    if(ParamTHP_Sensortype_ == 99)  // act as binary input
    {
        logDebugP("Setting INPUT_PULLUP to %d", m_pin0);
        pinMode(m_pin0, INPUT_PULLUP);
        pinMode(m_pin1, INPUT_PULLUP);
    }

    // Initialize Min/Max Ko
    // if save data are in flash, this will be overwritten
    KoTHP_SensorTempMinValue_.valueNoSend((float)1000, TempKODPT);
    KoTHP_SensorTempMaxValue_.valueNoSend((float)-1000, TempKODPT);
    KoTHP_SensorHumMinValue_.valueNoSend((float)1000, HumKODPT);
    KoTHP_SensorHumMaxValue_.valueNoSend((float)-1000, HumKODPT);
    KoTHP_SensorAbsHumMinValue_.valueNoSend((float)1000, AbsHumKODPT);
    KoTHP_SensorAbsHumMaxValue_.valueNoSend((float)-1000, AbsHumKODPT);
    KoTHP_SensorDewPointMinValue_.valueNoSend((float)1000, DewPointKODPT);
    KoTHP_SensorDewPointMaxValue_.valueNoSend((float)-1000, DewPointKODPT);
    KoTHP_SensorPressMinValue_.valueNoSend((float)1000, PressKODPT);
    KoTHP_SensorPressMaxValue_.valueNoSend((float)-1000, PressKODPT);

    
    // Debug

    logDebugP("Setup");
    logDebugP("ParamTHP_Sensortype_                       : %i", ParamTHP_Sensortype_                       );
    
    logDebugP("Temperature Parameters:");
    logDebugP("ParamTHP_SensorTemperatureSendChangeAmount_: %f", ParamTHP_SensorTemperatureSendChangeAmount_);
    logDebugP("ParamTHP_SensorTemperatureSendCycle_       : %i", ParamTHP_SensorTemperatureSendCycle_       );
    logDebugP("ParamTHP_SensorTemperatureAlign_           : %f", ParamTHP_SensorTemperatureAlign_           );
    logDebugP("ParamTHP_SensorTemperatureWarnL_           : %f", ParamTHP_SensorTemperatureWarnL_           );
    logDebugP("ParamTHP_SensorTemperatureWarnH_           : %f", ParamTHP_SensorTemperatureWarnH_           );
    logDebugP("ParamTHP_SensorTemperatureMinMax_          : %i", ParamTHP_SensorTemperatureMinMax_          );
	/*logDebugP("Humidity Parameters:");
    logDebugP("ParamTHP_SensorHumiditySendChangeAmount_   : %f", ParamTHP_SensorHumiditySendChangeAmount_   );
    logDebugP("ParamTHP_SensorHumiditySendCycle_          : %i", ParamTHP_SensorHumiditySendCycle_          );
    logDebugP("ParamTHP_SensorHumidityAlign_              : %f", ParamTHP_SensorHumidityAlign_              );
    logDebugP("ParamTHP_SensorHumidityWarnL_              : %f", ParamTHP_SensorHumidityWarnL_              );
    logDebugP("ParamTHP_SensorHumidityWarnH_              : %f", ParamTHP_SensorHumidityWarnH_              );
    logDebugP("ParamTHP_SensorHumidityMinMax_             : %i", ParamTHP_SensorHumidityMinMax_             );
    logDebugP("ParamTHP_SensorHumiditySend_               : %i", ParamTHP_SensorHumiditySend_               );
    logDebugP("Absolute Humidity Parameters:");
    logDebugP("ParamTHP_SensorAbsHumiditySendChangeAmount_: %f", ParamTHP_SensorAbsHumiditySendChangeAmount_);
    logDebugP("ParamTHP_SensorAbsHumiditySendCycle_       : %i", ParamTHP_SensorAbsHumiditySendCycle_       );
    logDebugP("ParamTHP_SensorAbsHumidityAlign_           : %f", ParamTHP_SensorAbsHumidityAlign_           );
    logDebugP("ParamTHP_SensorAbsHumidityWarnL_           : %f", ParamTHP_SensorAbsHumidityWarnL_           );
    logDebugP("ParamTHP_SensorAbsHumidityWarnH_           : %f", ParamTHP_SensorAbsHumidityWarnH_           );
    logDebugP("ParamTHP_SensorAbsHumidityMinMax_          : %i", ParamTHP_SensorAbsHumidityMinMax_          );
    logDebugP("Dew Point Parameters:");
    logDebugP("ParamTHP_SensorDewPointSendChangeAmount_   : %f", ParamTHP_SensorDewPointSendChangeAmount_   );
    logDebugP("ParamTHP_SensorDewPointSendCycle_          : %i", ParamTHP_SensorDewPointSendCycle_          );
    logDebugP("ParamTHP_SensorDewPointAlign_              : %f", ParamTHP_SensorDewPointAlign_              );
    logDebugP("ParamTHP_SensorDewPointWarnL_              : %f", ParamTHP_SensorDewPointWarnL_              );
    logDebugP("ParamTHP_SensorDewPointWarnH_              : %f", ParamTHP_SensorDewPointWarnH_              );
    logDebugP("ParamTHP_SensorDewPointMinMax_             : %i", ParamTHP_SensorDewPointMinMax_             );
	logDebugP("Pressure Parameters:");
    logDebugP("ParamTHP_SensorPressureSendChangeAmount_   : %f", ParamTHP_SensorPressureSendChangeAmount_   );
    logDebugP("ParamTHP_SensorPressureSendCycle_          : %i", ParamTHP_SensorPressureSendCycle_          );
    logDebugP("ParamTHP_SensorPressureAlign_              : %f", ParamTHP_SensorPressureAlign_              );
    logDebugP("ParamTHP_SensorPressureWarnL_              : %f", ParamTHP_SensorPressureWarnL_              );
    logDebugP("ParamTHP_SensorPressureWarnH_              : %f", ParamTHP_SensorPressureWarnH_              );
    logDebugP("ParamTHP_SensorPressureMinMax_             : %i", ParamTHP_SensorPressureMinMax_             );
	logDebugP("Binary Input 0 Parameters:");
    logDebugP("ParamTHP_Input0DebounceTime_               : %i", ParamTHP_Input0DebounceTime_               );
    logDebugP("ParamTHP_Input0ActionOpen_                 : %i", ParamTHP_Input0ActionOpen_                 );
    logDebugP("ParamTHP_Input0ActionClosed_               : %i", ParamTHP_Input0ActionClosed_               );
    logDebugP("ParamTHP_Input0SendCycle_                  : %i", ParamTHP_Input0SendCycle_                  );
    logDebugP("Binary Input 1 Parameters:");
    logDebugP("ParamTHP_Input1DebounceTime_               : %i", ParamTHP_Input1DebounceTime_               );
    logDebugP("ParamTHP_Input1ActionOpen_                 : %i", ParamTHP_Input1ActionOpen_                 );
    logDebugP("ParamTHP_Input1ActionClosed_               : %i", ParamTHP_Input1ActionClosed_               );
    logDebugP("ParamTHP_Input1SendCycle_                  : %i", ParamTHP_Input1SendCycle_                  );
    logDebugP("Temperature2 Parameters:");
    logDebugP("ParamTHP_SensorTemperature2SendChangeAmount: %f", ParamTHP_SensorTemperature2SendChangeAmount_);
    logDebugP("ParamTHP_SensorTemperature2SendCycle_      : %i", ParamTHP_SensorTemperature2SendCycle_       );
    logDebugP("ParamTHP_SensorTemperature2Align_          : %f", ParamTHP_SensorTemperature2Align_           );
    logDebugP("ParamTHP_SensorTemperature2WarnL_          : %f", ParamTHP_SensorTemperature2WarnL_           );
    logDebugP("ParamTHP_SensorTemperature2WarnH_          : %f", ParamTHP_SensorTemperature2WarnH_           );
    logDebugP("ParamTHP_SensorTemperature2MinMax_         : %i", ParamTHP_SensorTemperature2MinMax_          );
    */
    logDebugP("-------------------------------------------");
}

void Sensorchannel::loop()
{
    if(ParamTHP_Sensortype_ == 99)  // act as binary input
    {
        loop_binaryinput();
    }
    else
    {
        float temperature = m_hwSensors->GetTemperature(_channelIndex);
        float humidity = m_hwSensors->GetHumidity(_channelIndex);
        float abshumidity = CalcAbsHumidity(humidity, temperature);
        float dewpoint = CalcDewPoint(humidity, temperature);
        float pressure = m_hwSensors->GetPressure(_channelIndex);

        loop_temperature(temperature);
        loop_humidity(humidity);
        loop_abshumidity(abshumidity);
        loop_dewpoint(dewpoint);
        loop_pressure(pressure);
        
    }
}


void Sensorchannel::loop_binaryinput()
{
    bool new_input0 = !digitalRead(m_pin0);
    bool new_input1 = !digitalRead(m_pin1);
    bool send = false;

    if(new_input0 != m_input0)
    {
        if(!m_input0_debounce_millis)
        {
            m_input0_debounce_millis = millis();
        }
        else
        {
            if(delayCheck(m_input0_debounce_millis, ParamTHP_Input0DebounceTime_))
            {
                m_input0 = new_input0;
                m_input0_debounce_millis = 0;
                logInfoP(m_input0 ? "P0=1" : "P0=0");
                send = true;
            }
        }            
    }
    else
    {
        m_input0_debounce_millis = 0;

        if(ParamTHP_Input0SendCycle_)
            if(delayCheck(m_input0_last_send_millis, ParamTHP_Input0SendCycle_ * 60000) || m_input0_last_send_millis == 0)
                send = true;
    }
    if(send)
    {
        KoTHP_Input0_.value(m_input0 ? ParamTHP_Input0ActionClosed_ : ParamTHP_Input0ActionOpen_, Dpt(1,1)); // new value = 1 (Geschlossen (Closed))
        m_input0_last_send_millis = millis();
    }


    send = false;
    if(new_input1 != m_input1)
    {
        if(!m_input1_debounce_millis)
        {
            m_input1_debounce_millis = millis();
        }
        else
        {
            if(delayCheck(m_input1_debounce_millis, ParamTHP_Input1DebounceTime_))
            {
                m_input1 = new_input1;
                m_input1_debounce_millis = 0;
                logInfoP(m_input1 ? "P1=1" : "P1=0");
                send = true;
            }
        }            
    }
    else
    {
        m_input1_debounce_millis = 0;

        if(ParamTHP_Input1SendCycle_)
            if(delayCheck(m_input1_last_send_millis, ParamTHP_Input1SendCycle_ * 60000) || m_input1_last_send_millis == 0)
                send = true;
    }
    if(send)
    {
        KoTHP_Input1_.value(m_input1 ? ParamTHP_Input1ActionClosed_ : ParamTHP_Input1ActionOpen_, Dpt(1,1)); // new value = 1 (Geschlossen (Closed))
        m_input1_last_send_millis = millis();
    }
}

void Sensorchannel::loop_temperature(float temperature)
{
    if(!isnan(temperature))
    {
        uint8_t send_cycle = ParamTHP_SensorTemperatureSendCycle_;
        uint32_t send_millis = send_cycle * 60000;
        bool sendnow = false;
        if(send_cycle)
        {
            sendnow = millis() - m_temperature_last_send_millis > send_millis || m_temperature_last_send_millis == 0;
        }
        if(!sendnow)
        {
            float SendTresh = ParamTHP_SensorTemperatureSendChangeAmount_;
            if(SendTresh != 0)
            {
                float current_temp_diff = temperature - m_temperature_last_send_value;
                sendnow = current_temp_diff >= SendTresh || 0 - current_temp_diff >= SendTresh;
                if(sendnow)
                {
                    logDebugP("Send TempKO Diff: %f", current_temp_diff);
                }
            }
        }
        
        if(sendnow)
        {
            KoTHP_SensorTemp_.value(temperature + ParamTHP_SensorTemperatureAlign_, TempKODPT);
            logDebugP("Send TempKO: %f", temperature + ParamTHP_SensorTemperatureAlign_);
            m_temperature_last_send_millis = millis();
            m_temperature_last_send_value = temperature;
        }
        else
        {
            KoTHP_SensorTemp_.valueNoSend(temperature + ParamTHP_SensorTemperatureAlign_, TempKODPT);
        }

        if(ParamTHP_SensorTemperatureMinMax_)   // Min Max values enabled
        {
            if(temperature + ParamTHP_SensorTemperatureAlign_ > (float)KoTHP_SensorTempMaxValue_.value(TempKODPT))
            {
                KoTHP_SensorTempMaxValue_.valueNoSend(temperature, TempKODPT);
            }
            if(temperature + ParamTHP_SensorTemperatureAlign_ < (float)KoTHP_SensorTempMinValue_.value(TempKODPT))
            {
                KoTHP_SensorTempMinValue_.valueNoSend(temperature, TempKODPT);
            }
        }

        if(!(ParamTHP_SensorTemperatureWarnL_ == 0 && ParamTHP_SensorTemperatureWarnH_ == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = temperature + ParamTHP_SensorTemperatureAlign_ > ParamTHP_SensorTemperatureWarnH_;
            if( (bool)KoTHP_SensorTempAlarmH_.value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_temperature_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorTempAlarmH_.value(AlarmH, Dpt(1,5));
                m_temperature_alarmH_last_send_millis = millis();
            }

            bool AlarmL = temperature + ParamTHP_SensorTemperatureAlign_ < ParamTHP_SensorTemperatureWarnL_;
            if( (bool)KoTHP_SensorTempAlarmL_.value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_temperature_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorTempAlarmL_.value(AlarmL, Dpt(1,5));
                m_temperature_alarmL_last_send_millis = millis();
            }
        }
    }
}

void Sensorchannel::loop_humidity(float humidity)
{
    if(!isnan(humidity))
    {
        uint8_t send_cycle = ParamTHP_SensorHumiditySendCycle_;
        uint32_t send_millis = send_cycle * 60000;
        bool sendnow = false;
        if(send_cycle)
        {
            sendnow = millis() - m_humidity_last_send_millis > send_millis || m_humidity_last_send_millis == 0;
        }
        if(!sendnow)
        {
            float SendTresh = ParamTHP_SensorHumiditySendChangeAmount_;
            if(SendTresh != 0)
            {
                float current_diff = humidity - m_humidity_last_send_value;
                sendnow = current_diff >= SendTresh || 0 - current_diff >= SendTresh;
            }
        }
        
        if(sendnow)
        {
            KoTHP_SensorHum_.value(humidity + ParamTHP_SensorHumidityAlign_, HumKODPT);
            m_humidity_last_send_millis = millis();
            m_humidity_last_send_value = humidity;
        }
        else
        {
            KoTHP_SensorHum_.valueNoSend(humidity + ParamTHP_SensorHumidityAlign_, HumKODPT);
        }

        if(ParamTHP_SensorHumidityMinMax_)   // Min Max values enabled
        {
            if(humidity + ParamTHP_SensorHumidityAlign_ > (float)KoTHP_SensorHumMaxValue_.value(HumKODPT))
            {
                KoTHP_SensorHumMaxValue_.valueNoSend(humidity, HumKODPT);
            }
            if(humidity + ParamTHP_SensorHumidityAlign_ < (float)KoTHP_SensorHumMinValue_.value(HumKODPT))
            {
                KoTHP_SensorHumMinValue_.valueNoSend(humidity, HumKODPT);
            }
        }

        if(!(ParamTHP_SensorHumidityWarnL_ == 0 && ParamTHP_SensorHumidityWarnH_ == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = humidity + ParamTHP_SensorHumidityAlign_ > ParamTHP_SensorHumidityWarnH_;
            if( (bool)KoTHP_SensorHumAlarmH_.value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_humidity_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorHumAlarmH_.value(AlarmH, Dpt(1,5));
                m_humidity_alarmH_last_send_millis = millis();
            }

            bool AlarmL = humidity + ParamTHP_SensorHumidityAlign_ < ParamTHP_SensorHumidityWarnL_;
            if( (bool)KoTHP_SensorHumAlarmL_.value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_humidity_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorHumAlarmL_.value(AlarmL, Dpt(1,5));
                m_humidity_alarmL_last_send_millis = millis();
            }
        }
    }
}

void Sensorchannel::loop_abshumidity(float abshumidity) 
{
    if(!isnan(abshumidity))
    {
        uint8_t send_cycle = ParamTHP_SensorAbsHumiditySendCycle_;
        uint32_t send_millis = send_cycle * 60000;
        bool sendnow = false;
        if(send_cycle)
        {
            sendnow = millis() - m_abshumidity_last_send_millis > send_millis || m_abshumidity_last_send_millis == 0;
        }
        if(!sendnow)
        {
            float SendTresh = ParamTHP_SensorAbsHumiditySendChangeAmount_;
            if(SendTresh != 0)
            {
                float current_diff = abshumidity - m_abshumidity_last_send_value;
                sendnow = current_diff >= SendTresh || 0 - current_diff >= SendTresh;
            }
        }
        
        if(sendnow)
        {
            KoTHP_SensorAbsHum_.value(abshumidity + ParamTHP_SensorAbsHumidityAlign_, AbsHumKODPT);
            m_abshumidity_last_send_millis = millis();
            m_abshumidity_last_send_value = abshumidity;
        }
        else
        {
            KoTHP_SensorAbsHum_.valueNoSend(abshumidity + ParamTHP_SensorAbsHumidityAlign_, AbsHumKODPT);
        }

        if(ParamTHP_SensorAbsHumidityMinMax_)   // Min Max values enabled
        {
            if(abshumidity + ParamTHP_SensorAbsHumidityAlign_ > (float)KoTHP_SensorAbsHumMaxValue_.value(AbsHumKODPT))
            {
                KoTHP_SensorAbsHumMaxValue_.valueNoSend(abshumidity, AbsHumKODPT);
            }
            if(abshumidity + ParamTHP_SensorAbsHumidityAlign_ < (float)KoTHP_SensorAbsHumMinValue_.value(AbsHumKODPT))
            {
                KoTHP_SensorAbsHumMinValue_.valueNoSend(abshumidity, AbsHumKODPT);
            }
        }

        if(!(ParamTHP_SensorAbsHumidityWarnL_ == 0 && ParamTHP_SensorAbsHumidityWarnH_ == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = abshumidity + ParamTHP_SensorAbsHumidityAlign_ > ParamTHP_SensorAbsHumidityWarnH_;
            if( (bool)KoTHP_SensorAbsHumAlarmH_.value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_abshumidity_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorAbsHumAlarmH_.value(AlarmH, Dpt(1,5));
                m_abshumidity_alarmH_last_send_millis = millis();
            }

            bool AlarmL = abshumidity + ParamTHP_SensorAbsHumidityAlign_ < ParamTHP_SensorAbsHumidityWarnL_;
            if( (bool)KoTHP_SensorAbsHumAlarmL_.value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_abshumidity_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorAbsHumAlarmL_.value(AlarmL, Dpt(1,5));
                m_abshumidity_alarmL_last_send_millis = millis();
            }
        }
    }
}

void Sensorchannel::loop_dewpoint(float dewpoint)
{
    if(!isnan(dewpoint))
    {
        uint8_t send_cycle = ParamTHP_SensorDewPointSendCycle_;
        uint32_t send_millis = send_cycle * 60000;
        bool sendnow = false;
        if(send_cycle)
        {
            sendnow = millis() - m_dewpoint_last_send_millis > send_millis || m_dewpoint_last_send_millis == 0;
        }
        if(!sendnow)
        {
            float SendTresh = ParamTHP_SensorDewPointSendChangeAmount_;
            if(SendTresh != 0)
            {
                float current_diff = dewpoint - m_dewpoint_last_send_value;
                sendnow = current_diff >= SendTresh || 0 - current_diff >= SendTresh;
            }
        }
        
        if(sendnow)
        {
            KoTHP_SensorDewPoint_.value(dewpoint + ParamTHP_SensorDewPointAlign_, DewPointKODPT);
            m_dewpoint_last_send_millis = millis();
            m_dewpoint_last_send_value = dewpoint;
        }
        else
        {
            KoTHP_SensorDewPoint_.valueNoSend(dewpoint + ParamTHP_SensorDewPointAlign_, DewPointKODPT);
        }

        if(ParamTHP_SensorDewPointMinMax_)   // Min Max values enabled
        {
            if(dewpoint + ParamTHP_SensorDewPointAlign_ > (float)KoTHP_SensorDewPointMaxValue_.value(DewPointKODPT))
            {
                KoTHP_SensorDewPointMaxValue_.valueNoSend(dewpoint, DewPointKODPT);
            }
            if(dewpoint + ParamTHP_SensorDewPointAlign_ < (float)KoTHP_SensorDewPointMinValue_.value(DewPointKODPT))
            {
                KoTHP_SensorDewPointMinValue_.valueNoSend(dewpoint, DewPointKODPT);
            }
        }

        if(!(ParamTHP_SensorDewPointWarnL_ == 0 && ParamTHP_SensorDewPointWarnH_ == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = dewpoint + ParamTHP_SensorDewPointAlign_ > ParamTHP_SensorDewPointWarnH_;
            if( (bool)KoTHP_SensorDewPointAlarmH_.value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_dewpoint_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorDewPointAlarmH_.value(AlarmH, Dpt(1,5));
                m_dewpoint_alarmH_last_send_millis = millis();
            }

            bool AlarmL = dewpoint + ParamTHP_SensorDewPointAlign_ < ParamTHP_SensorDewPointWarnL_;
            if( (bool)KoTHP_SensorDewPointAlarmL_.value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_dewpoint_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorDewPointAlarmL_.value(AlarmL, Dpt(1,5));
                m_dewpoint_alarmL_last_send_millis = millis();
            }
        }
    }
}

void Sensorchannel::loop_pressure(float pressure)
{
    uint32_t send_cycle = ParamTHP_SensorPressureSendCycle_;
    uint32_t send_millis = send_cycle * 60000;
    bool sendnow = false;
    if(!isnan(pressure) )
    {
        if(send_cycle)
        {
            sendnow = millis() - m_pressure_last_send_millis > send_millis || m_pressure_last_send_millis == 0;
        }
        if(!sendnow)
        {
            float SendTresh = ParamTHP_SensorPressureSendChangeAmount_;
            if(SendTresh != 0)
            {
                float current_diff = pressure - m_pressure_last_send_value;
                sendnow = current_diff >= SendTresh || 0 - current_diff >= SendTresh;
            }
        }
        
        if(sendnow)
        {
            KoTHP_SensorPress_.value(pressure + ParamTHP_SensorPressureAlign_, PressKODPT);
            m_pressure_last_send_millis = millis();
            m_pressure_last_send_value = pressure;
        }
        else
        {
            KoTHP_SensorPress_.valueNoSend(pressure + ParamTHP_SensorPressureAlign_, PressKODPT);
        }

        if(ParamTHP_SensorPressureMinMax_)   // Min Max values enabled
        {
            if(pressure + ParamTHP_SensorPressureAlign_ > (float)KoTHP_SensorPressMaxValue_.value(PressKODPT))
            {
                KoTHP_SensorPressMaxValue_.valueNoSend(pressure, PressKODPT);
            }
            if(pressure + ParamTHP_SensorPressureAlign_ < (float)KoTHP_SensorPressMinValue_.value(PressKODPT))
            {
                KoTHP_SensorPressMinValue_.valueNoSend(pressure, PressKODPT);
            }
        }

        if(!(ParamTHP_SensorPressureWarnL_ == 0 && ParamTHP_SensorPressureWarnH_ == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = pressure + ParamTHP_SensorPressureAlign_ > ParamTHP_SensorPressureWarnH_;
            if( (bool)KoTHP_SensorPressAlarmH_.value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_pressure_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorPressAlarmH_.value(AlarmH, Dpt(1,5));
                m_pressure_alarmH_last_send_millis = millis();
            }

            bool AlarmL = pressure + ParamTHP_SensorPressureAlign_ < ParamTHP_SensorPressureWarnL_;
            if( (bool)KoTHP_SensorPressAlarmL_.value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_pressure_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                KoTHP_SensorPressAlarmL_.value(AlarmL, Dpt(1,5));
                m_pressure_alarmL_last_send_millis = millis();
            }
        }
    }
}

void Sensorchannel::processInputKo(GroupObject& ko)
{
    logTraceP("processInputKo");

    float setvalue = 0;

    switch(RelKO(ko.asap()))
    {
        case THP_KoSensorTempMinMaxReset_:
            setvalue = KoTHP_SensorTemp_.value(TempKODPT);
            KoTHP_SensorTempMaxValue_.valueNoSend(setvalue, TempKODPT);
            KoTHP_SensorTempMinValue_.valueNoSend(setvalue, TempKODPT);
        break;
        case THP_KoSensorHumMinMaxReset_:
            setvalue = knx.getGroupObject(AbsKO(THP_KoSensorHum_)).value(HumKODPT);
            knx.getGroupObject(AbsKO(THP_KoSensorHumMinValue_)).valueNoSend(setvalue, HumKODPT);
            knx.getGroupObject(AbsKO(THP_KoSensorHumMaxValue_)).valueNoSend(setvalue, HumKODPT);
        break;
        case THP_KoSensorAbsHumMinMaxReset_:
            setvalue = knx.getGroupObject(AbsKO(THP_KoSensorAbsHum_)).value(AbsHumKODPT);
            knx.getGroupObject(AbsKO(THP_KoSensorAbsHumMinValue_)).valueNoSend(setvalue, AbsHumKODPT);
            knx.getGroupObject(AbsKO(THP_KoSensorAbsHumMaxValue_)).valueNoSend(setvalue, AbsHumKODPT);
        break;
        case THP_KoSensorDewPointMinMaxReset_:
            setvalue = knx.getGroupObject(AbsKO(THP_KoSensorDewPoint_)).value(DewPointKODPT);
            knx.getGroupObject(AbsKO( THP_KoSensorDewPointMinValue_)).valueNoSend(setvalue, DewPointKODPT);
            knx.getGroupObject(AbsKO(THP_KoSensorDewPointMaxValue_)).valueNoSend(setvalue, DewPointKODPT);
        break;
        case THP_KoSensorPressMinMaxReset_:
            setvalue = knx.getGroupObject(AbsKO(THP_KoSensorPress_)).value(PressKODPT);
            knx.getGroupObject(AbsKO(THP_KoSensorPressMinValue_)).valueNoSend(setvalue, PressKODPT);
            knx.getGroupObject(AbsKO(THP_KoSensorPressMaxValue_)).valueNoSend(setvalue, PressKODPT);
        break;
    }
}


float Sensorchannel::CalcDewPoint(float relative_humidity, float temperature)
{
   float a,b;
    if( temperature >= 0 )
    {
        a = 7.5;
        b = 237.3;
    }
    else
    {
        a = 7.6;
        b = 240.7;
    }

    // Magnusformel
    double sdd = 6.1078 * pow(10.0, ( (a * temperature) / (b + temperature) ) ); //Sättigungsdampfdruck
    double dd = (relative_humidity / 100.0) * sdd; //Dampfdruck
    double v = log10 ( (dd / 6.1078) / log10(10));

    double value = (b * v) / (a - v); //Taupunkt 
    return (float)value;
}

float Sensorchannel::CalcAbsHumidity(float relative_humidity, float temperature)
{
    //input in 1°C , 0-100% humidity
    //ouput in g/m³

    float value = (6.112 * pow(2.718, (17.67 * temperature)/(temperature+243.5)) * relative_humidity * 2.1674) / (273.15 + temperature);
    return value;
}

const std::string Sensorchannel::name()
{
    return "Sensor";
}

void Sensorchannel::save()
{
    openknx.flash.writeFloat((float)KoTHP_SensorTempMaxValue_.value(TempKODPT));
    openknx.flash.writeFloat((float)KoTHP_SensorTempMinValue_.value(TempKODPT));

    openknx.flash.writeFloat((float)KoTHP_SensorHumMaxValue_.value(HumKODPT));
    openknx.flash.writeFloat((float)KoTHP_SensorHumMinValue_.value(HumKODPT));

    openknx.flash.writeFloat((float)KoTHP_SensorAbsHumMaxValue_.value(AbsHumKODPT));
    openknx.flash.writeFloat((float)KoTHP_SensorAbsHumMinValue_.value(AbsHumKODPT));

    openknx.flash.writeFloat((float)KoTHP_SensorDewPointMaxValue_.value(DewPointKODPT));
    openknx.flash.writeFloat((float)KoTHP_SensorDewPointMinValue_.value(DewPointKODPT));

    openknx.flash.writeFloat((float)KoTHP_SensorPressMaxValue_.value(PressKODPT));
    openknx.flash.writeFloat((float)KoTHP_SensorPressMinValue_.value(PressKODPT));
}

void Sensorchannel::restore()
{
    KoTHP_SensorTempMaxValue_.valueNoSend(openknx.flash.readFloat(), TempKODPT);
    KoTHP_SensorTempMinValue_.valueNoSend(openknx.flash.readFloat(), TempKODPT);

    KoTHP_SensorHumMaxValue_.valueNoSend(openknx.flash.readFloat(), HumKODPT);
    KoTHP_SensorHumMinValue_.valueNoSend(openknx.flash.readFloat(), HumKODPT);

    KoTHP_SensorAbsHumMaxValue_.valueNoSend(openknx.flash.readFloat(), AbsHumKODPT);
    KoTHP_SensorAbsHumMinValue_.valueNoSend(openknx.flash.readFloat(), AbsHumKODPT);

    KoTHP_SensorDewPointMaxValue_.valueNoSend(openknx.flash.readFloat(), DewPointKODPT);
    KoTHP_SensorDewPointMinValue_.valueNoSend(openknx.flash.readFloat(), DewPointKODPT);

    KoTHP_SensorPressMaxValue_.valueNoSend(openknx.flash.readFloat(), PressKODPT);
    KoTHP_SensorPressMinValue_.valueNoSend(openknx.flash.readFloat(), PressKODPT);
}