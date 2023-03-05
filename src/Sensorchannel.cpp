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

    //Todo load from eeprom
    KoTHP_SensorTempMinValue_.valueNoSend((float)1000, Dpt(9,1));
    KoTHP_SensorTempMaxValue_.valueNoSend((float)-1000, Dpt(9,1));
    KoTHP_SensorHumMinValue_.valueNoSend((float)1000, HumKODPT);
    KoTHP_SensorHumMaxValue_.valueNoSend((float)-1000, HumKODPT);
    KoTHP_SensorAbsHumMinValue_.valueNoSend((float)1000, Dpt(9,29));
    KoTHP_SensorAbsHumMaxValue_.valueNoSend((float)-1000, Dpt(9,29));
    KoTHP_SensorDewPointMinValue_.valueNoSend((float)1000, Dpt(9,1));
    KoTHP_SensorDewPointMaxValue_.valueNoSend((float)-1000, Dpt(9,1));
    KoTHP_SensorPressMinValue_.valueNoSend((float)1000, Dpt(9,6));
    KoTHP_SensorPressMaxValue_.valueNoSend((float)-1000, Dpt(9,6));

    
    // Debug
    logDebugP("Setup");
    /*
    logIndentUp();
    logDebug("", "ParamTHP_Sensortype_                       : %i", ParamTHP_Sensortype_                       );
    logDebug("", "ParamTHP_SensorTemperatureSendChangeAmount_: %i", ParamTHP_SensorTemperatureSendChangeAmount_);
    logDebug("", "ParamTHP_SensorTemperatureSendCycle_       : %i", ParamTHP_SensorTemperatureSendCycle_       );
    logDebug("", "ParamTHP_SensorTemperatureAlign_           : %i", ParamTHP_SensorTemperatureAlign_           );
    logDebug("", "ParamTHP_SensorTemperatureWarnL_           : %i", ParamTHP_SensorTemperatureWarnL_           );
    logDebug("", "ParamTHP_SensorTemperatureWarnH_           : %i", ParamTHP_SensorTemperatureWarnH_           );
    logDebug("", "ParamTHP_SensorTemperatureMinMax_          : %i", ParamTHP_SensorTemperatureMinMax_          );
			 "", 
    logDebug("", "ParamTHP_SensorHumiditySendChangeAmount_   : %i", ParamTHP_SensorHumiditySendChangeAmount_   );
    logDebug("", "ParamTHP_SensorHumiditySendCycle_          : %i", ParamTHP_SensorHumiditySendCycle_          );
    logDebug("", "ParamTHP_SensorHumidityAlign_              : %i", ParamTHP_SensorHumidityAlign_              );
    logDebug("", "ParamTHP_SensorHumidityWarnL_              : %i", ParamTHP_SensorHumidityWarnL_              );
    logDebug("", "ParamTHP_SensorHumidityWarnH_              : %i", ParamTHP_SensorHumidityWarnH_              );
    logDebug("", "ParamTHP_SensorHumidityMinMax_             : %i", ParamTHP_SensorHumidityMinMax_             );
    logDebug("", "ParamTHP_SensorHumiditySend_               : %i", ParamTHP_SensorHumiditySend_               );
			 "", 
    logDebug("", "ParamTHP_Input0DebounceTime_               : %i", ParamTHP_Input0DebounceTime_               );
    logDebug("", "ParamTHP_Input0ActionOpen_                 : %i", ParamTHP_Input0ActionOpen_                 );
    logDebug("", "ParamTHP_Input0ActionClosed_               : %i", ParamTHP_Input0ActionClosed_               );
    logDebug("", "ParamTHP_Input0SendCycle_                  : %i", ParamTHP_Input0SendCycle_                  );
    logDebug("", "ParamTHP_Input1DebounceTime_               : %i", ParamTHP_Input1DebounceTime_               );
    logDebug("", "ParamTHP_Input1ActionOpen_                 : %i", ParamTHP_Input1ActionOpen_                 );
    logDebug("", "ParamTHP_Input1ActionClosed_               : %i", ParamTHP_Input1ActionClosed_               );
    logDebug("", "ParamTHP_Input1SendCycle_                  : %i", ParamTHP_Input1SendCycle_                  );
    logIndentDown();*/
    logDebugP("ParamTHP_Sensortype_                       : %i", ParamTHP_Sensortype_                       );
    logDebugP("Temperature Parameters:");
    logDebugP("ParamTHP_SensorTemperatureSendChangeAmount_: %f", ParamTHP_SensorTemperatureSendChangeAmount_);
    logDebugP("ParamTHP_SensorTemperatureSendCycle_       : %i", ParamTHP_SensorTemperatureSendCycle_       );
    logDebugP("ParamTHP_SensorTemperatureAlign_           : %f", ParamTHP_SensorTemperatureAlign_           );
    logDebugP("ParamTHP_SensorTemperatureWarnL_           : %f", ParamTHP_SensorTemperatureWarnL_           );
    logDebugP("ParamTHP_SensorTemperatureWarnH_           : %f", ParamTHP_SensorTemperatureWarnH_           );
    logDebugP("ParamTHP_SensorTemperatureMinMax_          : %i", ParamTHP_SensorTemperatureMinMax_          );
	logDebugP("Humidity Parameters:");
    logDebugP("ParamTHP_SensorHumiditySendChangeAmount_   : %f", ParamTHP_SensorHumiditySendChangeAmount_   );
    logDebugP("ParamTHP_SensorHumiditySendCycle_          : %i", ParamTHP_SensorHumiditySendCycle_          );
    logDebugP("ParamTHP_SensorHumidityAlign_              : %f", ParamTHP_SensorHumidityAlign_              );
    logDebugP("ParamTHP_SensorHumidityWarnL_              : %f", ParamTHP_SensorHumidityWarnL_              );
    logDebugP("ParamTHP_SensorHumidityWarnH_              : %f", ParamTHP_SensorHumidityWarnH_              );
    logDebugP("ParamTHP_SensorHumidityMinMax_             : %i", ParamTHP_SensorHumidityMinMax_             );
    logDebugP("ParamTHP_SensorHumiditySend_               : %i", ParamTHP_SensorHumiditySend_               );
	logDebugP("Binary Input Parameters:");
    logDebugP("ParamTHP_Input0DebounceTime_               : %i", ParamTHP_Input0DebounceTime_               );
    logDebugP("ParamTHP_Input0ActionOpen_                 : %i", ParamTHP_Input0ActionOpen_                 );
    logDebugP("ParamTHP_Input0ActionClosed_               : %i", ParamTHP_Input0ActionClosed_               );
    logDebugP("ParamTHP_Input0SendCycle_                  : %i", ParamTHP_Input0SendCycle_                  );
    logDebugP("ParamTHP_Input1DebounceTime_               : %i", ParamTHP_Input1DebounceTime_               );
    logDebugP("ParamTHP_Input1ActionOpen_                 : %i", ParamTHP_Input1ActionOpen_                 );
    logDebugP("ParamTHP_Input1ActionClosed_               : %i", ParamTHP_Input1ActionClosed_               );
    logDebugP("ParamTHP_Input1SendCycle_                  : %i", ParamTHP_Input1SendCycle_                  );
}

