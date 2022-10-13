/*
 * Copyright (c) 2015-2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_ints.h)
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(driverlib/ssi.h)
#include DeviceFamily_constructPath(driverlib/sys_ctrl.h)
#include DeviceFamily_constructPath(driverlib/udma.h)
#include DeviceFamily_constructPath(driverlib/ioc.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)
#include DeviceFamily_constructPath(driverlib/rom.h)

#include <ti/drivers/dma/UDMACC26XX.h>
#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/dpl/SwiP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPICC26X2DMA.h>

#define MAX_DMA_TRANSFER_AMOUNT     (1024)

/* SPI test control register */
#define SSI_O_TCR                   (0x00000080)
#define SSI_TCR_TESTFIFO_ENABLE     (0x2)
#define SSI_TCR_TESTFIFO_DISABLE    (0x0)
/* SPI test data register */
#define SSI_O_TDR                   (0x0000008C)

#define PARAMS_DATASIZE_MIN         (4)
#define PARAMS_DATASIZE_MAX         (16)

/* Allocate space for DMA control table entries */
ALLOCATE_CONTROL_TABLE_ENTRY(dmaSpi0TxControlTableEntry, UDMA_CHAN_SSI0_TX);
ALLOCATE_CONTROL_TABLE_ENTRY(dmaSpi0RxControlTableEntry, UDMA_CHAN_SSI0_RX);
ALLOCATE_CONTROL_TABLE_ENTRY(dmaSpi1TxControlTableEntry, UDMA_CHAN_SSI1_TX);
ALLOCATE_CONTROL_TABLE_ENTRY(dmaSpi1RxControlTableEntry, UDMA_CHAN_SSI1_RX);
ALLOCATE_CONTROL_TABLE_ENTRY(dmaSpi0TxAltControlTableEntry,
                             (UDMA_CHAN_SSI0_TX | UDMA_ALT_SELECT));
ALLOCATE_CONTROL_TABLE_ENTRY(dmaSpi0RxAltControlTableEntry,
                             (UDMA_CHAN_SSI0_RX | UDMA_ALT_SELECT));
ALLOCATE_CONTROL_TABLE_ENTRY(dmaSpi1TxAltControlTableEntry,
                             (UDMA_CHAN_SSI1_TX | UDMA_ALT_SELECT));
ALLOCATE_CONTROL_TABLE_ENTRY(dmaSpi1RxAltControlTableEntry,
                             (UDMA_CHAN_SSI1_RX | UDMA_ALT_SELECT));

/* API Function Prototypes */
void SPICC26X2DMA_close(SPI_Handle handle);
int_fast16_t SPICC26X2DMA_control(SPI_Handle handle,
                                  uint_fast16_t cmd,
                                  void *arg);
void SPICC26X2DMA_init(SPI_Handle handle);
SPI_Handle SPICC26X2DMA_open(SPI_Handle handle, SPI_Params *params);
static void SPICC26X2DMA_swiFxn (uintptr_t arg0, uintptr_t arg1);
bool SPICC26X2DMA_transfer(SPI_Handle handle, SPI_Transaction *transaction);
void SPICC26X2DMA_transferCancel(SPI_Handle handle);

/* Local Function Prototypes */
static void blockingTransferCallback(SPI_Handle handle,
                                     SPI_Transaction *msg);
static void configNextTransfer(SPICC26X2DMA_Object *object,
                               SPICC26X2DMA_HWAttrs const *hwAttrs);
static void csnCallback(PIN_Handle handle, PIN_Id pinId);
static void flushFifos(SPICC26X2DMA_HWAttrs const *hwAttrs);
static inline uint32_t getDmaChannelNumber(uint32_t x);
static void initHw(SPI_Handle handle);
static bool initIO(SPI_Handle handle);
static inline void primeTransfer(SPICC26X2DMA_Object *object,
                                 SPICC26X2DMA_HWAttrs const *hwAttrs);
static inline void releaseConstraint(uint32_t txBufAddr);
static inline void setConstraint(uint32_t txBufAddr);
static inline void spiPollingTransfer(SPICC26X2DMA_Object *object,
                                      SPICC26X2DMA_HWAttrs const *hwAttrs,
                                      SPI_Transaction *transaction);
static int spiPostNotify(unsigned int eventType,
                         uintptr_t eventArg,
                         uintptr_t clientArg);
static inline bool spiBusy(SPICC26X2DMA_Object *object,
                           SPICC26X2DMA_HWAttrs const *hwAttrs);

/* SPI function table for SPICC26X2DMA implementation */
const SPI_FxnTable SPICC26X2DMA_fxnTable = {
    SPICC26X2DMA_close,
    SPICC26X2DMA_control,
    SPICC26X2DMA_init,
    SPICC26X2DMA_open,
    SPICC26X2DMA_transfer,
    SPICC26X2DMA_transferCancel
};

/* Mapping SPI frame format from generic driver to CC26XX driverlib */
static const uint32_t frameFormat[] = {
    SSI_FRF_MOTO_MODE_0,    /* SPI_POLO_PHA0 */
    SSI_FRF_MOTO_MODE_1,    /* SPI_POLO_PHA1 */
    SSI_FRF_MOTO_MODE_2,    /* SPI_POL1_PHA0 */
    SSI_FRF_MOTO_MODE_3,    /* SPI_POL1_PHA1 */
    SSI_FRF_TI,             /* SPI_TI */
    SSI_FRF_NMW             /* SPI_MW */
};

/*
 * These lookup tables are used to configure the DMA channels for the
 * appropriate (8bit or 16bit) transfer sizes.
 */
static const uint32_t dmaTxConfig[] = {
    UDMA_MODE_PINGPONG | UDMA_SIZE_8  | UDMA_SRC_INC_8  | UDMA_DST_INC_NONE |
    UDMA_ARB_4,
    UDMA_MODE_PINGPONG | UDMA_SIZE_16 | UDMA_SRC_INC_16 | UDMA_DST_INC_NONE |
    UDMA_ARB_4
};

static const uint32_t dmaRxConfig[] = {
    UDMA_MODE_PINGPONG | UDMA_SIZE_8  | UDMA_SRC_INC_NONE | UDMA_DST_INC_8  |
    UDMA_ARB_4,
    UDMA_MODE_PINGPONG | UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 |
    UDMA_ARB_4
};

static const uint32_t dmaNullConfig[] = {
    UDMA_MODE_PINGPONG | UDMA_SIZE_8  | UDMA_SRC_INC_NONE | UDMA_DST_INC_NONE |
    UDMA_ARB_4,
    UDMA_MODE_PINGPONG | UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_NONE |
    UDMA_ARB_4
};

/*
 *  ======== SPICC26X2DMA_close ========
 */
void SPICC26X2DMA_close(SPI_Handle handle)
{
    SPICC26X2DMA_Object          *object = handle->object;
    SPICC26X2DMA_HWAttrs const *hwAttrs = handle->hwAttrs;

    SSIDisable(hwAttrs->baseAddr);

    HwiP_destruct(&(object->hwi));

    UDMACC26XX_close(object->udmaHandle);

    SwiP_destruct(&(object->swi));

    if (object->transferMode == SPI_MODE_BLOCKING) {
        SemaphoreP_destruct(&(object->transferComplete));
    }

    PIN_close(object->pinHandle);

    Power_releaseDependency(hwAttrs->powerMngrId);

    Power_unregisterNotify(&object->spiPostObj);

    object->isOpen = false;
}

