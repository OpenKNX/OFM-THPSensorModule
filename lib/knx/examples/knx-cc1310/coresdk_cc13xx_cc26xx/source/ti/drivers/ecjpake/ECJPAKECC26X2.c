/*
 * Copyright (c) 2017-2018, Texas Instruments Incorporated
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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <ti/drivers/dpl/DebugP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SwiP.h>
#include <ti/drivers/dpl/SemaphoreP.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>
#include <ti/drivers/ECJPAKE.h>
#include <ti/drivers/ecjpake/ECJPAKECC26X2.h>
#include <ti/drivers/cryptoutils/sharedresources/PKAResourceCC26XX.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_ints.h)
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(inc/hw_pka.h)
#include DeviceFamily_constructPath(inc/hw_pka_ram.h)
#include DeviceFamily_constructPath(driverlib/pka.h)
#include DeviceFamily_constructPath(driverlib/trng.h)

#define ECJPAKECC26X2_SCRATCH_BUFFER_SIZE 256

/* Since we only support <= 384-bit curves, we can allocate two temporary buffers
 * at the midway address in PKA_RAM. This leaves us with 1024 bytes of RAM
 * to work with. That should be sufficient for our needs. Multiplications will
 * take up to 3x the curve param length of 384 bits or 48 bytes. ECC addition
 * requires 8x the curve param length in RAM. 1024 bytes would even suffice
 * for 384-bit curves.
 */
#define ECJPAKECC26X2_SCRATCH_BUFFER ((uint8_t *)(PKA_RAM_BASE + PKA_RAM_TOT_BYTE_SIZE / 2))
#define ECJPAKECC26X2_SCRATCH_BUFFER_2 ((uint8_t *)(PKA_RAM_BASE + PKA_RAM_TOT_BYTE_SIZE / 2 + ECJPAKECC26X2_SCRATCH_BUFFER_SIZE))

#define MAX(x,y)   (((x) > (y)) ?  (x) : (y))

/* Forward declarations */
static void ECJPAKECC26X2_hwiFxn (uintptr_t arg0);
static void ECJPAKECC26X2_internalCallbackFxn (ECJPAKE_Handle handle,
                                               int_fast16_t returnStatus,
                                               ECJPAKE_Operation operation,
                                               ECJPAKE_OperationType operationType);
static int_fast16_t ECJPAKECC26X2_waitForAccess(ECJPAKE_Handle handle);
static int_fast16_t ECJPAKECC26X2_waitForResult(ECJPAKE_Handle handle);
static int_fast16_t ECJPAKECC26X2_runFSM(ECJPAKE_Handle handle);
static int_fast16_t ECJPAKECC26X2_convertReturnValue(uint32_t pkaResult);

/* Extern globals */
extern const ECJPAKE_Config ECJPAKE_config[];
extern const uint_least8_t ECJPAKE_count;
extern const ECJPAKE_Params ECJPAKE_defaultParams;

/* Static globals */
static bool isInitialized = false;

static uint32_t scratchBufferSize = ECJPAKECC26X2_SCRATCH_BUFFER_SIZE;


/*
 *  ======== ECJPAKECC26X2_internalCallbackFxn ========
 */
static void ECJPAKECC26X2_internalCallbackFxn (ECJPAKE_Handle handle,
                                               int_fast16_t returnStatus,
                                               ECJPAKE_Operation operation,
                                               ECJPAKE_OperationType operationType) {
    ECJPAKECC26X2_Object *object = handle->object;

    /* This function is only ever registered when in ECJPAKE_RETURN_BEHAVIOR_BLOCKING
     * or ECJPAKE_RETURN_BEHAVIOR_POLLING.
     */
    if (object->returnBehavior == ECJPAKE_RETURN_BEHAVIOR_BLOCKING) {
        SemaphoreP_post(&PKAResourceCC26XX_operationSemaphore);
    }
    else {
        PKAResourceCC26XX_pollingFlag = 1;
    }
}

/*
 *  ======== ECJPAKECC26X2_hwiFxn ========
 */
static void ECJPAKECC26X2_hwiFxn (uintptr_t arg0) {
    ECJPAKECC26X2_Object *object = ((ECJPAKE_Handle)arg0)->object;
    uint32_t key;

    /* Disable interrupt again */
    IntDisable(INT_PKA_IRQ);

    do {
        object->operationStatus = ECJPAKECC26X2_runFSM((ECJPAKE_Handle)arg0);
        object->fsmState++;
    } while (object->operationStatus == ECJPAKECC26X2_STATUS_FSM_RUN_FSM);

    /* We need a critical section here in case the operation is canceled
     * asynchronously.
     */
    key = HwiP_disable();

    if(object->operationCanceled) {
        /* Set function register to 0. This should stop the current operation */
        HWREG(PKA_BASE + PKA_O_FUNCTION) = 0;

        object->operationStatus = ECJPAKE_STATUS_CANCELED;
    }

    switch (object->operationStatus) {
        case ECJPAKECC26X2_STATUS_FSM_RUN_PKA_OP:

            HwiP_restore(key);

            /* Do nothing. The PKA hardware
             * will execute in the background and post
             * this SWI when it is done.
             */
            break;
        case ECJPAKE_STATUS_SUCCESS:
            /* Intentional fall through */
        case ECJPAKE_STATUS_ERROR:
            /* Intentional fall through */
        case ECJPAKE_STATUS_CANCELED:
            /* Intentional fall through */
        default:

            /* Mark this operation as complete */
            object->operationInProgress = false;

            /* Clear any pending interrupt in case a transaction kicked off
             * above already finished
             */
            IntDisable(INT_PKA_IRQ);
            IntPendClear(INT_PKA_IRQ);

            /* We can end the critical section since the operation may no
             * longer be canceled
             */
            HwiP_restore(key);

            /* Make sure there is no keying material remaining in PKA RAM */
            PKAClearPkaRam();

            /*  Grant access for other threads to use the crypto module.
             *  The semaphore must be posted before the callbackFxn to allow the chaining
             *  of operations.
             */
            SemaphoreP_post(&PKAResourceCC26XX_accessSemaphore);

            Power_releaseConstraint(PowerCC26XX_DISALLOW_STANDBY);

            object->callbackFxn((ECJPAKE_Handle)arg0,
                                object->operationStatus,
                                object->operation,
                                object->operationType);
    }
}

/*
 *  ======== ECJPAKECC26X2_runSignFSM ========
 */
