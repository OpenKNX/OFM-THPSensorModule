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
        log("Setting INPUT_PULLUP to %d", m_pin0); // DEBUG
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
    log("Param SensorTemperatureSendChangeAmount: %i", ParamTHP_SensorTemperatureSendChangeAmount_);
    log("Param SensorTemperatureSendCycle: %i", ParamTHP_SensorTemperatureSendCycle_);

    Serial.print(" ParamTempAlign: ");
    Serial.print(ParamTempAlign());
    Serial.print(" ParamTempWarnL: ");
    Serial.print(ParamTempWarnL());
    Serial.print(" ParamTempWarnH: ");
    Serial.print(ParamTempWarnH());
    Serial.print(" ParamTempMinMax: ");
    Serial.print(ParamTempMinMax());
    Serial.println("");
    Serial.print(" ParamHumSendChangeAmount: ");
    Serial.print(ParamHumSendChangeAmount());
    Serial.print(" ParamHumSendCycle: ");
    Serial.print(ParamHumSendCycle());
    Serial.print(" ParamHumAlign: ");
    Serial.print(ParamHumAlign());
    Serial.print(" ParamHumWarnL: ");
    Serial.print(ParamHumWarnL());
    Serial.print(" ParamHumWarnH: ");
    Serial.print(ParamHumWarnH());
    Serial.print(" ParamHumMinMax: ");
    Serial.print(ParamHumMinMax());
    Serial.print(" ParamHumDPT: ");
    Serial.print(ParamHumDPT());
    Serial.println("");

    
}