/*!
 *  @brief  Function for setting control parameters of the SPI driver
 *          after it has been opened.
 *
 *  @pre    SPICC26X2DMA_open() has to be called first.
 *          Calling context: Hwi, Swi, Task
 *
 *  @param  handle A SPI handle returned from SPICC26X2DMA_open()
 *
 *  @param  cmd  The command to execute, supported commands are:
 *  | Command                                   | Description                  |
 *  |-------------------------------------------|------------------------------|
 *  | ::SPICC26X2DMA_CMD_RETURN_PARTIAL_ENABLE  | Enable RETURN_PARTIAL        |
 *  | ::SPICC26X2DMA_CMD_RETURN_PARTIAL_DISABLE | Disable RETURN_PARTIAL       |
 *  | ::SPICC26X2DMA_CMD_SET_CSN_PIN            | Re-configure chip select pin |
 *  | ::SPICC26X2DMA_CMD_SET_MANUAL             | Enable manual start mode     |
 *  | ::SPICC26X2DMA_CMD_CLR_MANUAL             | Disable manual start mode    |
 *  | ::SPICC26X2DMA_CMD_MANUAL_START           | Perform a manual start       |
 *
 *  @param  *arg  Pointer to command arguments.
 *
 *  @return ::SPI_STATUS_SUCCESS if success, or error code if error.
 */
int_fast16_t SPICC26X2DMA_control(SPI_Handle handle,
                                  uint_fast16_t cmd,
                                  void *arg)
{
    SPICC26X2DMA_Object          *object = handle->object;
    SPICC26X2DMA_HWAttrs const *hwAttrs = handle->hwAttrs;
    PIN_Config                    pinConfig;
    PIN_Id                        pinId;

    /* Initialize return value*/
    int ret = SPI_STATUS_ERROR;

    /* Perform command */
    switch(cmd) {
        case SPICC26X2DMA_CMD_RETURN_PARTIAL_ENABLE:
            /* Enable RETURN_PARTIAL if slave mode is enabled */

            if(object->mode == SPI_SLAVE){
                object->returnPartial = SPICC26X2DMA_retPartEnabledIntNotSet;
                ret = SPI_STATUS_SUCCESS;
            }
            else{
                /* Partial return not available in master mode. */
                ret = SPI_STATUS_ERROR;
            }
            break;

        case SPICC26X2DMA_CMD_RETURN_PARTIAL_DISABLE:
            /* Disable RETURN_PARTIAL */
            PIN_setInterrupt(object->pinHandle, object->csnPin);
            object->returnPartial = SPICC26X2DMA_retPartDisabled;
            ret = SPI_STATUS_SUCCESS;
            break;

        case SPICC26X2DMA_CMD_SET_CSN_PIN:
            pinId = ((*(PIN_Id *) arg));

            /* Configure CSN pin and remap PIN_ID to new CSN pin specified by
            arg */
            if (object->mode == SPI_SLAVE) {
                pinConfig = PIN_INPUT_EN | PIN_PULLUP | pinId;
            }
            else {
                pinConfig = PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL |
                            PIN_INPUT_DIS | PIN_DRVSTR_MED | pinId;
            }

            if (pinId != PIN_UNASSIGNED) {
                /* Attempt to add the new pin */
                if (PIN_add(object->pinHandle, pinConfig) == PIN_SUCCESS) {
                    /* Configure pin mux */
                    PINCC26XX_setMux(object->pinHandle,
                                     pinId,
                                    (hwAttrs->baseAddr == SSI0_BASE ?
                                    IOC_PORT_MCU_SSI0_FSS :
                                    IOC_PORT_MCU_SSI1_FSS));

                    /* Remove old pin and revert to default setting
                     * specified in the board file */
                    PIN_remove(object->pinHandle, object->csnPin);

                    /* Keep track of current CSN pin */
                    object->csnPin = pinId;

                    /* Set return value to indicate success */
                    ret = SPI_STATUS_SUCCESS;
                }
            }
            else {
                /* We want to use software ctrl CSN. Hence, undo any prior
                 * hardware CSN pin muxing. Remove old pin and revert to default
                 * setting specified in the board file (implicitly sets IO
                 * muxing to GPIO mode) */
                PIN_remove(object->pinHandle, object->csnPin);

                /* Keep track of current CSN pin */
                object->csnPin = pinId;

                /* Set return value to indicate success */
                ret = SPI_STATUS_SUCCESS;
            }
            break;
        case SPICC26X2DMA_CMD_SET_MANUAL:
            /* If a transaction is queued, do not modify */
            if (object->headPtr == NULL) {
                object->manualStart = true;
                ret = SPI_STATUS_SUCCESS;
            }
            break;
        case SPICC26X2DMA_CMD_CLR_MANUAL:
            /* If a transaction is queued, do not modify */
            if (object->headPtr == NULL) {
                object->manualStart = false;
                ret = SPI_STATUS_SUCCESS;
            }
            break;
        case SPICC26X2DMA_CMD_MANUAL_START:
            if (object->headPtr != NULL &&
                object->manualStart) {
                SSIDMAEnable(hwAttrs->baseAddr, SSI_DMA_TX | SSI_DMA_RX);
                UDMACC26XX_channelEnable(object->udmaHandle,
                                         hwAttrs->rxChannelBitMask |
                                         hwAttrs->txChannelBitMask);
                SSIEnable(hwAttrs->baseAddr);
                ret = SPI_STATUS_SUCCESS;
            }
            break;
        default:
            /* This command is not defined */
            ret = SPI_STATUS_UNDEFINEDCMD;
            break;
    }

    return (ret);
}

/*
 *  ======== SPICC26X2DMA_hwiFxn ========
 */
