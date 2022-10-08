#ifdef THPSENSORMODULE
#include "Helper.h"

#include "Sensors.h"
#include "HWSensors.h"

#include "Logic.h"
#include "KnxHelper.h"

struct sRuntimeInfo
{
    uint32_t startupDelay;
    uint32_t heartbeatDelay;
};

sRuntimeInfo gRuntimeData;

Sensors gSensors;
HWSensors gHWSensors = HWSensors();
Logic gLogic;

bool _knx_configured = false;

    int freq = 1000;
    uint32_t blink_lastcall = millis();
    bool blink = false;

void ProcessHeartbeat()
{
    // the first heartbeat is send directly after startup delay of the device
    if (gRuntimeData.heartbeatDelay == 0 || delayCheck(gRuntimeData.heartbeatDelay, getDelayPattern(LOG_HeartbeatDelayBase)))
    {
        // we waited enough, let's send a heartbeat signal
        knx.getGroupObject(LOG_KoHeartbeat).value(true, getDPT(VAL_DPT_1));
        gRuntimeData.heartbeatDelay = millis();
        // debug entry point
        gSensors.debug();
        gLogic.debug();
    }
}

void ProcessReadRequests() {
    // this method is called after startup delay and executes read requests, which should just happen once after startup
    static bool sCalledProcessReadRequests = false;
    if (!sCalledProcessReadRequests)
    {
        gSensors.processReadRequests();
        gLogic.processReadRequests();
        sCalledProcessReadRequests = true;
    }
}

// true solgange der Start des gesamten Moduls verzögert werden soll
bool startupDelay()
{
    return !delayCheck(gRuntimeData.startupDelay, getDelayPattern(LOG_StartupDelayBase, true));
}

bool processDiagnoseCommand()
{
    char *lBuffer = gLogic.getDiagnoseBuffer();
    bool lOutput = false;
    if (lBuffer[0] == 'v')
    {
        // Command v: retrun fimware version, do not forward this to logic,
        // because it means firmware version of the outermost module
        // sprintf(lBuffer, "VER [%d] %d.%d", cFirmwareMajor, cFirmwareMinor, cFirmwareRevision);
        lOutput = true;
    }
    else
    {
        // let's check other modules for this command
        lOutput = gSensors.processDiagnoseCommand(lBuffer);
        if (!lOutput) lOutput = gLogic.processDiagnoseCommand();
    }
    return lOutput;
}

void ProcessDiagnoseCommand(GroupObject &iKo)
{
    // this method is called as soon as iKo is changed
    // an external change is expected
    // because this iKo also is changed within this method,
    // the method is called again. This might result in
    // an endless loop. This is prevented by the isCalled pattern.
    static bool sIsCalledProcessDiagnoseCommand = false;
    if (!sIsCalledProcessDiagnoseCommand)
    {
        sIsCalledProcessDiagnoseCommand = true;
        //diagnose is interactive and reacts on commands
        gLogic.initDiagnose(iKo);
        if (processDiagnoseCommand())
            gLogic.outputDiagnose(iKo);
        sIsCalledProcessDiagnoseCommand = false;
    }
};

void ProcessKoCallback(GroupObject &iKo)
{
    // check if we evaluate own KO
    if (iKo.asap() == LOG_KoDiagnose)
    {
        ProcessDiagnoseCommand(iKo);
    }
    else
    {
        gSensors.processInputKo(iKo);
        gLogic.processInputKo(iKo);
    }
}

void appLoop()
{
    if(millis() > blink_lastcall + freq)
    {
        digitalWrite(DBGLED1, blink);
        blink = !blink;
        blink_lastcall = millis();
        SERIAL_DEBUG.println("blink appLoop");
    }


    if (knx.configured())
    {
        _knx_configured = true;
    }
    else
    {
        _knx_configured = false;
        multicore_reset_core1();
        return;
    }

    // handle KNX stuff
    if (startupDelay())
        return;

    // at this point startup-delay is done
    // we process heartbeat
    ProcessHeartbeat();
    ProcessReadRequests();
    gSensors.loop();
    gLogic.loop();
}

void appLoop_core1()
{
    int freq = 0;
    uint32_t blink_core1_lastcall = 0;
    bool blink_core1 = false;

    while(true)
    {
        if(_knx_configured)
        {
            gHWSensors.Loop();

            freq = 250;
        }
        else
        {
            freq = 100;
        }

        if(millis() > blink_core1_lastcall + freq)
        {
            digitalWrite(DBGLED2, blink_core1);
            blink_core1 = !blink_core1;
            blink_core1_lastcall = millis();
            SERIAL_DEBUG.println("blink appLoop_core1");
        }
    }
}

void appSetup(bool iSaveSupported)
{
    /*
    // try to get rid of occasional I2C lock...
    savePower();
    ledProg(true);
    ledInfo(true);
    delay(500);
    restorePower();
    */
    // check hardware availability
    //boardCheck();
    ledInfo(false);
    ledProg(false);

    if (knx.configured())
    {
        // 5 bit major, 5 bit minor, 6 bit revision
        // knx.bau().deviceObject().version(cFirmwareMajor << 11 | cFirmwareMinor << 6 | cFirmwareRevision);

        if (GroupObject::classCallback() == 0)
            GroupObject::classCallback(ProcessKoCallback);

        gRuntimeData.startupDelay = millis();
        gRuntimeData.heartbeatDelay = 0;

        const uint8_t pins[] = {
            THPCHANNEL_A_SCL,THPCHANNEL_A_SDA,
            THPCHANNEL_B_SCL,THPCHANNEL_B_SDA,
            THPCHANNEL_C_SCL,THPCHANNEL_C_SDA,
            THPCHANNEL_D_SCL,THPCHANNEL_D_SDA,
            THPCHANNEL_E_SCL,THPCHANNEL_E_SDA,
            THPCHANNEL_F_SCL,THPCHANNEL_F_SDA,
            THPCHANNEL_G_SCL,THPCHANNEL_G_SDA,
            THPCHANNEL_H_SCL,THPCHANNEL_H_SDA};

        uint8_t sensortypes[THP_ChannelCount];
        for(int i=0;i<THP_ChannelCount;i++)
            sensortypes[i] = knx.paramByte(THP_ParamBlockOffset+THP_ParamBlockSize*i+THP_Sensortype_);

        gHWSensors.Setup(pins, sensortypes);
        gSensors.setup(pins, &gHWSensors); // ToDo iSaveSupported
        gLogic.setup(iSaveSupported);

        multicore_launch_core1(appLoop_core1);
    }
}
#endif