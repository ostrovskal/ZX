
#include "zxCommon.h"
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>

static jobject objBmp = nullptr;
static jobject objProps = nullptr;

static int parseExtension(const char* name) {
    auto l = strlen(name);
    const char* ext = (l > 2 ? name + (l - 3) : "123");
    const char* lst[] = {".zx", "tga", "z80", "tap", "wav", "trd"};
    int ret = 0;
    for(auto& e: lst) {
        if(!strcasecmp(ext, e)) break;
        ret++;
    }
    return ret;
}

static void copyAFile(AAssetManager* aMgr, const char* aPath, const char* path, uint8_t** buffer = nullptr) {
    auto aFile = AAssetManager_open(aMgr, aPath, AASSET_MODE_UNKNOWN);
    bool result = false;
    if(aFile) {
        auto aLen = (size_t) AAsset_getLength(aFile);
        if(buffer) *buffer = new uint8_t[aLen];
        auto aBuf = buffer != nullptr ? *buffer : TMP_BUF;
        if(aLen < ZX_SIZE_TMP_BUF) {
            AAsset_read(aFile, aBuf, aLen);
            AAsset_close(aFile);
            if (!buffer) zxFile::writeFile(path, TMP_BUF, aLen);
            result = true;
        }
    }
    if(!result) info("Не удалось скопировать файл <%s>!", aPath);
}