static void SPICC26X2DMA_hwiFxn (uintptr_t arg)
{
    uint32_t                      freeChannel;
    uint32_t                      intStatus;
    uintptr_t                     key;
    SPI_Transaction              *completedList;
    size_t                       *transferSize;
    volatile tDMAControlTable    *rxDmaTableEntry;
    volatile tDMAControlTable    *txDmaTableEntry;
    SPICC26X2DMA_Object          *object = ((SPI_Handle) arg)->object;
    SPICC26X2DMA_HWAttrs const   *hwAttrs = ((SPI_Handle) arg)->hwAttrs;
    uint8_t                       i;

    intStatus = SSIIntStatus(hwAttrs->baseAddr, true);
    SSIIntClear(hwAttrs->baseAddr, intStatus);

    if (intStatus & SSI_RXOR) {
        key = HwiP_disable();

        if (object->headPtr != NULL) {
            /*
             * RX overrun during a transfer; mark the current transfer
             * as failed & cancel all remaining transfers.
             */
            object->headPtr->status = SPI_TRANSFER_FAILED;

            HwiP_restore(key);

            SPICC26X2DMA_transferCancel((SPI_Handle) arg);
        }
        else {
            SSIDisable(hwAttrs->baseAddr);

            /* Disable DMA and clear DMA interrupts */
            SSIDMADisable(hwAttrs->baseAddr, SSI_DMA_TX | SSI_DMA_RX);
            UDMACC26XX_channelDisable(object->udmaHandle,
                                      hwAttrs->rxChannelBitMask |
                                      hwAttrs->txChannelBitMask);
            UDMACC26XX_clearInterrupt(object->udmaHandle,
                                      hwAttrs->rxChannelBitMask |
                                      hwAttrs->txChannelBitMask);
            SSIIntDisable(hwAttrs->baseAddr, SSI_RXOR);
            SSIIntClear(hwAttrs->baseAddr, SSI_RXOR);

            /* Clear out the FIFO by resetting SPI module and re-initting */
            flushFifos(hwAttrs);

            HwiP_restore(key);
        }
    }
    else {
        UDMACC26XX_clearInterrupt(object->udmaHandle,
                                  hwAttrs->rxChannelBitMask |
                                  hwAttrs->txChannelBitMask);

        /*
         * We check both channels for completion; this is done in case the
         * second channel finishes while we are still configuring the first.
         */
        for (i = 0; i < 2; i++) {
            if (object->headPtr == NULL){
                /* When i was 0, we finished the last transaction */
                break;
            }

            if (object->activeChannel == UDMA_PRI_SELECT) {
                transferSize = &object->priTransferSize;

                rxDmaTableEntry = (hwAttrs->baseAddr == SSI0_BASE) ?
                                   &dmaSpi0RxControlTableEntry :
                                   &dmaSpi1RxControlTableEntry;

                txDmaTableEntry = (hwAttrs->baseAddr == SSI0_BASE) ?
                                   &dmaSpi0TxControlTableEntry :
                                   &dmaSpi1TxControlTableEntry;
            }
            else {
                transferSize = &object->altTransferSize;

                rxDmaTableEntry = (hwAttrs->baseAddr == SSI0_BASE) ?
                                   &dmaSpi0RxAltControlTableEntry :
                                   &dmaSpi1RxAltControlTableEntry;

                txDmaTableEntry = (hwAttrs->baseAddr == SSI0_BASE) ?
                                   &dmaSpi0TxAltControlTableEntry :
                                   &dmaSpi1TxAltControlTableEntry;
            }

            /*
             * The SPI TX FIFO continuously requests the DMA to fill it if there
             * is space available.  If there are no more frames to put in the
             * FIFO we run into a situation where DMA TX will cause undesired
             * interrupts.  To prevent many undesired interrupts disable DMA_TX
             * uf there are no more frames to load into the FIFO & there are no
             * pending queued transactions.
             */
            if (UDMACC26XX_channelDone(object->udmaHandle, hwAttrs->txChannelBitMask) &&
               (txDmaTableEntry->ui32Control & UDMA_MODE_M) == UDMA_MODE_STOP &&
                object->framesQueued == object->headPtr->count &&
                object->headPtr->nextPtr == NULL){
                    SSIDMADisable(hwAttrs->baseAddr, SSI_DMA_TX);
                    UDMACC26XX_clearInterrupt(object->udmaHandle,
                                              hwAttrs->txChannelBitMask);
            }

            if((rxDmaTableEntry->ui32Control & UDMA_MODE_M) == UDMA_MODE_STOP &&
                *transferSize != 0) {
                key = HwiP_disable();

                object->framesTransferred += *transferSize;
                freeChannel = object->activeChannel;
                object->activeChannel = (freeChannel == UDMA_PRI_SELECT) ?
                    UDMA_ALT_SELECT : UDMA_PRI_SELECT;

                /*
                 * Set the channel's transfer size to 0; 0 lets
                 * configNextTransfer() know that there is a free channel.
                 */
                *transferSize = 0;

                if ((object->framesQueued) < (object->headPtr->count) ||
                    (object->framesTransferred) < (object->headPtr->count)) {
                    /*
                     * In this case we need to reconfigure the channel to
                     * continue transferring frames. configNextTransfer() will
                     * continue queuing frames for the current transfer or
                     * start the following transaction if necessary.
                     */
                    configNextTransfer(object, hwAttrs);

                    if (object->manualStart &&
                        UDMACC26XX_channelDone(object->udmaHandle, hwAttrs->txChannelBitMask)) {
                        /* Ping pong flow was broken, restart */
                        UDMACC26XX_channelEnable(object->udmaHandle,
                                                 hwAttrs->txChannelBitMask);
                    }

                    HwiP_restore(key);
                }
                else {
                    /*
                     * All data has been transferred for the current
                     * transaction. Set status & move the transaction to
                     * object->completedList. This is required because
                     * object->headPtr is moved to the following transaction.
                     * Also, transaction callbacks are executed in the driver
                     * SWI which will be posted later.
                     */
                    object->headPtr->status = SPI_TRANSFER_COMPLETED;

                    if (object->completedTransfers == NULL) {
                        /* List is empty; just add the transaction */
                        object->completedTransfers = object->headPtr;
                        completedList = object->completedTransfers;
                    }
                    else {
                        /* Traverse to the last element */
                        completedList = object->completedTransfers;
                        while (completedList->nextPtr != NULL) {
                            completedList = completedList->nextPtr;
                        }

                        /* Store the completed transaction at end of list */
                        completedList->nextPtr = object->headPtr;

                        /*
                         * Make sure we are pointing to the end of the list;
                         * we need to clear references in completed transfer
                         * after we move object->headPtr forward.
                         */
                        completedList = completedList->nextPtr;
                    }

                    /* Move the object->headPtr to the next transaction */
                    object->headPtr = object->headPtr->nextPtr;

                    /* Clear references in completed transfer */
                    completedList->nextPtr = NULL;

                    /* Update object variables for the following transfer. */
                    object->framesQueued =
                        (object->activeChannel == UDMA_PRI_SELECT) ?
                        object->priTransferSize : object->altTransferSize;
                    object->framesTransferred = 0;

                    if (object->headPtr != NULL) {
                        /* Reconfigure channel for following transaction */
                        configNextTransfer(object, hwAttrs);

                        if (object->manualStart &&
                            UDMACC26XX_channelDone(object->udmaHandle, hwAttrs->txChannelBitMask)) {
                            /* Ping pong flow was broken, restart */
                            UDMACC26XX_channelEnable(object->udmaHandle,
                                                     hwAttrs->txChannelBitMask);
                        }
                    }
                    else {
                        /* No more queued transfers; disable DMA & SPI */
                        SSIDMADisable(hwAttrs->baseAddr,
                                      SSI_DMA_TX |
                                      SSI_DMA_RX);

                        /*
                         * For this driver implementation the peripheral is kept
                         * active until either a FIFO-overrun occurs or
                         * SPI_transferCancel() is executed.
                         */
                    }

                    HwiP_restore(key);

                    /* Post driver SWI to execute transaction callbacks */
                    SwiP_post(&(object->swi));
                }
            }
        }
    }
}

/*
 *  ======== SPICC26X2DMA_init ========
 */
void SPICC26X2DMA_init(SPI_Handle handle)
{
    ((SPICC26X2DMA_Object *) handle->object)->isOpen = false;
}

/*
 *  ======== SPICC26X2DMA_open ========
 */