void Sensorchannel::loop()
{
    if(ParamTHP_Sensortype_ == 99)  // act as binary input
    {
        bool new_input0 = !digitalRead(m_pin0);
        bool new_input1 = !digitalRead(m_pin1);

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
                    KoTHP_InputSCL_.value(m_input0, Dpt(1,1));
                }
            }            
        }
        else
            m_input0_debounce_millis = 0;

        if(new_input1 != m_input1)
        {
            m_input1 = new_input1;
            log(m_input1 ? "P1=1" : "P1=0");
            //KoTHP_InputSDA_.value(m_input1, Dpt(1,1));
        }
    }

    float temperature = m_hwSensors->GetTemperature(_channelIndex);
    if(!isnan(temperature))
    {
        //Serial.print("temp: ");
        //Serial.println(temperature);
        uint8_t send_cycle = ParamTHP_SensorTemperatureSendCycle_;
        uint32_t send_millis = send_cycle * 60000;
        bool sendnow = false;
        if(send_cycle)
        {
            sendnow = millis() - m_temperature_last_send_millis > send_millis || m_temperature_last_send_millis == 0;
        }
        if(!sendnow)
        {
            float SendTresh = ParamTempSendChangeAmount();
            if(SendTresh != 0)
            {
                float current_temp_diff = temperature - m_temperature_last_send_value;
                sendnow = current_temp_diff >= SendTresh || 0 - current_temp_diff >= SendTresh;
            }
        }
        
        if(sendnow)
        {
            GetTempKO().value(temperature + ParamTempAlign(), Dpt(9,1));
            m_temperature_last_send_millis = millis();
            m_temperature_last_send_value = temperature;
        }
        else
        {
            GetTempKO().valueNoSend(temperature + ParamTempAlign(), Dpt(9,1));
        }

        if(ParamTempMinMax())   // Min Max values enabled
        {
            if(temperature + ParamTempAlign() > (float)GetTempMaxValueKO().value(Dpt(9,1)))
            {
                GetTempMaxValueKO().valueNoSend(temperature, Dpt(9,1));
            }
            if(temperature + ParamTempAlign() < (float)GetTempMinValueKO().value(Dpt(9,1)))
            {
                GetTempMinValueKO().valueNoSend(temperature, Dpt(9,1));
            }
        }

        if(!(ParamTempWarnL() == 0 && ParamTempWarnH() == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = temperature + ParamTempAlign() > ParamTempWarnH();
            if( (bool)GetTempAlarmHKO().value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_temperature_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetTempAlarmHKO().value(AlarmH, Dpt(1,5));
                m_temperature_alarmH_last_send_millis = millis();
            }

            bool AlarmL = temperature + ParamTempAlign() < ParamTempWarnL();
            if( (bool)GetTempAlarmLKO().value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_temperature_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetTempAlarmLKO().value(AlarmL, Dpt(1,5));
                m_temperature_alarmL_last_send_millis = millis();
            }
        }
    }

    float humidity = m_hwSensors->GetHumidity(_channelIndex);
    if(!isnan(humidity))
    {
        //Serial.print("hum: ");
        //Serial.println(humidity);
        //Serial.print("hum+align: ");
        //Serial.println(humidity + ParamHumAlign());
        uint8_t send_cycle = ParamHumSendCycle();
        uint32_t send_millis = send_cycle * 60000;
        bool sendnow = false;
        if(send_cycle)
        {
            sendnow = millis() - m_humidity_last_send_millis > send_millis || m_humidity_last_send_millis == 0;
        }
        if(!sendnow)
        {
            float SendTresh = ParamHumSendChangeAmount();
            if(SendTresh != 0)
            {
                float current_diff = humidity - m_humidity_last_send_value;
                sendnow = current_diff >= SendTresh || 0 - current_diff >= SendTresh;
            }
        }
        
        if(sendnow)
        {
            GetHumKO().value(humidity + ParamHumAlign(), HumKODPT);
            m_humidity_last_send_millis = millis();
            m_humidity_last_send_value = humidity;
        }
        else
        {
            GetHumKO().valueNoSend(humidity + ParamHumAlign(), HumKODPT);
        }

        if(ParamHumMinMax())   // Min Max values enabled
        {
            if(humidity + ParamHumAlign() > (float)GetHumMaxValueKO().value(HumKODPT))
            {
                GetHumMaxValueKO().valueNoSend(humidity, HumKODPT);
            }
            if(humidity + ParamHumAlign() < (float)GetHumMinValueKO().value(HumKODPT))
            {
                GetHumMinValueKO().valueNoSend(humidity, HumKODPT);
            }
        }

        if(!(ParamHumWarnL() == 0 && ParamHumWarnH() == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = humidity + ParamHumAlign() > ParamHumWarnH();
            if( (bool)GetHumAlarmHKO().value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_humidity_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetHumAlarmHKO().value(AlarmH, Dpt(1,5));
                m_humidity_alarmH_last_send_millis = millis();
            }

            bool AlarmL = humidity + ParamHumAlign() < ParamHumWarnL();
            if( (bool)GetHumAlarmLKO().value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_humidity_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetHumAlarmLKO().value(AlarmL, Dpt(1,5));
                m_humidity_alarmL_last_send_millis = millis();
            }
        }
    }

    if(!isnan(humidity) &&!isnan(temperature))
    {
        float abshumidity = CalcAbsHumidity(humidity, temperature);
        // Serial.print("abshumidity: ");
        // Serial.println(abshumidity);
        // Serial.print("abshumidity+align: ");
        // Serial.println(abshumidity + ParamAbsHumAlign());

        uint8_t send_cycle = ParamAbsHumSendCycle();
        uint32_t send_millis = send_cycle * 60000;
        bool sendnow = false;
        if(send_cycle)
        {
            sendnow = millis() - m_abshumidity_last_send_millis > send_millis || m_abshumidity_last_send_millis == 0;
        }
        if(!sendnow)
        {
            float SendTresh = ParamAbsHumSendChangeAmount();
            if(SendTresh != 0)
            {
                float current_diff = abshumidity - m_abshumidity_last_send_value;
                sendnow = current_diff >= SendTresh || 0 - current_diff >= SendTresh;
            }
        }
        
        if(sendnow)
        {
            GetAbsHumKO().value(abshumidity + ParamAbsHumAlign(), Dpt(9,29));
            m_abshumidity_last_send_millis = millis();
            m_abshumidity_last_send_value = abshumidity;
        }
        else
        {
            GetAbsHumKO().valueNoSend(abshumidity + ParamAbsHumAlign(), Dpt(9,29));
        }

        if(ParamAbsHumMinMax())   // Min Max values enabled
        {
            if(abshumidity + ParamAbsHumAlign() > (float)GetAbsHumMaxValueKO().value(Dpt(9,29)))
            {
                GetAbsHumMaxValueKO().valueNoSend(abshumidity, Dpt(9,29));
            }
            if(abshumidity + ParamAbsHumAlign() < (float)GetAbsHumMinValueKO().value(Dpt(9,29)))
            {
                GetAbsHumMinValueKO().valueNoSend(abshumidity, Dpt(9,29));
            }
        }

        if(!(ParamAbsHumWarnL() == 0 && ParamAbsHumWarnH() == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = abshumidity + ParamAbsHumAlign() > ParamAbsHumWarnH();
            if( (bool)GetAbsHumAlarmHKO().value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_abshumidity_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetAbsHumAlarmHKO().value(AlarmH, Dpt(1,5));
                m_abshumidity_alarmH_last_send_millis = millis();
            }

            bool AlarmL = abshumidity + ParamAbsHumAlign() < ParamAbsHumWarnL();
            if( (bool)GetAbsHumAlarmLKO().value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_abshumidity_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetAbsHumAlarmLKO().value(AlarmL, Dpt(1,5));
                m_abshumidity_alarmL_last_send_millis = millis();
            }
        }

        float dewpoint = CalcDewPoint(humidity, temperature);
        send_cycle = ParamDewPointSendCycle();
        send_millis = send_cycle * 60000;
        sendnow = false;
        if(send_cycle)
        {
            sendnow = millis() - m_dewpoint_last_send_millis > send_millis || m_dewpoint_last_send_millis == 0;
        }
        if(!sendnow)
        {
            float SendTresh = ParamDewPointSendChangeAmount();
            if(SendTresh != 0)
            {
                float current_diff = dewpoint - m_dewpoint_last_send_value;
                sendnow = current_diff >= SendTresh || 0 - current_diff >= SendTresh;
            }
        }
        
        if(sendnow)
        {
            GetDewPointKO().value(dewpoint + ParamDewPointAlign(), Dpt(9,1));
            m_dewpoint_last_send_millis = millis();
            m_dewpoint_last_send_value = dewpoint;
        }
        else
        {
            GetDewPointKO().valueNoSend(dewpoint + ParamDewPointAlign(), Dpt(9,1));
        }

        if(ParamDewPointMinMax())   // Min Max values enabled
        {
            if(dewpoint + ParamDewPointAlign() > (float)GetDewPointMaxValueKO().value(Dpt(9,1)))
            {
                GetDewPointMaxValueKO().valueNoSend(dewpoint, Dpt(9,1));
            }
            if(dewpoint + ParamDewPointAlign() < (float)GetDewPointMinValueKO().value(Dpt(9,1)))
            {
                GetDewPointMinValueKO().valueNoSend(dewpoint, Dpt(9,1));
            }
        }

        if(!(ParamDewPointWarnL() == 0 && ParamDewPointWarnH() == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = dewpoint + ParamDewPointAlign() > ParamDewPointWarnH();
            if( (bool)GetDewPointAlarmHKO().value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_dewpoint_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetDewPointAlarmHKO().value(AlarmH, Dpt(1,5));
                m_dewpoint_alarmH_last_send_millis = millis();
            }

            bool AlarmL = dewpoint + ParamDewPointAlign() < ParamDewPointWarnL();
            if( (bool)GetDewPointAlarmLKO().value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_dewpoint_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetDewPointAlarmLKO().value(AlarmL, Dpt(1,5));
                m_dewpoint_alarmL_last_send_millis = millis();
            }
        }
    }

    float pressure = m_hwSensors->GetPressure(_channelIndex);
    uint32_t send_cycle = ParamPressSendCycle();
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
            float SendTresh = ParamPressSendChangeAmount();
            if(SendTresh != 0)
            {
                float current_diff = pressure - m_pressure_last_send_value;
                sendnow = current_diff >= SendTresh || 0 - current_diff >= SendTresh;
            }
        }
        
        if(sendnow)
        {
            GetPressKO().value(pressure + ParamPressAlign(), Dpt(9,6));
            m_pressure_last_send_millis = millis();
            m_pressure_last_send_value = pressure;
        }
        else
        {
            GetPressKO().valueNoSend(pressure + ParamPressAlign(), Dpt(9,6));
        }

        if(ParamPressMinMax())   // Min Max values enabled
        {
            if(pressure + ParamPressAlign() > (float)GetPressMaxValueKO().value(Dpt(9,6)))
            {
                GetPressMaxValueKO().valueNoSend(pressure, Dpt(9,6));
            }
            if(pressure + ParamPressAlign() < (float)GetPressMinValueKO().value(Dpt(9,6)))
            {
                GetPressMinValueKO().valueNoSend(pressure, Dpt(9,6));
            }
        }

        if(!(ParamPressWarnL() == 0 && ParamPressWarnH() == 0))   // not both are 0 (=> feature disabled)
        {
            bool AlarmH = pressure + ParamPressAlign() > ParamPressWarnH();
            if( (bool)GetPressAlarmHKO().value(Dpt(1,5)) != AlarmH ||                                          // alarm value has changed
                (AlarmH && millis() - m_pressure_alarmH_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetPressAlarmHKO().value(AlarmH, Dpt(1,5));
                m_pressure_alarmH_last_send_millis = millis();
            }

            bool AlarmL = pressure + ParamPressAlign() < ParamPressWarnL();
            if( (bool)GetPressAlarmLKO().value(Dpt(1,5)) != AlarmL ||                                          // alarm value has changed
                (AlarmL && millis() - m_pressure_alarmL_last_send_millis > send_millis))     // alarm is true and has not been sent for send_millis
            {
                GetPressAlarmLKO().value(AlarmL, Dpt(1,5));
                m_pressure_alarmL_last_send_millis = millis();
            }
        }
    }
}

void Sensorchannel::processInputKo(GroupObject& ko)
{
    log("Sensorchannel::processInputKo");

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