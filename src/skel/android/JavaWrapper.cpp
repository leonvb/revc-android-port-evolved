//
// Created by mrxenginner on 14/07/2025.
//

#include "common.h"
#include "JavaWrapper.h"

#if defined ANDROID

#include <SDL_system.h>

extern "C" JavaVM *javaVM;

JNIEnv* CJavaWrapper::GetEnv() {
    if (!javaVM) {
        return (JNIEnv*)SDL_AndroidGetJNIEnv();
    }

    JNIEnv* env = NULL;

    int getEnvStat = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (getEnvStat == JNI_EDETACHED) {
        debug("GetEnv: not attached");
        if (javaVM->AttachCurrentThread(&env, NULL) != 0) {
            debug("Failed to attach");
            return NULL;
        }
    }

    if (getEnvStat == JNI_EVERSION) {
        debug("GetEnv: version not supported");
        return NULL;
    }

    if (getEnvStat == JNI_ERR) {
        debug("GetEnv: JNI_ERR");
        return NULL;
    }
    return env;
}

void CJavaWrapper::ExitGame() {

    JNIEnv* env = GetEnv();

    if (!env) {
        debug("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_ExitGame);
}

CJavaWrapper::CJavaWrapper(JNIEnv *env, jobject activity) {
    this->activity = env->NewGlobalRef(activity);

    jclass nvEventClass = env->GetObjectClass(activity);

    if (!nvEventClass) {
        debug("nvEventClass null");
        return;
    }

    s_ExitGame = env->GetMethodID(nvEventClass, "exitGame", "()V");
    env->DeleteLocalRef(nvEventClass);
    env->DeleteLocalRef(activity);
}

CJavaWrapper::~CJavaWrapper() {
    JNIEnv* pEnv = GetEnv();
    if (pEnv) {
        pEnv->DeleteGlobalRef(this->activity);
    }
}

void CJavaWrapper::SetTargetRefreshRate(int fps) {
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
    if (!env) {
        env = GetEnv();
    }
    if (!env) {
        return;
    }

    jobject act = (jobject)SDL_AndroidGetActivity();
    bool releaseLocalAct = true;
    if (!act && g_pJavaWrapper) {
        act = g_pJavaWrapper->activity;
        releaseLocalAct = false;
    }
    if (!act) {
        return;
    }

    jclass actClass = env->GetObjectClass(act);
    if (actClass) {
        jmethodID mid = env->GetMethodID(actClass, "setTargetRefreshRate", "(I)V");
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            mid = nullptr;
        }
        if (mid) {
            env->CallVoidMethod(act, mid, (jint)fps);
            if (env->ExceptionCheck()) {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
        }
        env->DeleteLocalRef(actClass);
    }

    if (releaseLocalAct) {
        env->DeleteLocalRef(act);
    }
}

CJavaWrapper* g_pJavaWrapper = nullptr;
#endif