SPI_Handle SPICC26X2DMA_open(SPI_Handle handle, SPI_Params *params)
{
    union {
        HwiP_Params               hwiParams;
        SwiP_Params               swiParams;
    } paramsUnion;
    uint32_t                      key;
    SPICC26X2DMA_Object          *object = handle->object;
    SPICC26X2DMA_HWAttrs const *hwAttrs = handle->hwAttrs;

    key = HwiP_disable();

    /* Failure conditions */
    if (object->isOpen ||
        params->dataSize > PARAMS_DATASIZE_MAX ||
        params->dataSize < PARAMS_DATASIZE_MIN) {
        HwiP_restore(key);

        return (NULL);
    }
    object->isOpen = true;

    HwiP_restore(key);

    DebugP_assert((params->dataSize >= 4) && (params->dataSize <= 16));

    object->bitRate = params->bitRate;
    object->dataSize = params->dataSize;
    object->mode = params->mode;
    object->transferMode = params->transferMode;
    object->transferTimeout = params->transferTimeout;
    object->returnPartial = SPICC26X2DMA_retPartDisabled;
    object->headPtr = NULL;
    object->tailPtr = NULL;
    object->completedTransfers = NULL;
    object->format = frameFormat[params->frameFormat];
    object->txScratchBuf = hwAttrs->defaultTxBufValue;
    object->busyBit = (params->mode == SPI_MASTER ? SSI_SR_BSY : SSI_SR_TFE);
    object->manualStart = false;

    Power_setDependency(hwAttrs->powerMngrId);

    initHw(handle);

    /* CSN is initialized using hwAttrs, but can be re-configured later */
    object->csnPin = hwAttrs->csnPin;

    /*
     * Configure IOs after hardware has been initialized so that IOs aren't
     * toggled unnecessary
     */
    if (!initIO(handle)) {
        /*
         * Trying to use SPI driver when some other driver or application
         * has already allocated these pins, error!
         */

        Power_releaseDependency(hwAttrs->powerMngrId);

        object->isOpen = false;

        return (NULL);
    }

    HwiP_Params_init(&paramsUnion.hwiParams);
    paramsUnion.hwiParams.arg = (uintptr_t) handle;
    paramsUnion.hwiParams.priority = hwAttrs->intPriority;
    HwiP_construct(&(object->hwi),
                   (int) hwAttrs->intNum, SPICC26X2DMA_hwiFxn,
                   &paramsUnion.hwiParams);

    SwiP_Params_init(&paramsUnion.swiParams);
    paramsUnion.swiParams.arg0 = (uintptr_t)handle;
    paramsUnion.swiParams.priority = hwAttrs->swiPriority;
    SwiP_construct(&(object->swi),
                   SPICC26X2DMA_swiFxn,
                   &(paramsUnion.swiParams));

    object->udmaHandle = UDMACC26XX_open();

    /*
     * Configure PIN driver for CSN callback in optional RETURN_PARTIAL
     * slave mode
     */
    if (object->mode == SPI_SLAVE) {
        PIN_registerIntCb(object->pinHandle, csnCallback);
        PIN_setUserArg(object->pinHandle, (uintptr_t) handle);
    }

    Power_registerNotify(&object->spiPostObj,
                         PowerCC26XX_AWAKE_STANDBY,
                         (Power_NotifyFxn) spiPostNotify,
                         (uint32_t) handle);

    if (object->transferMode == SPI_MODE_BLOCKING) {
        /*
         * Create a semaphore to block task execution for the duration of the
         * SPI transfer
         */
        SemaphoreP_constructBinary(&(object->transferComplete), 0);
        object->transferCallbackFxn = blockingTransferCallback;
    }
    else {
        DebugP_assert(params->transferCallbackFxn != NULL);
        object->transferCallbackFxn = params->transferCallbackFxn;
    }

    return (handle);
}

/*
 *  ======== SPICC26X2DMA_swiFxn ========
 */
static void SPICC26X2DMA_swiFxn(uintptr_t arg0, uintptr_t arg1) {
    SPI_Transaction     *transaction;
    SPICC26X2DMA_Object *object = ((SPI_Handle) arg0)->object;

    while (object->completedTransfers != NULL) {
        transaction = object->completedTransfers;

        /* Move object->completedTransfers to the next transaction */
        object->completedTransfers = object->completedTransfers->nextPtr;

        transaction->nextPtr = NULL;

        /* Transaction complete; release power constraints */
        releaseConstraint((uint32_t) transaction->txBuf);

        /* Execute callback function for completed transfer */
        object->transferCallbackFxn((SPI_Handle) arg0, transaction);
    }
}

/*
 *  ======== SPICC26X2DMA_transfer ========
 */
bool SPICC26X2DMA_transfer(SPI_Handle handle, SPI_Transaction *transaction)
{
    uint8_t                       alignMask;
    bool                          buffersAligned;
    uintptr_t                     key;
    SPICC26X2DMA_Object          *object = handle->object;
    SPICC26X2DMA_HWAttrs const *hwAttrs = handle->hwAttrs;

    if (transaction->count == 0) {
        return (false);
    }

    key = HwiP_disable();

    /*
     * Make sure that the buffers are aligned properly.
     * alignMask is used to check if the RX/TX buffers addresses
     * are aligned to the frameSize.
     */
    alignMask = (object->dataSize < 9) ? 0x0 : 0x01;
    buffersAligned = ((((uint32_t) transaction->rxBuf & alignMask) == 0) &&
                     (((uint32_t) transaction->txBuf & alignMask) == 0));

    if (!buffersAligned ||
        (object->headPtr && object->transferMode == SPI_MODE_BLOCKING)) {
        transaction->status = SPI_TRANSFER_FAILED;

        HwiP_restore(key);

        return (false);
    }
    else {
        if (object->headPtr) {
            object->tailPtr->nextPtr = transaction;
            object->tailPtr = transaction;
            object->tailPtr->status = SPI_TRANSFER_QUEUED;
        }
        else {
            object->headPtr = transaction;
            object->tailPtr = transaction;

            object->framesQueued = 0;
            object->framesTransferred = 0;
            object->priTransferSize = 0;
            object->altTransferSize = 0;
            object->tailPtr->status =
                    (object->returnPartial != SPICC26X2DMA_retPartDisabled) ?
                     SPI_TRANSFER_PEND_CSN_ASSERT :
                     SPI_TRANSFER_STARTED;
        }

        object->tailPtr->nextPtr = NULL;
    }

    /* In slave mode, optionally enable callback on CSN de-assert */
    if (object->returnPartial == SPICC26X2DMA_retPartEnabledIntNotSet) {
        object->returnPartial = SPICC26X2DMA_retPartEnabledIntSet;
        PIN_setInterrupt(object->pinHandle, object->csnPin | PIN_IRQ_BOTHEDGES);
    }

    /* Set constraints to guarantee transaction */
    setConstraint((uint32_t)transaction->txBuf);

    /*
     * Polling transfer if BLOCKING mode & transaction->count < threshold
     * Slaves not allowed to use polling unless timeout is disabled
     */
    if (object->transferMode == SPI_MODE_BLOCKING &&
        transaction->count < hwAttrs->minDmaTransferSize &&
        (object->mode == SPI_MASTER ||
        object->transferTimeout == SPI_WAIT_FOREVER)) {
        HwiP_restore(key);

        spiPollingTransfer(object, hwAttrs, transaction);

        /* Release constraint since transaction is done */
        releaseConstraint((uint32_t) transaction->txBuf);

        /* Transaction completed; set status & mark SPI ready */
        object->headPtr->status = SPI_TRANSFER_COMPLETED;
        object->headPtr = NULL;
        object->tailPtr = NULL;
    }
    else {
        /*
         * Perform a DMA backed SPI transfer; we need exclusive access while
         * priming the transfer to prevent race conditions with
         * SPICC26X2DMA_transferCancel().
         */
        primeTransfer(object, hwAttrs);

        /* Enable the RX overrun interrupt in the SSI module */
        SSIIntEnable(hwAttrs->baseAddr, SSI_RXOR);

        HwiP_restore(key);

        if (object->transferMode == SPI_MODE_BLOCKING) {
            if (SemaphoreP_OK != SemaphoreP_pend(&(object->transferComplete),
                                                 object->transferTimeout)) {
                /* Timeout occurred; cancel the transfer */
                object->headPtr->status = SPI_TRANSFER_FAILED;
                SPICC26X2DMA_transferCancel(handle);

                /*
                 * SPICC26X2DMA_transferCancel() performs callback which posts
                 * transferComplete semaphore. This call consumes this extra
                 * post.
                 */
                SemaphoreP_pend(&(object->transferComplete),
                                SemaphoreP_NO_WAIT);

                return (false);
            }
        }
    }
    return (true);
}