void Sensorchannel::loop()
{
    if(ParamTHP_Sensortype_ == 99)  // act as binary input
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

    float temperature = m_hwSensors->GetTemperature(_channelIndex);
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
            }
        }
        
        if(sendnow)
        {
            KoTHP_SensorTemp_.value(temperature + ParamTHP_SensorTemperatureAlign_, Dpt(9,1));
            m_temperature_last_send_millis = millis();
            m_temperature_last_send_value = temperature;
        }
        else
        {
            KoTHP_SensorTemp_.valueNoSend(temperature + ParamTHP_SensorTemperatureAlign_, Dpt(9,1));
        }

        if(ParamTHP_SensorTemperatureMinMax_)   // Min Max values enabled
        {
            if(temperature + ParamTHP_SensorTemperatureAlign_ > (float)KoTHP_SensorTempMaxValue_.value(Dpt(9,1)))
            {
                KoTHP_SensorTempMaxValue_.valueNoSend(temperature, Dpt(9,1));
            }
            if(temperature + ParamTHP_SensorTemperatureAlign_ < (float)KoTHP_SensorTempMinValue_.value(Dpt(9,1)))
            {
                KoTHP_SensorTempMinValue_.valueNoSend(temperature, Dpt(9,1));
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

    float humidity = m_hwSensors->GetHumidity(_channelIndex);
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
            float SendTresh = ParamTHP_SensorAbsHumiditySendChangeAmount_;
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

    if(!isnan(humidity) &&!isnan(temperature))
    {
        float abshumidity = CalcAbsHumidity(humidity, temperature);

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
            KoTHP_SensorAbsHum_.value(abshumidity + ParamTHP_SensorAbsHumidityAlign_, Dpt(9,29));
            m_abshumidity_last_send_millis = millis();
            m_abshumidity_last_send_value = abshumidity;
        }
        else
        {
            KoTHP_SensorAbsHum_.valueNoSend(abshumidity + ParamTHP_SensorAbsHumidityAlign_, Dpt(9,29));
        }

        if(ParamTHP_SensorAbsHumidityMinMax_)   // Min Max values enabled
        {
            if(abshumidity + ParamTHP_SensorAbsHumidityAlign_ > (float)KoTHP_SensorAbsHumMaxValue_.value(Dpt(9,29)))
            {
                KoTHP_SensorAbsHumMaxValue_.valueNoSend(abshumidity, Dpt(9,29));
            }
            if(abshumidity + ParamTHP_SensorAbsHumidityAlign_ < (float)KoTHP_SensorAbsHumMinValue_.value(Dpt(9,29)))
            {
                KoTHP_SensorAbsHumMinValue_.valueNoSend(abshumidity, Dpt(9,29));
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

        float dewpoint = CalcDewPoint(humidity, temperature);
        send_cycle = ParamTHP_SensorDewPointSendCycle_;
        send_millis = send_cycle * 60000;
        sendnow = false;
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
            KoTHP_SensorDewPoint_.value(dewpoint + ParamTHP_SensorDewPointAlign_, Dpt(9,1));
            m_dewpoint_last_send_millis = millis();
            m_dewpoint_last_send_value = dewpoint;
        }
        else
        {
            KoTHP_SensorDewPoint_.valueNoSend(dewpoint + ParamTHP_SensorDewPointAlign_, Dpt(9,1));
        }

        if(ParamTHP_SensorDewPointMinMax_)   // Min Max values enabled
        {
            if(dewpoint + ParamTHP_SensorDewPointAlign_ > (float)KoTHP_SensorDewPointMaxValue_.value(Dpt(9,1)))
            {
                KoTHP_SensorDewPointMaxValue_.valueNoSend(dewpoint, Dpt(9,1));
            }
            if(dewpoint + ParamTHP_SensorDewPointAlign_ < (float)KoTHP_SensorDewPointMinValue_.value(Dpt(9,1)))
            {
                KoTHP_SensorDewPointMinValue_.valueNoSend(dewpoint, Dpt(9,1));
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

    float pressure = m_hwSensors->GetPressure(_channelIndex);
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
            KoTHP_SensorPress_.value(pressure + ParamTHP_SensorPressureAlign_, Dpt(9,6));
            m_pressure_last_send_millis = millis();
            m_pressure_last_send_value = pressure;
        }
        else
        {
            KoTHP_SensorPress_.valueNoSend(pressure + ParamTHP_SensorPressureAlign_, Dpt(9,6));
        }

        if(ParamTHP_SensorPressureMinMax_)   // Min Max values enabled
        {
            if(pressure + ParamTHP_SensorPressureAlign_ > (float)KoTHP_SensorPressMaxValue_.value(Dpt(9,6)))
            {
                KoTHP_SensorPressMaxValue_.valueNoSend(pressure, Dpt(9,6));
            }
            if(pressure + ParamTHP_SensorPressureAlign_ < (float)KoTHP_SensorPressMinValue_.value(Dpt(9,6)))
            {
                KoTHP_SensorPressMinValue_.valueNoSend(pressure, Dpt(9,6));
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
            setvalue = knx.getGroupObject(AbsKO(THP_KoSensorTemp_)).value(TempKODPT);
            knx.getGroupObject(AbsKO(THP_KoSensorTempMinValue_)).valueNoSend(setvalue, TempKODPT);
            knx.getGroupObject(AbsKO(THP_KoSensorTempMaxValue_)).valueNoSend(setvalue, TempKODPT);
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
    return "Sensorchannel";
}