static int_fast16_t ECJPAKECC26X2_runFSM(ECJPAKE_Handle handle) {
    static uint32_t resultAddress;
    ECJPAKECC26X2_Object *object = handle->object;
    uint32_t pkaResult;
    uint32_t i;

    switch (object->fsmState) {
        /* ====================================================================
         * ROUND ONE KEY GENERATION FSM STATE IMPLEMENTATIONS
         * ====================================================================
         */
        case ECJPAKECC26X2_FSM_ROUND_ONE_VALIDATE_MYPRIVATEKEY1:

            if (PKAArrayAllZeros(object->operation.generateRoundOneKeys->myPrivateKey1->u.plaintext.keyMaterial,
                                 object->operation.generateRoundOneKeys->curve->length)) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_KEY;
            }

            PKABigNumCmpStart(object->operation.generateRoundOneKeys->myPrivateKey1->u.plaintext.keyMaterial,
                              object->operation.generateRoundOneKeys->curve->order,
                              object->operation.generateRoundOneKeys->curve->length);

            while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

            pkaResult = PKABigNumCmpGetResult();

            if (pkaResult != PKA_STATUS_A_LESS_THAN_B) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_KEY;
            }
            else {
                return ECJPAKECC26X2_STATUS_FSM_RUN_FSM;
            }

        case ECJPAKECC26X2_FSM_ROUND_ONE_VALIDATE_MYPRIVATEKEY2:

            if (PKAArrayAllZeros(object->operation.generateRoundOneKeys->myPrivateKey2->u.plaintext.keyMaterial,
                                 object->operation.generateRoundOneKeys->curve->length)) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_KEY;
            }

            PKABigNumCmpStart(object->operation.generateRoundOneKeys->myPrivateKey2->u.plaintext.keyMaterial,
                              object->operation.generateRoundOneKeys->curve->order,
                              object->operation.generateRoundOneKeys->curve->length);

            while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

            pkaResult = PKABigNumCmpGetResult();

            if (pkaResult != PKA_STATUS_A_LESS_THAN_B) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_KEY;
            }
            else {
                return ECJPAKECC26X2_STATUS_FSM_RUN_FSM;
            }

        case ECJPAKECC26X2_FSM_ROUND_ONE_VALIDATE_MYPRIVATEV1:

            if (PKAArrayAllZeros(object->operation.generateRoundOneKeys->myPrivateV1->u.plaintext.keyMaterial,
                                 object->operation.generateRoundOneKeys->curve->length)) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_V;
            }

            PKABigNumCmpStart(object->operation.generateRoundOneKeys->myPrivateV1->u.plaintext.keyMaterial,
                              object->operation.generateRoundOneKeys->curve->order,
                              object->operation.generateRoundOneKeys->curve->length);

            while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

            pkaResult = PKABigNumCmpGetResult();

            if (pkaResult != PKA_STATUS_A_LESS_THAN_B) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_V;
            }
            else {
                return ECJPAKECC26X2_STATUS_FSM_RUN_FSM;
            }


        case ECJPAKECC26X2_FSM_ROUND_ONE_VALIDATE_MYPRIVATEV2:

            if (PKAArrayAllZeros(object->operation.generateRoundOneKeys->myPrivateV2->u.plaintext.keyMaterial,
                                 object->operation.generateRoundOneKeys->curve->length)) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_V;
            }

            PKABigNumCmpStart(object->operation.generateRoundOneKeys->myPrivateV2->u.plaintext.keyMaterial,
                              object->operation.generateRoundOneKeys->curve->order,
                              object->operation.generateRoundOneKeys->curve->length);

            while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

            pkaResult = PKABigNumCmpGetResult();

            if (pkaResult != PKA_STATUS_A_LESS_THAN_B) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_V;
            }
            else {
                return ECJPAKECC26X2_STATUS_FSM_RUN_FSM;
            }

        case ECJPAKECC26X2_FSM_ROUND_ONE_GENERATE_MYPUBLICKEY1:

            PKAEccMultiplyStart(object->operation.generateRoundOneKeys->myPrivateKey1->u.plaintext.keyMaterial,
                                object->operation.generateRoundOneKeys->curve->generatorX,
                                object->operation.generateRoundOneKeys->curve->generatorY,
                                object->operation.generateRoundOneKeys->curve->prime,
                                object->operation.generateRoundOneKeys->curve->a,
                                object->operation.generateRoundOneKeys->curve->b,
                                object->operation.generateRoundOneKeys->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_ONE_GENERATE_MYPUBLICKEY1_RESULT:

            pkaResult = PKAEccMultiplyGetResult(object->operation.generateRoundOneKeys->myPublicKey1->u.plaintext.keyMaterial,
                                                object->operation.generateRoundOneKeys->myPublicKey1->u.plaintext.keyMaterial
                                                    + object->operation.generateRoundOneKeys->curve->length,
                                                resultAddress,
                                                object->operation.generateRoundOneKeys->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_ROUND_ONE_GENERATE_MYPUBLICKEY2:

            PKAEccMultiplyStart(object->operation.generateRoundOneKeys->myPrivateKey2->u.plaintext.keyMaterial,
                                object->operation.generateRoundOneKeys->curve->generatorX,
                                object->operation.generateRoundOneKeys->curve->generatorY,
                                object->operation.generateRoundOneKeys->curve->prime,
                                object->operation.generateRoundOneKeys->curve->a,
                                object->operation.generateRoundOneKeys->curve->b,
                                object->operation.generateRoundOneKeys->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_ONE_GENERATE_MYPUBLICKEY2_RESULT:

            pkaResult = PKAEccMultiplyGetResult(object->operation.generateRoundOneKeys->myPublicKey2->u.plaintext.keyMaterial,
                                                object->operation.generateRoundOneKeys->myPublicKey2->u.plaintext.keyMaterial
                                                    + object->operation.generateRoundOneKeys->curve->length,
                                                resultAddress,
                                                object->operation.generateRoundOneKeys->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_ROUND_ONE_GENERATE_MYPUBLICV1:

            PKAEccMultiplyStart(object->operation.generateRoundOneKeys->myPrivateV1->u.plaintext.keyMaterial,
                                object->operation.generateRoundOneKeys->curve->generatorX,
                                object->operation.generateRoundOneKeys->curve->generatorY,
                                object->operation.generateRoundOneKeys->curve->prime,
                                object->operation.generateRoundOneKeys->curve->a,
                                object->operation.generateRoundOneKeys->curve->b,
                                object->operation.generateRoundOneKeys->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_ONE_GENERATE_MYPUBLICV1_RESULT:

            pkaResult = PKAEccMultiplyGetResult(object->operation.generateRoundOneKeys->myPublicV1->u.plaintext.keyMaterial,
                                                object->operation.generateRoundOneKeys->myPublicV1->u.plaintext.keyMaterial
                                                    + object->operation.generateRoundOneKeys->curve->length,
                                                resultAddress,
                                                object->operation.generateRoundOneKeys->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_ROUND_ONE_GENERATE_MYPUBLICV2:

            PKAEccMultiplyStart(object->operation.generateRoundOneKeys->myPrivateV2->u.plaintext.keyMaterial,
                                object->operation.generateRoundOneKeys->curve->generatorX,
                                object->operation.generateRoundOneKeys->curve->generatorY,
                                object->operation.generateRoundOneKeys->curve->prime,
                                object->operation.generateRoundOneKeys->curve->a,
                                object->operation.generateRoundOneKeys->curve->b,
                                object->operation.generateRoundOneKeys->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_ONE_GENERATE_MYPUBLICV2_RESULT:

            pkaResult = PKAEccMultiplyGetResult(object->operation.generateRoundOneKeys->myPublicV2->u.plaintext.keyMaterial,
                                                object->operation.generateRoundOneKeys->myPublicV2->u.plaintext.keyMaterial
                                                    + object->operation.generateRoundOneKeys->curve->length,
                                                resultAddress,
                                                object->operation.generateRoundOneKeys->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        /* ====================================================================
         * GENERATE ZKP FSM STATE IMPLEMENTATIONS
         * ====================================================================
         */
        case ECJPAKECC26X2_FSM_GENERATE_ZKP_PRIVATEKEY_X_HASH:

            PKABigNumMultiplyStart(object->operation.generateZKP->myPrivateKey->u.plaintext.keyMaterial,
                                   object->operation.generateZKP->curve->length,
                                   object->operation.generateZKP->hash,
                                   object->operation.generateZKP->curve->length,
                                   &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_GENERATE_ZKP_PRIVATEKEY_X_HASH_RESULT:

            pkaResult = PKABigNumMultGetResult(ECJPAKECC26X2_SCRATCH_BUFFER,
                                               &scratchBufferSize,
                                               resultAddress);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_GENERATE_ZKP_PRIVATEKEY_X_HASH_MOD_N:

            /* Zero out the buffer up to the curve length. The PKA hardware cannot
             * handle modulus operations where the byte-length of the dividend is smaller
             * than the divisor.
             */
            if (object->operation.generateZKP->curve->length > scratchBufferSize) {
                PKAZeroOutArray(ECJPAKECC26X2_SCRATCH_BUFFER + scratchBufferSize,
                                object->operation.generateZKP->curve->length - scratchBufferSize);
            }

            /* The scratch buffer content has a real length of scratchBufferSize but is
             * zero-extended until curve->length. We cannot start a modulo operation on
             * the PKA where dividend length < divisor length. Hence, the buffer size
             * always needs to be >= curve->length.
             */
            PKABigNumModStart(ECJPAKECC26X2_SCRATCH_BUFFER,
                              MAX(scratchBufferSize, object->operation.generateZKP->curve->length),
                              object->operation.generateZKP->curve->order,
                              object->operation.generateZKP->curve->length,
                              &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_GENERATE_ZKP_PRIVATEKEY_X_HASH_MOD_N_RESULT:

            pkaResult = PKABigNumModGetResult(object->operation.generateZKP->r,
                                              object->operation.generateZKP->curve->length,
                                              resultAddress);


            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_GENERATE_ZKP_ADD_V_TO_N:

            PKABigNumAddStart(object->operation.generateZKP->myPrivateV->u.plaintext.keyMaterial,
                              object->operation.generateZKP->curve->length,
                              object->operation.generateZKP->curve->order,
                              object->operation.generateZKP->curve->length,
                              &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_GENERATE_ZKP_ADD_V_TO_N_RESULT:

            scratchBufferSize = ECJPAKECC26X2_SCRATCH_BUFFER_SIZE;

            pkaResult = PKABigNumAddGetResult(ECJPAKECC26X2_SCRATCH_BUFFER,
                                              &scratchBufferSize,
                                              resultAddress);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_GENERATE_ZKP_SUBTRACT_RESULTS:

            PKABigNumSubStart(ECJPAKECC26X2_SCRATCH_BUFFER,
                              scratchBufferSize,
                              object->operation.generateZKP->r,
                              object->operation.generateZKP->curve->length,
                              &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_GENERATE_ZKP_SUBTRACT_RESULTS_RESULT:

            scratchBufferSize = ECJPAKECC26X2_SCRATCH_BUFFER_SIZE;

            pkaResult = PKABigNumSubGetResult(ECJPAKECC26X2_SCRATCH_BUFFER,
                                              &scratchBufferSize,
                                              resultAddress);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_GENERATE_ZKP_SUBTRACT_RESULT_MOD_N:

            /* Zero out the buffer up to the curve length. The PKA hardware cannot
             * handle modulus operations where the byte-length of the dividend is smaller
             * than the divisor.
             */
            if (object->operation.generateZKP->curve->length > scratchBufferSize) {
                PKAZeroOutArray(ECJPAKECC26X2_SCRATCH_BUFFER + scratchBufferSize,
                                object->operation.generateZKP->curve->length - scratchBufferSize);
            }

            /* The scratch buffer content has a real length of scratchBufferSize but is
             * zero-extended until curve->length. We cannot start a modulo operation on
             * the PKA where dividend length < divisor length. Hence, the buffer size
             * always needs to be >= curve->length.
             */
            PKABigNumModStart(ECJPAKECC26X2_SCRATCH_BUFFER,
                              MAX(scratchBufferSize, object->operation.generateZKP->curve->length),
                              object->operation.generateZKP->curve->order,
                              object->operation.generateZKP->curve->length,
                              &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_GENERATE_ZKP_SUBTRACT_RESULT_MOD_N_RESULT:

            pkaResult = PKABigNumModGetResult(object->operation.generateZKP->r,
                                              object->operation.generateZKP->curve->length,
                                              resultAddress);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        /* ====================================================================
         * VERIFY ZKP FSM STATE IMPLEMENTATIONS
         * ====================================================================
         */
        case ECJPAKECC26X2_FSM_VERIFY_ZKP_VALIDATE_PUBLIC_KEY:

            pkaResult = PKAEccVerifyPublicKeyWeierstrassStart(object->operation.verifyZKP->theirPublicKey->u.plaintext.keyMaterial,
                                                              object->operation.verifyZKP->theirPublicKey->u.plaintext.keyMaterial
                                                                + object->operation.verifyZKP->curve->length,
                                                              object->operation.verifyZKP->curve->prime,
                                                              object->operation.verifyZKP->curve->a,
                                                              object->operation.verifyZKP->curve->b,
                                                              object->operation.verifyZKP->curve->order,
                                                              object->operation.verifyZKP->curve->length);

            // Break out early since no PKA operation was started by the verify fxn
            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_VERIFY_ZKP_MULT_G_BY_R:

            PKAEccMultiplyStart(object->operation.verifyZKP->r,
                                object->operation.verifyZKP->theirGenerator->u.plaintext.keyMaterial,
                                object->operation.verifyZKP->theirGenerator->u.plaintext.keyMaterial
                                    + object->operation.verifyZKP->curve->length,
                                object->operation.verifyZKP->curve->prime,
                                object->operation.verifyZKP->curve->a,
                                object->operation.verifyZKP->curve->b,
                                object->operation.verifyZKP->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_VERIFY_ZKP_MULT_G_BY_R_RESULT:

            pkaResult = PKAEccMultiplyGetResult(ECJPAKECC26X2_SCRATCH_BUFFER,
                                                ECJPAKECC26X2_SCRATCH_BUFFER + object->operation.verifyZKP->curve->length,
                                                resultAddress,
                                                object->operation.verifyZKP->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_VERIFY_ZKP_HASH_MOD_N:

            PKABigNumModStart(object->operation.verifyZKP->hash,
                              object->operation.verifyZKP->curve->length,
                              object->operation.verifyZKP->curve->order,
                              object->operation.verifyZKP->curve->length,
                              &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_VERIFY_ZKP_HASH_MOD_N_RESULT:

            pkaResult = PKABigNumModGetResult(ECJPAKECC26X2_SCRATCH_BUFFER_2,
                                              object->operation.verifyZKP->curve->length,
                                              resultAddress);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_VERIFY_ZKP_MULT_X_BY_HASH:

            PKAEccMultiplyStart(ECJPAKECC26X2_SCRATCH_BUFFER_2,
                                object->operation.verifyZKP->theirPublicKey->u.plaintext.keyMaterial,
                                object->operation.verifyZKP->theirPublicKey->u.plaintext.keyMaterial
                                    + object->operation.verifyZKP->curve->length,
                                object->operation.verifyZKP->curve->prime,
                                object->operation.verifyZKP->curve->a,
                                object->operation.verifyZKP->curve->b,
                                object->operation.verifyZKP->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_VERIFY_ZKP_MULT_X_BY_HASH_RESULT:

            pkaResult = PKAEccMultiplyGetResult(ECJPAKECC26X2_SCRATCH_BUFFER_2,
                                                ECJPAKECC26X2_SCRATCH_BUFFER_2 + object->operation.verifyZKP->curve->length,
                                                resultAddress,
                                                object->operation.verifyZKP->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_VERIFY_ZKP_ADD_RESULTS:

            PKAEccAddStart(ECJPAKECC26X2_SCRATCH_BUFFER,
                           ECJPAKECC26X2_SCRATCH_BUFFER + object->operation.verifyZKP->curve->length,
                           ECJPAKECC26X2_SCRATCH_BUFFER_2,
                           ECJPAKECC26X2_SCRATCH_BUFFER_2 + object->operation.verifyZKP->curve->length,
                           object->operation.verifyZKP->curve->prime,
                           object->operation.verifyZKP->curve->a,
                           object->operation.verifyZKP->curve->length,
                           &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_VERIFY_ZKP_ADD_RESULTS_RESULT:

            pkaResult = PKAEccAddGetResult(ECJPAKECC26X2_SCRATCH_BUFFER,
                                           ECJPAKECC26X2_SCRATCH_BUFFER + object->operation.verifyZKP->curve->length,
                                           resultAddress,
                                           object->operation.verifyZKP->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_VERIFY_ZKP_COMPARE_AGAINST_V:

            /* Compare 2 * curve length since we wish to check the entire point not just the X coordinate */
            for (i = 0; i < (2 * object->operation.verifyZKP->curve->length) / sizeof(uint32_t); i++) {
                if (((uint32_t *)object->operation.verifyZKP->theirPublicV->u.plaintext.keyMaterial)[i] != ((uint32_t *)ECJPAKECC26X2_SCRATCH_BUFFER)[i]) {
                    return ECJPAKE_STATUS_ERROR;
                }
            }

            return ECJPAKE_STATUS_SUCCESS;

        /* ====================================================================
         * ROUND TWO KEY GENERATION FSM STATE IMPLEMENTATIONS
         * ====================================================================
         */
        case ECJPAKECC26X2_FSM_ROUND_TWO_MULT_MYPRIVATEKEY2_BY_PRESHAREDSECRET:

            PKABigNumMultiplyStart(object->operation.generateRoundTwoKeys->myPrivateKey2->u.plaintext.keyMaterial,
                                   object->operation.generateRoundTwoKeys->curve->length,
                                   object->operation.generateRoundTwoKeys->preSharedSecret->u.plaintext.keyMaterial,
                                   object->operation.generateRoundTwoKeys->preSharedSecret->u.plaintext.keyLength,
                                   &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_TWO_MULT_MYPRIVATEKEY2_BY_PRESHAREDSECRET_RESULT:

            pkaResult = PKABigNumMultGetResult(ECJPAKECC26X2_SCRATCH_BUFFER,
                                               &scratchBufferSize,
                                               resultAddress);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_ROUND_TWO_MYCOMBINEDPRIVATEKEY_MOD_N:

            /* Zero out the buffer up to the curve length. The PKA hardware cannot
             * handle modulus operations where the byte-length of the dividend is smaller
             * than the divisor.
             */
            if (object->operation.generateRoundTwoKeys->curve->length > scratchBufferSize) {
                PKAZeroOutArray(ECJPAKECC26X2_SCRATCH_BUFFER + scratchBufferSize,
                                object->operation.generateRoundTwoKeys->curve->length - scratchBufferSize);
            }

            /* The scratch buffer content has a real length of scratchBufferSize but is
             * zero-extended until curve->length. We cannot start a modulo operation on
             * the PKA where dividend length < divisor length. Hence, the buffer size
             * always needs to be >= curve->length.
             */
            PKABigNumModStart(ECJPAKECC26X2_SCRATCH_BUFFER,
                              MAX(scratchBufferSize, object->operation.generateRoundTwoKeys->curve->length),
                              object->operation.generateRoundTwoKeys->curve->order,
                              object->operation.generateRoundTwoKeys->curve->length,
                              &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_TWO_MYCOMBINEDPRIVATEKEY_MOD_N_RESULT:

            pkaResult = PKABigNumModGetResult(object->operation.generateRoundTwoKeys->myCombinedPrivateKey->u.plaintext.keyMaterial,
                                              object->operation.generateRoundTwoKeys->curve->length,
                                              resultAddress);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);


        case ECJPAKECC26X2_FSM_ROUND_TWO_ADD_MYPUBLICKEY1_TO_THEIRPUBLICKEY1:

            PKAEccAddStart(object->operation.generateRoundTwoKeys->myPublicKey1->u.plaintext.keyMaterial,
                           object->operation.generateRoundTwoKeys->myPublicKey1->u.plaintext.keyMaterial
                            + object->operation.generateRoundTwoKeys->curve->length,
                           object->operation.generateRoundTwoKeys->theirPublicKey1->u.plaintext.keyMaterial,
                           object->operation.generateRoundTwoKeys->theirPublicKey1->u.plaintext.keyMaterial
                            + object->operation.generateRoundTwoKeys->curve->length,
                           object->operation.generateRoundTwoKeys->curve->prime,
                           object->operation.generateRoundTwoKeys->curve->a,
                           object->operation.generateRoundTwoKeys->curve->length,
                           &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_TWO_ADD_MYPUBLICKEY1_TO_THEIRPUBLICKEY1_RESULT:

            pkaResult = PKAEccAddGetResult(ECJPAKECC26X2_SCRATCH_BUFFER,
                                           ECJPAKECC26X2_SCRATCH_BUFFER + object->operation.generateRoundTwoKeys->curve->length,
                                           resultAddress,
                                           object->operation.generateRoundTwoKeys->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_ROUND_TWO_ADD_THEIRPUBLICKEY2:

            PKAEccAddStart(ECJPAKECC26X2_SCRATCH_BUFFER,
                           ECJPAKECC26X2_SCRATCH_BUFFER + object->operation.generateRoundTwoKeys->curve->length,
                           object->operation.generateRoundTwoKeys->theirPublicKey2->u.plaintext.keyMaterial,
                           object->operation.generateRoundTwoKeys->theirPublicKey2->u.plaintext.keyMaterial
                           + object->operation.generateRoundTwoKeys->curve->length,
                           object->operation.generateRoundTwoKeys->curve->prime,
                           object->operation.generateRoundTwoKeys->curve->a,
                           object->operation.generateRoundTwoKeys->curve->length,
                           &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_TWO_ADD_THEIRPUBLICKEY2_RESULT:

            pkaResult = PKAEccAddGetResult(object->operation.generateRoundTwoKeys->myNewGenerator->u.plaintext.keyMaterial,
                                           object->operation.generateRoundTwoKeys->myNewGenerator->u.plaintext.keyMaterial
                                            + object->operation.generateRoundTwoKeys->curve->length,
                                           resultAddress,
                                           object->operation.generateRoundTwoKeys->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_ROUND_TWO_ADD_MYPUBLICKEY2:

            PKAEccAddStart(ECJPAKECC26X2_SCRATCH_BUFFER,
                           ECJPAKECC26X2_SCRATCH_BUFFER + object->operation.generateRoundTwoKeys->curve->length,
                           object->operation.generateRoundTwoKeys->myPublicKey2->u.plaintext.keyMaterial,
                           object->operation.generateRoundTwoKeys->myPublicKey2->u.plaintext.keyMaterial
                            + object->operation.generateRoundTwoKeys->curve->length,
                           object->operation.generateRoundTwoKeys->curve->prime,
                           object->operation.generateRoundTwoKeys->curve->a,
                           object->operation.generateRoundTwoKeys->curve->length,
                           &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_TWO_ADD_MYPUBLICKEY2_RESULT:

            pkaResult = PKAEccAddGetResult(object->operation.generateRoundTwoKeys->theirNewGenerator->u.plaintext.keyMaterial,
                                           object->operation.generateRoundTwoKeys->theirNewGenerator->u.plaintext.keyMaterial
                                            + object->operation.generateRoundTwoKeys->curve->length,
                                           resultAddress,
                                           object->operation.generateRoundTwoKeys->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);


        case ECJPAKECC26X2_FSM_ROUND_TWO_MULT_MYCOMBINEDPRIVATEKEY_BY_MYNEWGENERATOR:

            PKAEccMultiplyStart(object->operation.generateRoundTwoKeys->myCombinedPrivateKey->u.plaintext.keyMaterial,
                                object->operation.generateRoundTwoKeys->myNewGenerator->u.plaintext.keyMaterial,
                                object->operation.generateRoundTwoKeys->myNewGenerator->u.plaintext.keyMaterial
                                    + object->operation.generateRoundTwoKeys->curve->length,
                                object->operation.generateRoundTwoKeys->curve->prime,
                                object->operation.generateRoundTwoKeys->curve->a,
                                object->operation.generateRoundTwoKeys->curve->b,
                                object->operation.generateRoundTwoKeys->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_TWO_MULT_MYCOMBINEDPRIVATEKEY_BY_MYNEWGENERATOR_RESULT:

            pkaResult = PKAEccMultiplyGetResult(object->operation.generateRoundTwoKeys->myCombinedPublicKey->u.plaintext.keyMaterial,
                                                object->operation.generateRoundTwoKeys->myCombinedPublicKey->u.plaintext.keyMaterial
                                                    + object->operation.generateRoundTwoKeys->curve->length,
                                                resultAddress,
                                                object->operation.generateRoundTwoKeys->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_ROUND_TWO_VALIDATE_MYPRIVATEV:

            if (PKAArrayAllZeros(object->operation.generateRoundTwoKeys->myPrivateV->u.plaintext.keyMaterial,
                                 object->operation.generateRoundTwoKeys->curve->length)) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_KEY;
            }

            PKABigNumCmpStart(object->operation.generateRoundTwoKeys->myPrivateV->u.plaintext.keyMaterial,
                              object->operation.generateRoundTwoKeys->curve->order,
                              object->operation.generateRoundTwoKeys->curve->length);

            while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

            pkaResult = PKABigNumCmpGetResult();

            if (pkaResult != PKA_STATUS_A_LESS_THAN_B) {
                return ECJPAKE_STATUS_INVALID_PRIVATE_KEY;
            }
            else {
                return ECJPAKECC26X2_STATUS_FSM_RUN_FSM;
            }

        case ECJPAKECC26X2_FSM_ROUND_TWO_GENERATE_MYPUBLICV:

            PKAEccMultiplyStart(object->operation.generateRoundTwoKeys->myPrivateV->u.plaintext.keyMaterial,
                                object->operation.generateRoundTwoKeys->myNewGenerator->u.plaintext.keyMaterial,
                                object->operation.generateRoundTwoKeys->myNewGenerator->u.plaintext.keyMaterial
                                    + object->operation.generateRoundTwoKeys->curve->length,
                                object->operation.generateRoundTwoKeys->curve->prime,
                                object->operation.generateRoundTwoKeys->curve->a,
                                object->operation.generateRoundTwoKeys->curve->b,
                                object->operation.generateRoundTwoKeys->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_ROUND_TWO_GENERATE_MYPUBLICV_RESULT:

            pkaResult = PKAEccMultiplyGetResult(object->operation.generateRoundTwoKeys->myPublicV->u.plaintext.keyMaterial,
                                                object->operation.generateRoundTwoKeys->myPublicV->u.plaintext.keyMaterial
                                                    + object->operation.generateRoundTwoKeys->curve->length,
                                                resultAddress,
                                                object->operation.generateRoundTwoKeys->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);


        /* ====================================================================
         * GENERATE SHARED SECRET FSM STATE IMPLEMENTATIONS
         * ====================================================================
         */
        case ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_MULT_THEIRPUBLICKEY2_BY_MYCOMBINEDPRIVATEKEY:

            PKAEccMultiplyStart(object->operation.computeSharedSecret->myCombinedPrivateKey->u.plaintext.keyMaterial,
                                object->operation.computeSharedSecret->theirPublicKey2->u.plaintext.keyMaterial,
                                object->operation.computeSharedSecret->theirPublicKey2->u.plaintext.keyMaterial
                                    + object->operation.computeSharedSecret->curve->length,
                                object->operation.computeSharedSecret->curve->prime,
                                object->operation.computeSharedSecret->curve->a,
                                object->operation.computeSharedSecret->curve->b,
                                object->operation.computeSharedSecret->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_MULT_THEIRPUBLICKEY2_BY_MYCOMBINEDPRIVATEKEY_RESULT:

            pkaResult = PKAEccMultiplyGetResult(object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial,
                                                object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial
                                                    + object->operation.computeSharedSecret->curve->length,
                                                resultAddress,
                                                object->operation.computeSharedSecret->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

         case ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_SUB_YCOORDINATE_FROM_PRIME:

            PKABigNumSubStart(object->operation.computeSharedSecret->curve->prime,
                              object->operation.computeSharedSecret->curve->length,
                              object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial
                                + object->operation.computeSharedSecret->curve->length,
                              object->operation.computeSharedSecret->curve->length,
                              &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_SUB_YCOORDINATE_FROM_PRIME_RESULT:

            /* Use scratchBufferSize as a dummy length variable since we will not copy the
             * result into the ECJPAKECC26X2_SCRATCH_BUFFER
             */
            scratchBufferSize = object->operation.computeSharedSecret->curve->length;

            pkaResult = PKABigNumSubGetResult(object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial
                                                + object->operation.computeSharedSecret->curve->length,
                                              &scratchBufferSize,
                                              resultAddress);

            /* Zero out the private key buffer up to the curve length. Otherwise, we may
             * have rubbish floating around the buffer instead of being zero sign-extended.
             */
            if (object->operation.computeSharedSecret->curve->length > scratchBufferSize) {
                PKAZeroOutArray(object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial
                                    + object->operation.computeSharedSecret->curve->length + scratchBufferSize,
                                object->operation.computeSharedSecret->curve->length - scratchBufferSize);
            }

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_ADD_THEIRCOMBINEDPUBLICKEY:

            PKAEccAddStart(object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial,
                           object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial
                            + object->operation.computeSharedSecret->curve->length,
                           object->operation.computeSharedSecret->theirCombinedPublicKey->u.plaintext.keyMaterial,
                           object->operation.computeSharedSecret->theirCombinedPublicKey->u.plaintext.keyMaterial
                            + object->operation.computeSharedSecret->curve->length,
                           object->operation.computeSharedSecret->curve->prime,
                           object->operation.computeSharedSecret->curve->a,
                           object->operation.computeSharedSecret->curve->length,
                           &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_ADD_THEIRCOMBINEDPUBLICKEY_RESULT:

            pkaResult = PKAEccAddGetResult(object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial,
                                           object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial
                                            + object->operation.computeSharedSecret->curve->length,
                                           resultAddress,
                                           object->operation.computeSharedSecret->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);


        case ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_MULT_BY_MYPRIVATEKEY2:

            PKAEccMultiplyStart(object->operation.computeSharedSecret->myPrivateKey2->u.plaintext.keyMaterial,
                                object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial,
                                object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial
                                    + object->operation.computeSharedSecret->curve->length,
                                object->operation.computeSharedSecret->curve->prime,
                                object->operation.computeSharedSecret->curve->a,
                                object->operation.computeSharedSecret->curve->b,
                                object->operation.computeSharedSecret->curve->length,
                                &resultAddress);

            break;

        case ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_MULT_BY_MYPRIVATEKEY2_RESULT:

            pkaResult = PKAEccMultiplyGetResult(object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial,
                                                object->operation.computeSharedSecret->sharedSecret->u.plaintext.keyMaterial
                                                    + object->operation.computeSharedSecret->curve->length,
                                                resultAddress,
                                                object->operation.computeSharedSecret->curve->length);

            return ECJPAKECC26X2_convertReturnValue(pkaResult);

        case ECJPAKECC26X2_FSM_ROUND_ONE_GENERATE_RETURN:
        case ECJPAKECC26X2_FSM_ROUND_TWO_GENERATE_RETURN:
        case ECJPAKECC26X2_FSM_ZKP_GENERATE_RETURN:
        case ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_RETURN:
            return ECJPAKE_STATUS_SUCCESS;
        default:
            return ECJPAKE_STATUS_ERROR;
    }

    // If we get to this point, we want to perform another PKA operation
    IntPendClear(INT_PKA_IRQ);
    IntEnable(INT_PKA_IRQ);

    return ECJPAKECC26X2_STATUS_FSM_RUN_PKA_OP;
}

/*
 *  ======== ECJPAKECC26X2_convertReturnValue ========
 */
static int_fast16_t ECJPAKECC26X2_convertReturnValue(uint32_t pkaResult) {
    switch (pkaResult) {
        case PKA_STATUS_SUCCESS:
        case PKA_STATUS_A_LESS_THAN_B:
        case PKA_STATUS_EQUAL:
            /* A less than B and equal only come up when checking private
             * key values. They indicate a key within the correct range.
             */
            return ECJPAKECC26X2_STATUS_FSM_RUN_FSM;

        case PKA_STATUS_X_ZERO:
        case PKA_STATUS_Y_ZERO:
        case PKA_STATUS_RESULT_0:
            /* Theoretically, PKA_STATUS_RESULT_0 might be caused by other
             * operations failing but the only one that really should yield
             * 0 is ECC multiplication with invalid inputs that yield the
             * point at infinity.
             */
            return ECJPAKE_STATUS_POINT_AT_INFINITY;

        case PKA_STATUS_X_LARGER_THAN_PRIME:
        case PKA_STATUS_Y_LARGER_THAN_PRIME:
            return ECJPAKE_STATUS_PUBLIC_KEY_LARGER_THAN_PRIME;

        case PKA_STATUS_POINT_NOT_ON_CURVE:
            return ECJPAKE_STATUS_PUBLIC_KEY_NOT_ON_CURVE;

        default:
            return ECJPAKE_STATUS_ERROR;
    }
}

/*
 *  ======== ECJPAKECC26X2_waitForAccess ========
 */
static int_fast16_t ECJPAKECC26X2_waitForAccess(ECJPAKE_Handle handle) {
    ECJPAKECC26X2_Object *object = handle->object;
    uint32_t timeout;

    /* Set to SemaphoreP_NO_WAIT to start operations from SWI or HWI context */
    timeout = object->returnBehavior == ECJPAKE_RETURN_BEHAVIOR_BLOCKING ? object->semaphoreTimeout : SemaphoreP_NO_WAIT;

    return SemaphoreP_pend(&PKAResourceCC26XX_accessSemaphore, timeout);
}

/*
 *  ======== ECJPAKECC26X2_waitForResult ========
 */
static int_fast16_t ECJPAKECC26X2_waitForResult(ECJPAKE_Handle handle){
    ECJPAKECC26X2_Object *object = handle->object;

    object->operationInProgress = true;

    switch (object->returnBehavior) {
        case ECJPAKE_RETURN_BEHAVIOR_POLLING:
            while(!PKAResourceCC26XX_pollingFlag);
            return object->operationStatus;

        case ECJPAKE_RETURN_BEHAVIOR_BLOCKING:
            SemaphoreP_pend(&PKAResourceCC26XX_operationSemaphore, SemaphoreP_WAIT_FOREVER);
            return object->operationStatus;

        case ECJPAKE_RETURN_BEHAVIOR_CALLBACK:
            return ECJPAKE_STATUS_SUCCESS;

        default:
            return ECJPAKE_STATUS_ERROR;
    }
}


/*
 *  ======== ECJPAKE_init ========
 */
void ECJPAKE_init(void) {
    PKAResourceCC26XX_constructRTOSObjects();

    isInitialized = true;
}


/*
 *  ======== ECJPAKE_close ========
 */
void ECJPAKE_close(ECJPAKE_Handle handle) {
    ECJPAKECC26X2_Object         *object;

    DebugP_assert(handle);

    /* Get the pointer to the object */
    object = handle->object;

    /* Release power dependency on PKA Module. */
    Power_releaseDependency(PowerCC26X2_PERIPH_PKA);

    /* Mark the module as available */
    object->isOpen = false;
}


/*
 *  ======== ECJPAKE_open ========
 */
ECJPAKE_Handle ECJPAKE_open(uint_least8_t index, ECJPAKE_Params *params) {
    ECJPAKE_Handle                  handle;
    ECJPAKECC26X2_Object           *object;
    uint_fast8_t                    key;

    handle = (ECJPAKE_Handle)&(ECJPAKE_config[index]);
    object = handle->object;

    /* If params are NULL, use defaults */
    if (params == NULL) {
        params = (ECJPAKE_Params *)&ECJPAKE_defaultParams;
    }

    DebugP_assert(index < ECJPAKE_count);

    key = HwiP_disable();

    if (!isInitialized || object->isOpen) {
        HwiP_restore(key);
        return NULL;
    }

    object->isOpen = true;

    HwiP_restore(key);

    DebugP_assert((params->returnBehavior == ECJPAKE_RETURN_BEHAVIOR_CALLBACK) ? params->callbackFxn : true);

    object->returnBehavior = params->returnBehavior;
    object->callbackFxn = params->returnBehavior == ECJPAKE_RETURN_BEHAVIOR_CALLBACK ? params->callbackFxn : ECJPAKECC26X2_internalCallbackFxn;
    object->semaphoreTimeout = params->timeout;

    /* Set power dependency - i.e. power up and enable clock for PKA (PKAResourceCC26XX) module. */
    Power_setDependency(PowerCC26X2_PERIPH_PKA);

    return handle;
}

/*
 *  ======== ECJPAKE_roundOneGenerateKeys ========
 */
int_fast16_t ECJPAKE_roundOneGenerateKeys(ECJPAKE_Handle handle, ECJPAKE_OperationRoundOneGenerateKeys *operation) {
    ECJPAKECC26X2_Object *object              = handle->object;
    ECJPAKECC26X2_HWAttrs const *hwAttrs      = handle->hwAttrs;

    if (ECJPAKECC26X2_waitForAccess(handle) != SemaphoreP_OK) {
        return ECJPAKE_STATUS_RESOURCE_UNAVAILABLE;
    }

    /* Copy over all parameters we will need access to in the FSM.
     * The FSM runs in SWI context and thus needs to keep track of
     * all of them somehow.
     */
    object->fsmState                        = ECJPAKECC26X2_FSM_ROUND_ONE_VALIDATE_MYPRIVATEKEY1;
    object->operationStatus                 = ECJPAKE_STATUS_ERROR;
    object->operation.generateRoundOneKeys  = operation;
    object->operationType                   = ECJPAKE_OPERATION_TYPE_ROUND_ONE_GENERATE_KEYS;
    object->operationCanceled               = false;
    scratchBufferSize                       = ECJPAKECC26X2_SCRATCH_BUFFER_SIZE;


    /* We need to set the HWI function and priority since the same physical interrupt is shared by multiple
     * drivers and they all need to coexist. Whenever a driver starts an operation, it
     * registers its HWI callback with the OS.
     */
    HwiP_setFunc(&PKAResourceCC26XX_hwi, ECJPAKECC26X2_hwiFxn, (uintptr_t)handle);
    HwiP_setPriority(INT_PKA_IRQ, hwAttrs->intPriority);

    PKAResourceCC26XX_pollingFlag = 0;

    Power_setConstraint(PowerCC26XX_DISALLOW_STANDBY);

    /* Run the FSM by triggering the interrupt. It is level triggered
     * and the complement of the RUN bit.
     */
    IntEnable(INT_PKA_IRQ);

    return ECJPAKECC26X2_waitForResult(handle);
}

/*
 *  ======== ECJPAKE_generateZKP ========
 */
int_fast16_t ECJPAKE_generateZKP(ECJPAKE_Handle handle, ECJPAKE_OperationGenerateZKP *operation) {
    ECJPAKECC26X2_Object *object              = handle->object;
    ECJPAKECC26X2_HWAttrs const *hwAttrs      = handle->hwAttrs;

    if (ECJPAKECC26X2_waitForAccess(handle) != SemaphoreP_OK) {
        return ECJPAKE_STATUS_RESOURCE_UNAVAILABLE;
    }

    /* Copy over all parameters we will need access to in the FSM.
     * The FSM runs in SWI context and thus needs to keep track of
     * all of them somehow.
     */
    object->fsmState                        = ECJPAKECC26X2_FSM_GENERATE_ZKP_PRIVATEKEY_X_HASH;
    object->operationStatus                 = ECJPAKE_STATUS_ERROR;
    object->operation.generateZKP           = operation;
    object->operationType                   = ECJPAKE_OPERATION_TYPE_GENERATE_ZKP;
    object->operationCanceled               = false;
    scratchBufferSize                       = ECJPAKECC26X2_SCRATCH_BUFFER_SIZE;

    /* We need to set the HWI function and priority since the same physical interrupt is shared by multiple
     * drivers and they all need to coexist. Whenever a driver starts an operation, it
     * registers its HWI callback with the OS.
     */
    HwiP_setFunc(&PKAResourceCC26XX_hwi, ECJPAKECC26X2_hwiFxn, (uintptr_t)handle);
    HwiP_setPriority(INT_PKA_IRQ, hwAttrs->intPriority);

    PKAResourceCC26XX_pollingFlag = 0;

    Power_setConstraint(PowerCC26XX_DISALLOW_STANDBY);

    /* Run the FSM by triggering the interrupt. It is level triggered
     * and the complement of the RUN bit.
     */
    IntEnable(INT_PKA_IRQ);

    return ECJPAKECC26X2_waitForResult(handle);
}

/*
 *  ======== ECJPAKE_verifyZKP ========
 */
int_fast16_t ECJPAKE_verifyZKP(ECJPAKE_Handle handle, ECJPAKE_OperationVerifyZKP *operation) {
    ECJPAKECC26X2_Object *object              = handle->object;
    ECJPAKECC26X2_HWAttrs const *hwAttrs      = handle->hwAttrs;

    if (ECJPAKECC26X2_waitForAccess(handle) != SemaphoreP_OK) {
        return ECJPAKE_STATUS_RESOURCE_UNAVAILABLE;
    }

    /* Copy over all parameters we will need access to in the FSM.
     * The FSM runs in SWI context and thus needs to keep track of
     * all of them somehow.
     */
    object->fsmState                        = ECJPAKECC26X2_FSM_VERIFY_ZKP_VALIDATE_PUBLIC_KEY;
    object->operationStatus                 = ECJPAKE_STATUS_ERROR;
    object->operation.verifyZKP             = operation;
    object->operationType                   = ECJPAKE_OPERATION_TYPE_VERIFY_ZKP;
    object->operationCanceled               = false;
    scratchBufferSize                       = ECJPAKECC26X2_SCRATCH_BUFFER_SIZE;

    /* We need to set the HWI function and priority since the same physical interrupt is shared by multiple
     * drivers and they all need to coexist. Whenever a driver starts an operation, it
     * registers its HWI callback with the OS.
     */
    HwiP_setFunc(&PKAResourceCC26XX_hwi, ECJPAKECC26X2_hwiFxn, (uintptr_t)handle);
    HwiP_setPriority(INT_PKA_IRQ, hwAttrs->intPriority);

    PKAResourceCC26XX_pollingFlag = 0;

    Power_setConstraint(PowerCC26XX_DISALLOW_STANDBY);

    /* Run the FSM by triggering the interrupt. It is level triggered
     * and the complement of the RUN bit.
     */
    IntEnable(INT_PKA_IRQ);

    return ECJPAKECC26X2_waitForResult(handle);
}

/*
 *  ======== ECJPAKE_roundTwoGenerateKeys ========
 */
int_fast16_t ECJPAKE_roundTwoGenerateKeys(ECJPAKE_Handle handle, ECJPAKE_OperationRoundTwoGenerateKeys *operation) {
    ECJPAKECC26X2_Object *object              = handle->object;
    ECJPAKECC26X2_HWAttrs const *hwAttrs      = handle->hwAttrs;

    if (ECJPAKECC26X2_waitForAccess(handle) != SemaphoreP_OK) {
        return ECJPAKE_STATUS_RESOURCE_UNAVAILABLE;
    }

    /* Copy over all parameters we will need access to in the FSM.
     * The FSM runs in SWI context and thus needs to keep track of
     * all of them somehow.
     */
    object->fsmState                        = ECJPAKECC26X2_FSM_ROUND_TWO_MULT_MYPRIVATEKEY2_BY_PRESHAREDSECRET;
    object->operationStatus                 = ECJPAKE_STATUS_ERROR;
    object->operation.generateRoundTwoKeys  = operation;
    object->operationType                   = ECJPAKE_OPERATION_TYPE_ROUND_TWO_GENERATE_KEYS;
    object->operationCanceled               = false;
    scratchBufferSize                       = ECJPAKECC26X2_SCRATCH_BUFFER_SIZE;


    /* We need to set the HWI function and priority since the same physical interrupt is shared by multiple
     * drivers and they all need to coexist. Whenever a driver starts an operation, it
     * registers its HWI callback with the OS.
     */
    HwiP_setFunc(&PKAResourceCC26XX_hwi, ECJPAKECC26X2_hwiFxn, (uintptr_t)handle);
    HwiP_setPriority(INT_PKA_IRQ, hwAttrs->intPriority);

    PKAResourceCC26XX_pollingFlag = 0;

    Power_setConstraint(PowerCC26XX_DISALLOW_STANDBY);

    /* Run the FSM by triggering the interrupt. It is level triggered
     * and the complement of the RUN bit.
     */
    IntEnable(INT_PKA_IRQ);

    return ECJPAKECC26X2_waitForResult(handle);
}

/*
 *  ======== ECJPAKE_computeSharedSecret ========
 */
int_fast16_t ECJPAKE_computeSharedSecret(ECJPAKE_Handle handle, ECJPAKE_OperationComputeSharedSecret *operation) {
    ECJPAKECC26X2_Object *object              = handle->object;
    ECJPAKECC26X2_HWAttrs const *hwAttrs      = handle->hwAttrs;

    if (ECJPAKECC26X2_waitForAccess(handle) != SemaphoreP_OK) {
        return ECJPAKE_STATUS_RESOURCE_UNAVAILABLE;
    }

    /* Copy over all parameters we will need access to in the FSM.
     * The FSM runs in SWI context and thus needs to keep track of
     * all of them somehow.
     */
    object->fsmState                        = ECJPAKECC26X2_FSM_GENERATE_SHARED_SECRET_MULT_THEIRPUBLICKEY2_BY_MYCOMBINEDPRIVATEKEY;
    object->operationStatus                 = ECJPAKE_STATUS_ERROR;
    object->operation.computeSharedSecret   = operation;
    object->operationType                   = ECJPAKE_OPERATION_TYPE_COMPUTE_SHARED_SECRET;
    object->operationCanceled               = false;
    scratchBufferSize                       = ECJPAKECC26X2_SCRATCH_BUFFER_SIZE;


    /* We need to set the HWI function and priority since the same physical interrupt is shared by multiple
     * drivers and they all need to coexist. Whenever a driver starts an operation, it
     * registers its HWI callback with the OS.
     */
    HwiP_setFunc(&PKAResourceCC26XX_hwi, ECJPAKECC26X2_hwiFxn, (uintptr_t)handle);
    HwiP_setPriority(INT_PKA_IRQ, hwAttrs->intPriority);

    PKAResourceCC26XX_pollingFlag = 0;

    Power_setConstraint(PowerCC26XX_DISALLOW_STANDBY);

    /* Run the FSM by triggering the interrupt. It is level triggered
     * and the complement of the RUN bit.
     */
    IntEnable(INT_PKA_IRQ);

    return ECJPAKECC26X2_waitForResult(handle);
}

/*
 *  ======== ECJPAKE_cancelOperation ========
 */
int_fast16_t ECJPAKE_cancelOperation(ECJPAKE_Handle handle) {
    ECJPAKECC26X2_Object *object = handle->object;

    if(!object->operationInProgress){
        return ECJPAKE_STATUS_ERROR;
    }

    object->operationCanceled = true;

    /* Post hwi as if operation finished for cleanup */
    IntEnable(INT_PKA_IRQ);
    HwiP_post(INT_PKA_IRQ);


    return ECJPAKE_STATUS_SUCCESS;
}