/*
 *  ======== SPICC26X2DMA_transferCancel ========
 */
void SPICC26X2DMA_transferCancel(SPI_Handle handle) {
    uintptr_t                    key;
    uint32_t                     temp;
    SPI_Transaction              *tempPtr;
    volatile tDMAControlTable    *rxDmaTableEntry;
    volatile tDMAControlTable    *rxDmaTableAltEntry;
    SPICC26X2DMA_Object          *object = handle->object;
    SPICC26X2DMA_HWAttrs const   *hwAttrs = handle->hwAttrs;

    /*
     * Acquire exclusive access to the driver.  Required to prevent race
     * conditions if preempted by code trying to configure another transfer.
     */
    key = HwiP_disable();

    if (object->headPtr == NULL) {
        HwiP_restore(key);

        return;
    }

    /*
     * There are 2 use cases in which to call transferCancel():
     *   1.  The driver is in CALLBACK mode.
     *   2.  The driver is in BLOCKING mode & there has been a transfer timeout.
     */
    if (object->transferMode != SPI_MODE_BLOCKING ||
        object->headPtr->status == SPI_TRANSFER_FAILED ||
        object->headPtr->status == SPI_TRANSFER_CSN_DEASSERT) {

        /* Prevent interrupt while canceling the transfer */
        HwiP_disableInterrupt(hwAttrs->intNum);

        /*
         * Disable the TX DMA channel first to stop feeding more frames to
         * the FIFO.  Next, wait until the TX FIFO is empty (all frames in
         * FIFO have been sent).  RX DMA channel is disabled later to allow
         * the DMA to move all frames already in FIFO to memory.
         */
        UDMACC26XX_channelDisable(object->udmaHandle,
                                  hwAttrs->txChannelBitMask);

        if (object->mode == SPI_MASTER) {
            /*
             * Wait until the TX FIFO is empty; this is to make sure the
             * chip select is deasserted before disabling the SPI.
             */
            while (SSIBusy(hwAttrs->baseAddr)) {}
        }

        SSIDisable(hwAttrs->baseAddr);

        /* Now disable the RX, DMA & interrupts */
        UDMACC26XX_channelDisable(object->udmaHandle,
                                  hwAttrs->rxChannelBitMask);
        SSIDMADisable(hwAttrs->baseAddr,
                      SSI_DMA_TX | SSI_DMA_RX);
        UDMACC26XX_clearInterrupt(object->udmaHandle,
                                  hwAttrs->rxChannelBitMask |
                                  hwAttrs->txChannelBitMask);
        SSIIntDisable(hwAttrs->baseAddr, SSI_RXOR);
        SSIIntClear(hwAttrs->baseAddr, SSI_RXOR);

        /*
         * Update transaction->count with the amount of frames which have
         * been transferred.
         */
        if (hwAttrs->baseAddr == SSI0_BASE) {
            rxDmaTableEntry = &dmaSpi0RxControlTableEntry;
            rxDmaTableAltEntry = &dmaSpi0RxAltControlTableEntry;
        }
        else {
            rxDmaTableEntry = &dmaSpi1RxControlTableEntry;
            rxDmaTableAltEntry = &dmaSpi1RxAltControlTableEntry;
        }

        object->headPtr->count = object->framesTransferred;
        if (object->priTransferSize) {
            temp = UDMACC26XX_GET_TRANSFER_SIZE(rxDmaTableEntry->ui32Control);

            if (temp <= object->priTransferSize) {
                object->headPtr->count += (object->priTransferSize - temp);
             }
        }

        if (object->altTransferSize) {
            temp =
            UDMACC26XX_GET_TRANSFER_SIZE(rxDmaTableAltEntry->ui32Control);

            if (temp <= object->altTransferSize) {
                object->headPtr->count += (object->altTransferSize - temp);
            }
        }

        /*
         * Disables peripheral, clears all registers & reinitializes it to
         * parameters used in SPI_open()
         */
        initHw(handle);

        HwiP_clearInterrupt(hwAttrs->intNum);
        HwiP_enableInterrupt(hwAttrs->intNum);

        /*
         * Go through all queued transfers; set status CANCELED (if we did
         * not cancel due to timeout).  The object->headPtr->count is
         * stored/restored temporarily.
         */
        temp = object->headPtr->count;
        tempPtr = object->headPtr;

        while (tempPtr != NULL) {
            if (tempPtr->status != SPI_TRANSFER_FAILED &&
                tempPtr->status != SPI_TRANSFER_CSN_DEASSERT) {
                tempPtr->status = SPI_TRANSFER_CANCELED;
            }

            tempPtr->count = 0;
            tempPtr = tempPtr->nextPtr;
        }
        object->headPtr->count = temp;

        /* Add all cancelled transactions to object->completedTransfers */
        tempPtr = object->completedTransfers;
        if (tempPtr == NULL) {
            /* Empty list; just add all of the cancelled transactions */
            object->completedTransfers = object->headPtr;
        }
        else {
            /* Move through the list until we reach the last element */
            while (tempPtr->nextPtr != NULL) {
                tempPtr = tempPtr->nextPtr;
            }

            /* Add all of the cancelled transactions */
            tempPtr->nextPtr = object->headPtr;
        }

        /* Clear all driver object variables*/
        object->headPtr = NULL;
        object->tailPtr = NULL;
        object->framesQueued = 0;
        object->framesTransferred = 0;
        object->priTransferSize = 0;
        object->altTransferSize = 0;

        HwiP_restore(key);

        /*
         * All transactions have been marked as cancelled & added to
         * object->completedTransfers.  Post the driver SWI to execute
         * callback functions.
         */
        SwiP_post(&(object->swi));

        /* Must return here; do not call HwiP_restore() twice */
        return;
    }

    HwiP_restore(key);
}

/*
 *  ======== blockingTransferCallback ========
 */
static void blockingTransferCallback(SPI_Handle handle, SPI_Transaction *msg)
{
    SPICC26X2DMA_Object *object = handle->object;

    SemaphoreP_post(&(object->transferComplete));
}

/*
 *  ======== configNextTransfer ========
 *  This function must be executed with interrupts disabled.
 */