extern "C" {

    int zxExecute(JNIEnv*, jclass) {
        ALU->execute();
        return ALU->updateKeys(0, 0);
    }

    void zxSurface(JNIEnv *env, jclass, jobject bmp) {
        info("zxSurface1");
        if(objBmp) {
            env->DeleteGlobalRef(objBmp);
            objBmp = nullptr;
        }
        if(!(objBmp = env->NewGlobalRef(bmp))) {
            info("NewGlobalRef(bmp) error!");
            abort();
        }
        if (AndroidBitmap_lockPixels(env, bmp, (void**)&ALU->surface) < 0) {
            info("AndroidBitmap_lockPixels error!");
            abort();
        }
        AndroidBitmap_unlockPixels(env, bmp);
        ALU->updateProps();
        info("zxSurface2");
    }

    void zxShutdown(JNIEnv *env, jobject) {
        info("zxShutdown1");
        if(objBmp) {
            env->DeleteGlobalRef(objBmp);
            objBmp = nullptr;
        }
        if(objProps) {
            env->DeleteGlobalRef(objProps);
            objProps = nullptr;
        }
        info("zxShutdown2");
    }

    jboolean zxIO(JNIEnv* env, jclass, jstring nm, jboolean load) {
        auto name = env->GetStringUTFChars(nm, nullptr);
        info("zxIO1 name: %s load: %i", name, load);
        auto type = parseExtension(name);
        auto ret = (jboolean)(load ? ALU->load(name, type) : ALU->save(name, type));
        if(!ret) info("Не удалось загрузить/записать <%s>!", name);
        info("zxIO2");
        return ret;
    }

    void zxInit(JNIEnv *env, jobject, jobject asset, jboolean errors) {
        info("zxInit1");
        TMP_BUF = new uint8_t[ZX_SIZE_TMP_BUF];
        ALU = new zxALU();

        auto amgr = AAssetManager_fromJava(env, asset);
        if(opts[ZX_PROP_FIRST_LAUNCH]) {
            // перенести все игры из активов на диск
            std::string fl("games/");
            auto dir = AAssetManager_openDir(amgr, "games");
            const char* file;
            while ((file = AAssetDir_getNextFileName(dir))) {
                copyAFile(amgr, (fl + file).c_str(), file);
            }
            AAssetDir_close(dir);
            opts[ZX_PROP_FIRST_LAUNCH] = 0;
            copyAFile(amgr, "tapLoader.zx", "tapLoader.zx");
            copyAFile(amgr, "trdLoader.zx", "trdLoader.zx");
        }
        copyAFile(amgr, "labels.bin", nullptr, &labels);
        copyAFile(amgr, "zx.rom", nullptr, &ALU->ROMS);
        ALU->changeModel(opts[ZX_PROP_MODEL_TYPE], 255, true);
        if(!errors) ALU->load(ZX_AUTO_SAVE, ZX_CMD_IO_STATE);
        info("zxInit2");
    }

    jbyteArray zxSaveState(JNIEnv* env, jclass) {
        info("zxSaveState1");
        auto jmemory = env->NewByteArray(262144);
        auto mem = (uint8_t*) env->GetPrimitiveArrayCritical(jmemory, nullptr);
        memcpy(mem, ALU->RAMs, 262144);
        env->ReleasePrimitiveArrayCritical((jarray)jmemory, mem, JNI_ABORT);
        ALU->setPages();
        info("zxSaveState2");
        return jmemory;
    }

    void zxLoadState(JNIEnv* env, jclass, jbyteArray jmemory) {
        info("zxLoadState1");
        auto mem = (uint8_t*) env->GetPrimitiveArrayCritical(jmemory, nullptr);
        memcpy(ALU->RAMs, mem, 262144);
        env->ReleasePrimitiveArrayCritical((jarray)jmemory, mem, JNI_ABORT);
        ALU->updateProps();
        info("zxLoadState2");
    }

    void zxProps(JNIEnv* env, jclass, jbyteArray props) {
        info("zxProps1");
        if(!(objProps = env->NewGlobalRef(props))) {
            info("NewGlobalRef(props) error!");
            abort();
        }
        opts = (uint8_t*) env->GetPrimitiveArrayCritical(props, nullptr);
        ssh_memzero(opts, ZX_PROPS_COUNT);
        env->ReleasePrimitiveArrayCritical((jarray)props, opts, JNI_ABORT);
        info("zxProps2");
    }

    jstring zxPresets(JNIEnv* env, jclass, jint cmd) {
        info("zxPresets1");
        auto ret = ALU->presets("", cmd);
        info("zxPresets2 res: %s", ret);
        return env->NewStringUTF(ret);
    }

    jstring zxSetProp(JNIEnv* env, jclass, jint idx) {
//        info("zxSetProp1 idx: %i", idx);
        const char* ret = nullptr;
        idx += ZX_PROP_FIRST_LAUNCH;
        uint32_t n = opts[idx];
        if(idx < ZX_PROP_ACTIVE_DISK) ret = ssh_ntos(&n, RADIX_BOL);
        else if(idx < ZX_PROP_COLORS) ret = ssh_ntos(&n, RADIX_DEC);
        else if(idx < (ZX_PROP_COLORS + 23)) {
            n = *(uint32_t*)(opts + (ZX_PROP_COLORS + (idx - ZX_PROP_COLORS) * 4));
            ret = ssh_ntos(&n, RADIX_HEX);
        }
//        info("zxSetProp2 res: %s", ret);
        return env->NewStringUTF(ret);
    }

    void zxGetProp(JNIEnv* env, jclass, jstring key, jint idx) {
        auto kv = env->GetStringUTFChars(key, nullptr);
//        info("zxGetProp1 kv: %s idx: %i", kv, idx);
        if(kv) {
            idx += ZX_PROP_FIRST_LAUNCH;
            if (idx < ZX_PROP_ACTIVE_DISK) opts[idx] = *(uint8_t *) ssh_ston(kv, RADIX_BOL);
            else if (idx < ZX_PROP_COLORS) opts[idx] = *(uint8_t *) ssh_ston(kv, RADIX_DEC);
            else if (idx < (ZX_PROP_COLORS + 23))
                *(uint32_t *) (opts + (ZX_PROP_COLORS + (idx - ZX_PROP_COLORS) * 4)) = *(uint32_t *) ssh_ston(kv, RADIX_HEX);
//            info("zxGetProp2");
        }
    }

    long zxInt(JNIEnv*, jclass, jint idx, jboolean read, jint value) {
        if(!read) {
//            info("zxIntSave idx: %i value: %i", idx, value);
            *(uint32_t*)(opts + idx) = (uint32_t)value;
        }
        auto ret = (long)(*(uint32_t*)(opts + idx));
//        info("zxIntRead idx: %i value: %i", idx, ret);
        return ret;
    }

    jint zxCmd(JNIEnv* env, jclass, jint cmd, jint arg1, jint arg2, jstring arg3) {
//        info("zxCmd1 cmd: %i", cmd);
        switch(cmd) {
            default:                info("Неизвестная комманда в zxCmd(%i)", cmd); break;
            case ZX_CMD_POKE:       ::wm8(realPtr((uint16_t)arg1), (uint8_t)arg2); break;
            case ZX_CMD_UPDATE_KEY: ALU->updateKeys(arg1, arg2); break;
            case ZX_CMD_PRESETS:    ALU->presets(env->GetStringUTFChars(arg3, nullptr), arg1); break;
            case ZX_CMD_DIAG:       ALU->diag(); break;
            case ZX_CMD_PROPS:      ALU->updateProps(); break;
            case ZX_CMD_MODEL:      ALU->changeModel(opts[ZX_PROP_MODEL_TYPE], *ALU->_MODEL, true); break;
            case ZX_CMD_RESET:      ALU->signalRESET(true); break;
        }
//        info("zxCmd2");
        return 0;
    }

    jint zxStringToNumber(JNIEnv* env, jclass, jstring value, jint radix) {
        return *(jint*)ssh_ston(env->GetStringUTFChars(value, nullptr), (int)radix);
    }

    jstring zxNumberToString(JNIEnv* env, jclass, jint value, jint radix) {
        return env->NewStringUTF(ssh_ntos(&value, radix));
    }

    static JNINativeMethod zxMethods[] = {
            { "zxInit", "(Landroid/content/res/AssetManager;Z)V", (void*)&zxInit },
            { "zxShutdown", "()V", (void*)&zxShutdown },
            { "zxProps", "([B)V", (void*)&zxProps },
            { "zxGetProp", "(Ljava/lang/String;I)V", (void*)&zxGetProp },
            { "zxSetProp", "(I)Ljava/lang/String;", (void*)&zxSetProp },
            { "zxExecute", "()I", (void*)&zxExecute },
            { "zxSurface", "(Landroid/graphics/Bitmap;)V", (void*)&zxSurface },
            { "zxCmd", "(IIILjava/lang/String;)I", (void*)&zxCmd },
            { "zxIO", "(Ljava/lang/String;Z)Z", (void*)&zxIO },
            { "zxPresets", "(I)Ljava/lang/String;", (void*)&zxPresets },
            { "zxInt", "(IZI)J", (void*)&zxInt },
            { "zxSaveState", "()[B", (void*)&zxSaveState },
            { "zxLoadState", "([B)V", (void*)&zxLoadState },
            { "zxStringToNumber", "(Ljava/lang/String;I)I", (void*)&zxStringToNumber },
            { "zxNumberToString", "(II)Ljava/lang/String;", (void*)&zxNumberToString }
    };

    JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
        JNIEnv *env;
        if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
            info("GetEnv() error!");
            abort();
        }
        auto wnd = env->FindClass("ru/ostrovskal/zx/ZxWnd");
        env->RegisterNatives(wnd, zxMethods, (sizeof(zxMethods) / sizeof(JNINativeMethod)));

        return JNI_VERSION_1_6;
    }
}
