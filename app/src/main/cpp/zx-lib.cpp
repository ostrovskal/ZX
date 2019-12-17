
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
    if(!result) debug("Не удалось скопировать файл <%s>!", aPath);
}

extern "C" {

    int zxExecute(JNIEnv*, jclass) {
        ALU->execute();
        if(*ALU->_STATE & ZX_BP) {
            debug("zx_bp %i", zxALU::PC);
            modifySTATE(0, ZX_BP);
            return 2;
        }
        return ALU->updateKeys(0, 0);
    }

    void zxSurface(JNIEnv *env, jclass, jobject bmp) {
        info("zxSurface");
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
        debug("zxSurface finish");
    }

    void zxShutdown(JNIEnv *env, jobject) {
        info("zxShutdown");
        if(objBmp) {
            env->DeleteGlobalRef(objBmp);
            objBmp = nullptr;
        }
        if(objProps) {
            env->DeleteGlobalRef(objProps);
            objProps = nullptr;
        }
        debug("zxShutdown finish");
    }

    jboolean zxIO(JNIEnv* env, jclass, jstring nm, jboolean load) {
        auto name = env->GetStringUTFChars(nm, nullptr);
        info("zxIO name: %s load: %i", name, load);
        auto type = parseExtension(name);
        auto ret = (jboolean)(load ? ALU->load(name, type) : ALU->save(name, type));
        if(!ret) debug("Не удалось загрузить/записать <%s>!", name);
        debug("zxIO finish");
        return ret;
    }

    void zxInit(JNIEnv *env, jobject, jobject asset, jstring name, jboolean errors) {
        auto nameSave = env->GetStringUTFChars(name, nullptr);
        info("zxInit name: %s error: %i", nameSave, errors);
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
        if(!errors) ALU->load(nameSave, ZX_CMD_IO_STATE);
        debug("zxInit finish");
    }

    jbyteArray zxSaveState(JNIEnv* env, jclass) {
        info("zxSaveState");
        auto jmemory = env->NewByteArray(262144);
        auto mem = (uint8_t*) env->GetPrimitiveArrayCritical(jmemory, nullptr);
        memcpy(mem, ALU->RAMs, 262144);
        env->ReleasePrimitiveArrayCritical((jarray)jmemory, mem, JNI_ABORT);
        ALU->setPages();
        debug("zxSaveState finish");
        return jmemory;
    }

    void zxLoadState(JNIEnv* env, jclass, jbyteArray jmemory) {
        info("zxLoadState");
        auto mem = (uint8_t*) env->GetPrimitiveArrayCritical(jmemory, nullptr);
        memcpy(ALU->RAMs, mem, 262144);
        env->ReleasePrimitiveArrayCritical((jarray)jmemory, mem, JNI_ABORT);
        ALU->updateProps();
        debug("zxLoadState finish");
    }

    void zxProps(JNIEnv* env, jclass, jbyteArray props) {
        info("zxProps");
        if(!(objProps = env->NewGlobalRef(props))) {
            info("NewGlobalRef(props) error!");
            abort();
        }
        opts = (uint8_t*) env->GetPrimitiveArrayCritical(props, nullptr);
        bps = (BREAK_POINT*)&opts[258];
        env->ReleasePrimitiveArrayCritical((jarray)props, opts, JNI_ABORT);
        ssh_memzero(opts, ZX_PROPS_COUNT);
        debug("zxProps finish");
    }

    jstring zxPresets(JNIEnv* env, jclass, jint cmd) {
        info("zxPresets");
        auto ret = ALU->presets("", cmd);
        debug("zxPresets res: %s", ret);
        return env->NewStringUTF(ret);
    }

    jstring zxSetProp(JNIEnv* env, jclass, jint idx) {
        debug("zxSetProp idx: %i", idx);
        char* ret = nullptr;
        idx += ZX_PROP_FIRST_LAUNCH;
        uint32_t n = opts[idx];
        if(idx < ZX_PROP_ACTIVE_DISK) ret = ssh_ntos(&n, RADIX_BOL);
        else if(idx < ZX_PROP_COLORS) ret = ssh_ntos(&n, RADIX_DEC);
        else if(idx < (ZX_PROP_COLORS + 22)) {
            n = *(uint32_t*)(opts + (ZX_PROP_COLORS + (idx - ZX_PROP_COLORS) * 4));
            ret = ssh_ntos(&n, RADIX_HEX);
        } else if(idx < (ZX_PROP_BPS + 8)) {
            auto bp = &bps[idx - ZX_PROP_BPS];
            auto tmp = ret = (char*)&TMP_BUF[65536];
            n = bp->address1; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->address2; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->msk; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->val; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->ops; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->flg; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX));
        }
        debug("zxSetProp2 finish res: %s", ret);
        return env->NewStringUTF(ret);
    }

    void zxGetProp(JNIEnv* env, jclass, jstring key, jint idx) {
        auto kv = env->GetStringUTFChars(key, nullptr);
        debug("zxGetProp kv: %s idx: %i", kv, idx);
        if(kv) {
            idx += ZX_PROP_FIRST_LAUNCH;
            if (idx < ZX_PROP_ACTIVE_DISK) opts[idx] = *(uint8_t *) ssh_ston(kv, RADIX_BOL);
            else if (idx < ZX_PROP_COLORS) opts[idx] = *(uint8_t *) ssh_ston(kv, RADIX_DEC);
            else if (idx < (ZX_PROP_COLORS + 22))
                *(uint32_t *) (opts + (ZX_PROP_COLORS + (idx - ZX_PROP_COLORS) * 4)) = *(uint32_t *) ssh_ston(kv, RADIX_HEX);
            else if(idx < (ZX_PROP_BPS + 8)) {
                auto bp = &bps[idx - ZX_PROP_BPS];
                bp->address1 = *(uint16_t *)ssh_ston(kv, RADIX_HEX, &kv); kv++;
                bp->address2 = *(uint16_t *)ssh_ston(kv, RADIX_HEX, &kv); kv++;
                bp->msk = *(uint8_t *)ssh_ston(kv, RADIX_HEX, &kv); kv++;
                bp->val = *(uint8_t *)ssh_ston(kv, RADIX_HEX, &kv); kv++;
                bp->ops = *(uint8_t *)ssh_ston(kv, RADIX_HEX, &kv); kv++;
                bp->flg = *(uint8_t *)ssh_ston(kv, RADIX_HEX, &kv);
            }
            debug("zxGetProp finish");
        }
    }

    long zxInt(JNIEnv*, jclass, jint idx, jint mask, jboolean read, jint value) {
        auto ret = (long)(*(uint32_t*)(opts + idx));
        if(!read) {
            debug("zxIntSave idx: %i value: %i", idx, value);
            ret &= ~mask; ret |= (value & mask);
            *(uint32_t*)(opts + idx) = (uint32_t)ret;
        }
        debug("zxIntRead idx: %i value: %i", idx, ret);
        return (ret & mask);
    }

    jint zxCmd(JNIEnv* env, jclass, jint cmd, jint arg1, jint arg2, jstring arg3) {
        debug("zxCmd cmd: %i", cmd);
        int ret(0);
        switch(cmd) {
            default:                debug("Неизвестная комманда в zxCmd(%i)", cmd); break;
            case ZX_CMD_POKE:       ::wm8(realPtr((uint16_t)arg1), (uint8_t)arg2); break;
            case ZX_CMD_UPDATE_KEY: ALU->updateKeys(arg1, arg2); break;
            case ZX_CMD_PRESETS:    ALU->presets(env->GetStringUTFChars(arg3, nullptr), arg1); break;
            case ZX_CMD_DIAG:       ALU->diag(); break;
            case ZX_CMD_PROPS:      ALU->updateProps(); break;
            case ZX_CMD_MODEL:      ALU->changeModel(opts[ZX_PROP_MODEL_TYPE], *ALU->_MODEL, true); break;
            case ZX_CMD_RESET:      ALU->signalRESET(true); break;
            case ZX_CMD_TRACER:     ALU->tracer(arg1); break;
            case ZX_CMD_QUICK_BP:   ALU->quickBP((uint16_t)arg1); break;
            case ZX_CMD_TRACE_X:    ALU->debugger->trace(arg1); break;
            case ZX_CMD_STEP_DEBUG: ALU->stepDebug(); break;
            case ZX_CMD_MOVE_PC:    ret = ALU->debugger->move(arg1, arg2, 10); break;
            case ZX_CMD_JUMP:       ret = ALU->debugger->jump(arg1, arg2, true); break;
            case ZX_CMD_ASSEMBLER:  ret = ALU->assembler->parser(arg1, env->GetStringUTFChars(arg3, nullptr)); break;
        }
        debug("zxCmd finish");
        return ret;
    }

    jint zxStringToNumber(JNIEnv* env, jclass, jstring value, jint radix) {
        return *(jint*)ssh_ston(env->GetStringUTFChars(value, nullptr), (int)radix);
    }

    jstring zxDebuggerString(JNIEnv* env, jclass, jint cmd, jint data, jint flags) {
        auto ret = ALU->debugger->itemList(cmd, data, flags);
        return env->NewStringUTF(ret);
    }

    jstring zxFormatNumber(JNIEnv* env, jclass, jint value, jint radix, jboolean force) {
        auto ret = ssh_fmtValue(value, radix, force);
        debug("zxFormatNumber value: %i type: %i ret: <%s>", value, radix, ret);
        return env->NewStringUTF(ret);
    }

    jstring zxNumberToString(JNIEnv* env, jclass, jint value, jint radix) {
        auto ret = ssh_ntos(&value, radix);
        debug("zxNumberToString value: %i type: %i ret: <%s>", value, radix, ret);
        return env->NewStringUTF(ret);
    }

    static JNINativeMethod zxMethods[] = {
            { "zxInit", "(Landroid/content/res/AssetManager;Ljava/lang/String;Z)V", (void*)&zxInit },
            { "zxShutdown", "()V", (void*)&zxShutdown },
            { "zxProps", "([B)V", (void*)&zxProps },
            { "zxGetProp", "(Ljava/lang/String;I)V", (void*)&zxGetProp },
            { "zxSetProp", "(I)Ljava/lang/String;", (void*)&zxSetProp },
            { "zxExecute", "()I", (void*)&zxExecute },
            { "zxSurface", "(Landroid/graphics/Bitmap;)V", (void*)&zxSurface },
            { "zxCmd", "(IIILjava/lang/String;)I", (void*)&zxCmd },
            { "zxIO", "(Ljava/lang/String;Z)Z", (void*)&zxIO },
            { "zxPresets", "(I)Ljava/lang/String;", (void*)&zxPresets },
            { "zxInt", "(IIZI)J", (void*)&zxInt },
            { "zxFormatNumber", "(IIZ)Ljava/lang/String;", (void*)&zxFormatNumber },
            { "zxDebuggerString", "(III)Ljava/lang/String;", (void*)&zxDebuggerString },
            { "zxStringToNumber", "(Ljava/lang/String;I)I", (void*)&zxStringToNumber },
            { "zxSaveState", "()[B", (void*)&zxSaveState },
            { "zxLoadState", "([B)V", (void*)&zxLoadState },
            { "zxNumberToString", "(II)Ljava/lang/String;", (void*)&zxNumberToString }
    };

    JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
        JNIEnv *env;
        if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
            info("GetEnv() error!");
            abort();
        }
        auto wnd = env->FindClass("ru/ostrovskal/zx/ZxWnd");
        env->RegisterNatives(wnd, zxMethods, (sizeof(zxMethods) / sizeof(JNINativeMethod)) - 3);

        return JNI_VERSION_1_6;
    }
}