static void configNextTransfer(SPICC26X2DMA_Object *object,
                               SPICC26X2DMA_HWAttrs const *hwAttrs)
{
    size_t                     framesQueued;
    uint32_t                   transferAmt;
    SPI_Transaction           *transaction;
    volatile tDMAControlTable *rxDmaTableEntry;
    volatile tDMAControlTable *txDmaTableEntry;
    uint8_t                    optionsIndex;

    /*
     * The DMA options vary according to data frame size; options for 8-bit
     * data (or smaller) are in index 0.  Options for larger frame sizes are
     * in index 1.
     *
     * optionsIndex was originally calculated by:
     *     optionsIndex = (object->dataSize < 9) ? 0x00 : 0x01;
     *
     * However, the IAR compiler generated incorrect assembly:
     *
     *  configNextTransfer:
     *         0x5140: 0xe92d 0x41fc  PUSH.W    {R2-R8, LR}
     *      optionsIndex = (object->dataSize < 9) ? 0x00 : 0x01;
     *         0x5144: 0xf100 0x0594  ADD.W     R5, R0, #148            ; 0x94
     *         0x5148: 0x6aaa         LDR       R2, [R5, #0x28]
     *         0x514a: 0x2a08         CMP       R2, #8
     *         0x514c: 0x419b         SBCS      R3, R3, R3
     *         0x514e: 0x43db         MVNS      R3, R3
     *         0x5150: 0x0fdb         LSRS      R3, R3, #31
     *
     * To work around this issue is calculated as follows:
     */
    optionsIndex = ((int32_t)(object->dataSize - 0x08) > 0) ? 0x01 : 0x00;

    /*
     * object->framesQueued keeps track of how many frames (of the current
     * transaction) have been configured for DMA transfer.  If
     * object->framesQueued == transaction->count; all frames have been queued
     * & we should configure the free DMA channel to send the next transaction.
     * When the current transaction has completed; object->framesQueued
     * will be updated (in the ISR) to reflect the amount of frames queued
     * of the following transaction.
     */
    transaction = object->headPtr;
    if (object->framesQueued < transaction->count) {
        framesQueued = object->framesQueued;
    }
    else {
        transaction = object->headPtr->nextPtr;
        if (transaction == NULL) {
            /* There are no queued transactions */
            return;
        }

        framesQueued = 0;
        transaction->status = SPI_TRANSFER_STARTED;
    }

    /*
     * The DMA has a max transfer amount of 1024.  If the transaction is
     * greater; we must transfer it in chunks.  framesQueued keeps track of
     * how much data has been queued for transfer.
     */
    if ((transaction->count - framesQueued) > MAX_DMA_TRANSFER_AMOUNT) {
        transferAmt = MAX_DMA_TRANSFER_AMOUNT;
    }
    else {
        transferAmt = transaction->count - framesQueued;
    }

    /* Determine free channel & mark it as used by setting transfer size */
    if (object->priTransferSize == 0) {
        object->priTransferSize = transferAmt;

        if (hwAttrs->baseAddr == SSI0_BASE) {
            rxDmaTableEntry = &dmaSpi0RxControlTableEntry;
            txDmaTableEntry = &dmaSpi0TxControlTableEntry;
        }
        else {
            rxDmaTableEntry = &dmaSpi1RxControlTableEntry;
            txDmaTableEntry = &dmaSpi1TxControlTableEntry;
        }
    }
    else {
        object->altTransferSize = transferAmt;

        if (hwAttrs->baseAddr == SSI0_BASE) {
            rxDmaTableEntry = &dmaSpi0RxAltControlTableEntry;
            txDmaTableEntry = &dmaSpi0TxAltControlTableEntry;
        }
        else {
            rxDmaTableEntry = &dmaSpi1RxAltControlTableEntry;
            txDmaTableEntry = &dmaSpi1TxAltControlTableEntry;
        }
    }

    /* Setup the TX transfer buffers & characteristics */
    if (transaction->txBuf) {
        txDmaTableEntry->ui32Control = dmaTxConfig[optionsIndex];

        /*
         * Add an offset for the amount of data transfered.  The offset is
         * calculated by: object->framesQueued * (optionsIndex + 1).  This
         * accounts for 8 or 16-bit sized transfers.
         */
        txDmaTableEntry->pvSrcEndAddr =
            (void *)((uint32_t) transaction->txBuf +
            (uint32_t) (framesQueued * (optionsIndex + 1)) +
            (transferAmt << optionsIndex) - 1);
    }
    else {
        txDmaTableEntry->ui32Control = dmaNullConfig[optionsIndex];
        txDmaTableEntry->pvSrcEndAddr = (void *) &(object->txScratchBuf);
    }
    txDmaTableEntry->pvDstEndAddr = (void *) (hwAttrs->baseAddr + SSI_O_DR);
    txDmaTableEntry->ui32Control |=
        UDMACC26XX_SET_TRANSFER_SIZE((uint16_t) transferAmt);

    /* Setup the RX transfer buffers & characteristics */
    if (transaction->rxBuf) {
        rxDmaTableEntry->ui32Control = dmaRxConfig[optionsIndex];

        /*
         * Add an offset for the amount of data transfered.  The offset is
         * calculated by: object->framesQueued * (optionsIndex + 1).  This
         * accounts for 8 or 16-bit sized transfers.
         */

        rxDmaTableEntry->pvDstEndAddr =
            (void *) ((uint32_t) transaction->rxBuf +
            (uint32_t) (framesQueued * (optionsIndex+ 1)) +
            (transferAmt << optionsIndex) - 1);
    }
    else {
        rxDmaTableEntry->ui32Control = dmaNullConfig[optionsIndex];
        rxDmaTableEntry->pvDstEndAddr = &object->rxScratchBuf;
    }
    rxDmaTableEntry->pvSrcEndAddr = (void *) (hwAttrs->baseAddr + SSI_O_DR);
    rxDmaTableEntry->ui32Control |=
        UDMACC26XX_SET_TRANSFER_SIZE((uint16_t) transferAmt);

    if (transaction == object->headPtr) {
        /*
         * Only update object->framesQueued if we are configuring a DMA
         * channel for the current transaction.
         */
        object->framesQueued += transferAmt;
    }

    if (!object->manualStart) {
        /* Enable DMA to generate interrupt on SPI peripheral */
        SSIDMAEnable(hwAttrs->baseAddr, SSI_DMA_TX | SSI_DMA_RX);
        UDMACC26XX_channelEnable(object->udmaHandle,
                                 hwAttrs->rxChannelBitMask |
                                 hwAttrs->txChannelBitMask);
    }

    return;
}

/*
 *  ======== csnCallback ========
 *  Slave mode optional callback function for when the CSN is asserted &
 *  deasserted.
 */
static void csnCallback(PIN_Handle handle, PIN_Id pinId)
{
    uintptr_t            key;
    SPICC26X2DMA_Object *object;
    SPI_Handle           spiHandle;

    spiHandle = (SPI_Handle) PIN_getUserArg(handle);
    object = spiHandle->object;

    /* Transfer started if CSN low */
    if (!PIN_getInputValue(object->csnPin)) {
        key = HwiP_disable();

        if (object->headPtr != NULL) {
            /* Indicate transaction started */
            object->headPtr->status = SPI_TRANSFER_STARTED;
        }
        else {
            /* Disable all interrupts */
            PIN_setInterrupt(handle, object->csnPin);
            object->returnPartial = SPICC26X2DMA_retPartEnabledIntNotSet;
        }

        HwiP_restore(key);
    }

    /* Cancel transfer if CSN high */
    if (PIN_getInputValue(object->csnPin)) {
        key = HwiP_disable();

        /* Disable all interrupts */
        PIN_setInterrupt(handle, object->csnPin);
        object->returnPartial = SPICC26X2DMA_retPartEnabledIntNotSet;

        /* Indicate why the transaction completed */
        if (object->headPtr != NULL) {
            object->headPtr->status = SPI_TRANSFER_CSN_DEASSERT;
        }

        HwiP_restore(key);

        /* Cancel the current transaction */
        SPICC26X2DMA_transferCancel(spiHandle);
    }
}

