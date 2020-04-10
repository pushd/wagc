//
// Created by James Tomson on 2020-04-09.
//
// Taken from examples at https://github.com/cpuimage/WebRTC_AGC

#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <jni.h>
#include <android/log.h>

#include "agc.h"

#define MIN(A,B) ((A < B)? A : B )

#define  LOG_TAG    "Wagc"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

int agcProcess(int16_t *buffer, uint32_t sampleRate, size_t samplesCount, void *agcInst, int16_t *outBuffer) {
    size_t numBands = 1;
    int inMicLevel = 0;
    int outMicLevel = -1;
    uint8_t saturationWarning = 1;
    int16_t echo = 0;
    size_t numSamples10ms = MIN(160, sampleRate / 100);
    size_t nIterations = (samplesCount / numSamples10ms);
    int16_t *input = buffer;
    int16_t *out16 = outBuffer;

    for (int i = 0; i < nIterations; i++) {
        int nAgcRet = WebRtcAgc_Process(agcInst, (const int16_t *const *) &input, numBands, numSamples10ms,
                                        (int16_t *const *) &out16, inMicLevel, &outMicLevel, echo,
                                        &saturationWarning);

        if (nAgcRet != 0) {
            LOGE("failed in WebRtcAgc_Process ret: %d", nAgcRet);
            WebRtcAgc_Free(agcInst);
            return -1;
        }
        input += numSamples10ms;
        out16 += numSamples10ms;
    }

    const size_t remainingSamples = samplesCount - (nIterations * numSamples10ms);
    if (remainingSamples > 0) {
        if (nIterations > 0) {
            int offset = numSamples10ms + remainingSamples;
            input -= offset;
            out16 -= offset;
        }

        int nAgcRet = WebRtcAgc_Process(agcInst, (const int16_t *const *) &input, numBands, numSamples10ms,
                                        (int16_t *const *) &out16, inMicLevel, &outMicLevel, echo,
                                        &saturationWarning);

        if (nAgcRet != 0) {
            LOGE("failed in WebRtcAgc_Process during filtering the last chunk ret: %d", nAgcRet);
            WebRtcAgc_Free(agcInst);
            return -1;
        }
        memcpy(&input[numSamples10ms-remainingSamples], &outBuffer[numSamples10ms-remainingSamples], remainingSamples * sizeof(int16_t));
    }

    return outMicLevel;
}

JNIEXPORT jlong Java_com_pushd_wagc_Wagc_init(JNIEnv *, jobject) {
    WebRtcAgcConfig agcConfig;
    agcConfig.compressionGaindB = 9; // default 9 dB, maximum gain the digital compression stage many apply in dB
    agcConfig.limiterEnable = 1; // default kAgcTrue (on)
    agcConfig.targetLevelDbfs = 3; // default 3 (-3 dBOv), the decrease value relative to Full Scale (0). The smaller the value, the louder the sound.
    uint32_t sampleRate = 16000;
    int minLevel = 0;
    int maxLevel = 255;

    void *agcInst = WebRtcAgc_Create();
    if (agcInst == NULL) return -1;
    int status = WebRtcAgc_Init(agcInst, minLevel, maxLevel, kAgcModeFixedDigital, sampleRate);
    if (status != 0) {
        LOGE("WebRtcAgc_Init fail status: %d", status);
        WebRtcAgc_Free(agcInst);
        return -1;
    }
    status = WebRtcAgc_set_config(agcInst, agcConfig);
    if (status != 0) {
        LOGE("WebRtcAgc_set_config fail status: %d", status);
        WebRtcAgc_Free(agcInst);
        return -1;
    }
    return (jlong)agcInst;
}

JNIEXPORT void Java_com_pushd_wagc_Wagc_destroy(JNIEnv *, jobject, jlong longHandle) {
    WebRtcAgc_Free((void*)longHandle);
}

JNIEXPORT jint Java_com_pushd_wagc_Wagc_process(JNIEnv *env, jobject, jlong longHandle, jshortArray inArray, jshortArray outArray) {
    jsize samplesCount = env->GetArrayLength(inArray);
    jshort* inBuffer = env->GetShortArrayElements(inArray, 0);
    jshort* outBuffer = env->GetShortArrayElements(outArray, 0);

    int outMicLevel = agcProcess(inBuffer,
                                 (uint32_t)16000,
                                 (size_t)samplesCount,
                                 (void*)longHandle,
                                 outBuffer);

    env->ReleaseShortArrayElements(inArray, inBuffer, JNI_COMMIT);
    env->ReleaseShortArrayElements(outArray, outBuffer, JNI_COMMIT);

    return outMicLevel;
}

#ifdef __cplusplus
}
#endif