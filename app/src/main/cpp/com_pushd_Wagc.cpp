//
// Created by James Tomson on 2020-04-09.
//
// Taken from examples at https://github.com/cpuimage/WebRTC_AGC

#include <stdlib.h>
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

int agcProcess(int16_t *buffer, uint32_t sampleRate, size_t samplesCount, int16_t agcMode, void *agcInst, int inMicLevel, int16_t *out_buffer) {
    size_t num_bands = 1;
    int outMicLevel = -1;
    uint8_t saturationWarning = 1;                 //是否有溢出发生，增益放大以后的最大值超过了65536
    int16_t echo = 0;                                 //增益放大是否考虑回声影响
    size_t samples = MIN(160, sampleRate / 100);
    size_t nTotal = (samplesCount / samples);
    int16_t *input = buffer;
    int16_t *out16 = out_buffer;

    for (int i = 0; i < nTotal; i++) {
        inMicLevel = 0;
        int nAgcRet = WebRtcAgc_Process(agcInst, (const int16_t *const *) &input, num_bands, samples,
                                        (int16_t *const *) &out16, inMicLevel, &outMicLevel, echo,
                                        &saturationWarning);

        if (nAgcRet != 0) {
            LOGE("failed in WebRtcAgc_Process ret: %d", nAgcRet);
            WebRtcAgc_Free(agcInst);
            return -1;
        }
        input += samples;
    }

    const size_t remainedSamples = samplesCount - nTotal * samples;
    if (remainedSamples > 0) {
        if (nTotal > 0) {
            input = input - samples + remainedSamples;
        }

        inMicLevel = 0;
        int nAgcRet = WebRtcAgc_Process(agcInst, (const int16_t *const *) &input, num_bands, samples,
                                        (int16_t *const *) &out16, inMicLevel, &outMicLevel, echo,
                                        &saturationWarning);

        if (nAgcRet != 0) {
            LOGE("failed in WebRtcAgc_Process during filtering the last chunk ret: %d", nAgcRet);
            WebRtcAgc_Free(agcInst);
            return -1;
        }
        memcpy(&input[samples-remainedSamples], &out_buffer[samples-remainedSamples], remainedSamples * sizeof(int16_t));
        input += samples;
    }

    return outMicLevel;
}

JNIEXPORT jlong Java_com_pushd_wagc_Wagc_init(JNIEnv *, jobject) {
    WebRtcAgcConfig agcConfig;
    agcConfig.compressionGaindB = 9; // default 9 dB
    agcConfig.limiterEnable = 1; // default kAgcTrue (on)
    agcConfig.targetLevelDbfs = 3; // default 3 (-3 dBOv)
    int sampleRate = 16000;
    int minLevel = 0;
    int maxLevel = 255;

    void *agcInst = WebRtcAgc_Create();
    if (agcInst == NULL) return -1;
    int status = WebRtcAgc_Init(agcInst, minLevel, maxLevel, kAgcModeAdaptiveDigital, sampleRate);
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

JNIEXPORT jint Java_com_pushd_wagc_Wagc_process(JNIEnv *env, jobject, jlong longHandle, jshortArray inArray, jint micLevel, jshortArray outArray) {
    jsize samplesCount = env->GetArrayLength(inArray);
    jshort* inBuffer = env->GetShortArrayElements(inArray, 0);
    jshort* outBuffer = env->GetShortArrayElements(outArray, 0);

    int outMicLevel = agcProcess((int16_t*)inBuffer,
                                 (uint32_t)16000,
                                 (size_t)samplesCount,
                                 kAgcModeAdaptiveDigital,
                                 (void*)longHandle,
                                 micLevel,
                                 outBuffer);

    env->ReleaseShortArrayElements(inArray, inBuffer, JNI_COMMIT);
    env->ReleaseShortArrayElements(outArray, outBuffer, JNI_COMMIT);

    return outMicLevel;
}

#ifdef __cplusplus
}
#endif