/*
 *  ======== flushFifos ========
 */
static void flushFifos(SPICC26X2DMA_HWAttrs const *hwAttrs)
{
    /* Flush RX FIFO */
    while(HWREG(hwAttrs->baseAddr + SSI_O_SR) & SSI_RX_NOT_EMPTY) {
        /* Read element from RX FIFO and discard */
        HWREG(hwAttrs->baseAddr + SSI_O_DR);
    }

    /* Enable TESTFIFO mode */
    HWREG(hwAttrs->baseAddr + SSI_O_TCR) = SSI_TCR_TESTFIFO_ENABLE;

    /* Flush TX FIFO */
    while(!(HWREG(hwAttrs->baseAddr + SSI_O_SR) & SSI_TX_EMPTY)) {
        /* Read element from TX FIFO and discard */
        HWREG(hwAttrs->baseAddr + SSI_O_TDR);
    }

    /* Disable TESTFIFO mode */
    HWREG(hwAttrs->baseAddr + SSI_O_TCR) = SSI_TCR_TESTFIFO_DISABLE;
}

/*
 *  ======== getDmaChannelNumber ========
 */
static inline uint32_t getDmaChannelNumber(uint32_t x) {
#if defined(__TI_COMPILER_VERSION__)
    return ((uint32_t) __clz(__rbit(x)));
#elif defined(__GNUC__)
    return ((uint32_t) __builtin_ctz(x));
#elif defined(__IAR_SYSTEMS_ICC__)
    return ((uint32_t) __CLZ(__RBIT(x)));
#else
    #error "Unsupported compiler"
#endif
}

/*
 *  ======== initHw ========
 */
static void initHw(SPI_Handle handle) {
    ClockP_FreqHz              freq;
    SPICC26X2DMA_Object        *object = handle->object;
    SPICC26X2DMA_HWAttrs const *hwAttrs = handle->hwAttrs;

    flushFifos(hwAttrs);

    /* Disable SSI operation */
    SSIDisable(hwAttrs->baseAddr);

    /* Disable SPI module interrupts */
    SSIIntDisable(hwAttrs->baseAddr, SSI_RXOR | SSI_RXFF | SSI_RXTO | SSI_TXFF);
    SSIIntClear(hwAttrs->baseAddr, SSI_RXOR | SSI_RXTO);

    /* Set the SPI configuration */
    ClockP_getCpuFreq(&freq);
    SSIConfigSetExpClk(hwAttrs->baseAddr,
                       freq.lo,
                       object->format,
                       object->mode,
                       object->bitRate,
                       object->dataSize);
}

/*
*  ======== initIO ========
*  This functions initializes the SPI IOs.
*
*  @pre    Function assumes that the SPI handle is pointing to a hardware
*          module which has already been opened.
*/
static bool initIO(SPI_Handle handle) {
    uint32_t                   i = 0;
    SPICC26X2DMA_Object        *object = handle->object;
    SPICC26X2DMA_HWAttrs const *hwAttrs = handle->hwAttrs;
    PIN_Config                 spiPinTable[5];

    /* Build local list of pins, allocate through PIN driver and map HW ports */
    if (object->mode == SPI_SLAVE) {
      spiPinTable[i++] = hwAttrs->mosiPin | PIN_INPUT_EN;
      spiPinTable[i++] = hwAttrs->misoPin | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW |
                         PIN_PUSHPULL | PIN_INPUT_DIS | PIN_DRVSTR_MED;
      spiPinTable[i++] = hwAttrs->clkPin  | PIN_INPUT_EN;
      spiPinTable[i++] = object->csnPin   | PIN_INPUT_EN | PIN_PULLUP;
    }
    else {
      spiPinTable[i++] = hwAttrs->mosiPin | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW |
                         PIN_PUSHPULL | PIN_INPUT_DIS | PIN_DRVSTR_MED;
      spiPinTable[i++] = hwAttrs->misoPin | PIN_INPUT_EN | PIN_PULLDOWN;

      /* Output low signal on SCLK until SPI module drives signal if clock
       * polarity is configured to '0'. Output high signal on SCLK until SPI
       * module drives signal if clock polarity is configured to '1' */
        if (object->format == SSI_FRF_MOTO_MODE_0 ||
            object->format == SSI_FRF_MOTO_MODE_1) {
            spiPinTable[i++] = hwAttrs->clkPin | PIN_GPIO_OUTPUT_EN |
                               PIN_GPIO_LOW | PIN_PUSHPULL | PIN_INPUT_DIS |
                               PIN_DRVSTR_MED;
        }
        else {
            spiPinTable[i++] = hwAttrs->clkPin | PIN_GPIO_OUTPUT_EN |
                               PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_INPUT_DIS |
                               PIN_DRVSTR_MED;
        }

        /* If CSN isn't SW controlled, drive it high until SPI module drives
        * signal to avoid glitches */
        if(object->csnPin != PIN_UNASSIGNED) {
            spiPinTable[i++] = object->csnPin | PIN_GPIO_OUTPUT_EN |
                               PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_INPUT_DIS |
                               PIN_DRVSTR_MED;
        }
    }
    spiPinTable[i++] = PIN_TERMINATE;

    /* Open and assign pins through pin driver */
    if (!(object->pinHandle = PIN_open(&(object->pinState), spiPinTable))) {
        return false;
    }

    /* Set IO muxing for the SPI pins */
    if (object->mode == SSI_MODE_SLAVE) {
        /* Configure IOs for slave mode */
        PINCC26XX_setMux(object->pinHandle,
                         hwAttrs->mosiPin,
                         (hwAttrs->baseAddr == SSI0_BASE ?
                         IOC_PORT_MCU_SSI0_RX : IOC_PORT_MCU_SSI1_RX));
        PINCC26XX_setMux(object->pinHandle,
                         hwAttrs->misoPin,
                         (hwAttrs->baseAddr == SSI0_BASE ?
                         IOC_PORT_MCU_SSI0_TX : IOC_PORT_MCU_SSI1_TX));
        PINCC26XX_setMux(object->pinHandle,
                         hwAttrs->clkPin,
                         (hwAttrs->baseAddr == SSI0_BASE ?
                         IOC_PORT_MCU_SSI0_CLK : IOC_PORT_MCU_SSI1_CLK));
        PINCC26XX_setMux(object->pinHandle,
                         object->csnPin,
                         (hwAttrs->baseAddr == SSI0_BASE ?
                         IOC_PORT_MCU_SSI0_FSS : IOC_PORT_MCU_SSI1_FSS));
    }
    else {
        /* Configure IOs for master mode */
        PINCC26XX_setMux(object->pinHandle,
                         hwAttrs->mosiPin,
                         (hwAttrs->baseAddr == SSI0_BASE ?
                         IOC_PORT_MCU_SSI0_TX : IOC_PORT_MCU_SSI1_TX));
        PINCC26XX_setMux(object->pinHandle,
                         hwAttrs->misoPin,
                         (hwAttrs->baseAddr == SSI0_BASE ?
                         IOC_PORT_MCU_SSI0_RX : IOC_PORT_MCU_SSI1_RX));
        PINCC26XX_setMux(object->pinHandle,
                         hwAttrs->clkPin,
                         (hwAttrs->baseAddr == SSI0_BASE ?
                         IOC_PORT_MCU_SSI0_CLK : IOC_PORT_MCU_SSI1_CLK));
        if(object->csnPin != PIN_UNASSIGNED) {
            PINCC26XX_setMux(object->pinHandle,
                             object->csnPin,
                             (hwAttrs->baseAddr == SSI0_BASE ?
                             IOC_PORT_MCU_SSI0_FSS : IOC_PORT_MCU_SSI1_FSS));
        }
    }

    return (true);
}

/*
 *  ======== primeTransfer ========
 *  Function must be executed with interrupts disabled.
 */
static inline void primeTransfer(SPICC26X2DMA_Object *object,
                                 SPICC26X2DMA_HWAttrs const *hwAttrs)
{
    if (object->priTransferSize != 0 && object->altTransferSize != 0) {
        /*
         * Both primary & alternate channels are configured for a transfer.
         * In this case no work is required; the Hwi will configure channels
         * as transfers continue & complete.
         */
    }
    else if (object->priTransferSize == 0 && object->altTransferSize == 0) {
        /*
         * Primary & alternate channels are disabled; no active transfer,
         * configure a new transfer.
         *
         * DMA based transfers use the DMA in ping-pong mode. If the transfer is
         * larger than what the primary channel can handle; alternate channel is
         * configured to continue where the primary channel left off. Channels
         * are continuously reconfigured until the transfer is completed.
         *
         * We disable the alternate channel initially.  This however causes an
         * undesired interrupt to be triggered; so we need to
         * disable/clear/re-enable the interrupt.
         */
        HwiP_disableInterrupt(hwAttrs->intNum);

        /* Set the primary DMA structure as active */
        UDMACC26XX_disableAttribute(object->udmaHandle,
                                    getDmaChannelNumber(hwAttrs->rxChannelBitMask),
                                    UDMA_ATTR_ALTSELECT);
        UDMACC26XX_disableAttribute(object->udmaHandle,
                                    getDmaChannelNumber(hwAttrs->txChannelBitMask),
                                    UDMA_ATTR_ALTSELECT);

        HwiP_clearInterrupt(hwAttrs->intNum);
        HwiP_enableInterrupt(hwAttrs->intNum);

        /* Configure RX & TX DMA transfers */
        configNextTransfer(object, hwAttrs);
        object->activeChannel = UDMA_PRI_SELECT;
        if (object->headPtr->count > MAX_DMA_TRANSFER_AMOUNT) {
            configNextTransfer(object, hwAttrs);
        }

        /* Enable DMA to generate interrupt on SPI peripheral */
        if (!object->manualStart) {
            SSIEnable(hwAttrs->baseAddr);
        }
    }
    else {
        /* One of the channels is active; configure the other channel */
        configNextTransfer(object, hwAttrs);
    }
}

/*
 *  ======== releaseConstraint ========
 */
static inline void releaseConstraint(uint32_t txBufAddr)
{
    /* Release need flash if buffer was in flash. */
    if (((txBufAddr & 0xF0000000) == 0x0) && (txBufAddr)) {
        Power_releaseConstraint(PowerCC26XX_DISALLOW_XOSC_HF_SWITCHING);
        Power_releaseConstraint(PowerCC26XX_NEED_FLASH_IN_IDLE);
    }

    /* Release standby constraint since operation is done. */
    Power_releaseConstraint(PowerCC26XX_DISALLOW_STANDBY);
}

/*
 *  ======== setConstraint ========
 */
static inline void setConstraint(uint32_t txBufAddr)
{
    /*
     * Ensure flash is available if TX buffer is in flash.
     * Flash starts with 0x0..
     */
    if (((txBufAddr & 0xF0000000) == 0x0) && (txBufAddr)) {
        Power_setConstraint(PowerCC26XX_NEED_FLASH_IN_IDLE);
        Power_setConstraint(PowerCC26XX_DISALLOW_XOSC_HF_SWITCHING);
    }

    /* Set constraints to guarantee operation */
    Power_setConstraint(PowerCC26XX_DISALLOW_STANDBY);
}

/*
 *  ======== spiPollingTransfer ========
 */
static inline void spiPollingTransfer(SPICC26X2DMA_Object *object,
                                      SPICC26X2DMA_HWAttrs const *hwAttrs,
                                      SPI_Transaction *transaction)
{
    uint8_t   increment;
    uint32_t  dummyBuffer;
    size_t    transferCount;
    void     *rxBuf;
    void     *txBuf;

    if (transaction->rxBuf) {
        rxBuf = transaction->rxBuf;
    }
    else {
        rxBuf = &(object->rxScratchBuf);
    }

    if (transaction->txBuf) {
        txBuf = transaction->txBuf;
    }
    else {
        txBuf = (void *) &(object->txScratchBuf);
    }

    increment = (object->dataSize < 9) ? sizeof(uint8_t) : sizeof(uint16_t);
    transferCount = transaction->count;

    SSIEnable(hwAttrs->baseAddr);

    while (transferCount--) {
        if (object->dataSize < 9) {
            SSIDataPut(hwAttrs->baseAddr, *((uint8_t *) txBuf));
            SSIDataGet(hwAttrs->baseAddr, &dummyBuffer);
            *((uint8_t *) rxBuf) = (uint8_t) dummyBuffer;
        }
        else {
            SSIDataPut(hwAttrs->baseAddr, *((uint16_t *) txBuf));
            SSIDataGet(hwAttrs->baseAddr, &dummyBuffer);
            *((uint16_t *) rxBuf) = (uint16_t) dummyBuffer;
        }

        /* Only increment source & destination if buffers were provided */
        if (transaction->rxBuf) {
            rxBuf = (void *) (((uint32_t) rxBuf) + increment);
        }
        if (transaction->txBuf) {
            txBuf = (void *) (((uint32_t) txBuf) + increment);
        }
    }

    while (spiBusy(object, hwAttrs)) {}

    /*
     * For this driver implementation the peripheral is kept active until
     * either a FIFO-overrun occurs or SPI_transferCancel() is executed.
     *
     * SSIDisable(hwAttrs->baseAddr);
     */
}

/*
 *  ======== spiPostNotify ========
 */
static int spiPostNotify(unsigned int eventType, uintptr_t eventArg,
    uintptr_t clientArg)
{
    initHw((SPI_Handle) clientArg);

    return (Power_NOTIFYDONE);
}

/*
 *  ======== spiBusy ========
 *  HW is busy when in master mode and BSY bit is set, or when in slave mode
 *  and TFE bit is not set.
 */
static inline bool spiBusy(SPICC26X2DMA_Object *object,
                           SPICC26X2DMA_HWAttrs const *hwAttrs)
{
    bool registerBit = (bool)(HWREG(hwAttrs->baseAddr + SSI_O_SR) & (object->busyBit));
    if (object->busyBit == SSI_SR_BSY){
        return(registerBit);
    }
    else
    {
        return(!registerBit);
    }
}
