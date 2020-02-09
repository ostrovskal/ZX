
#include "zxCommon.h"
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>

static jobject objProps = nullptr;

static void copyAssetsFile(AAssetManager *aMgr, const char *aPath, const char *path, uint8_t **buffer = nullptr) {
    auto aFile = AAssetManager_open(aMgr, aPath, AASSET_MODE_UNKNOWN);
    bool result = aFile != nullptr;
    if(result) {
        auto aLen = (size_t) AAsset_getLength(aFile);
        auto aBuf = new uint8_t[aLen];
        AAsset_read(aFile, aBuf, aLen);
        AAsset_close(aFile);
        if(!buffer) {
            zxFile::writeFile(path, aBuf, aLen);
            SAFE_A_DELETE(aBuf);
        } else *buffer = aBuf;
    } else {
        LOG_DEBUG("Не удалось найти файл <%s>!", aPath);
    }
}

static long tme = 0;
static int turbo_delay = 0;

extern "C" {

    int zxExecute(JNIEnv*, jclass) {
        ULA->execute();
        if(opts[ZX_PROP_TURBO_MODE]) {
            turbo_delay++;
            if(turbo_delay & 1) ULA->execute();
        }
        auto tms = currentTimeMillis();
        auto tm = tms - tme;
        if(tm < 20) {
            auto t = 20 - tm;
            usleep((uint32_t)(t * 1000));
            tms += t;
        }
        tme = tms;
        if(checkSTATE(ZX_BP)) {
            *ULA->_STATE &= ~ZX_BP;
            return 2;
        }
        return ULA->updateKeys(0, 0);
    }

    void zxShutdown(JNIEnv *env, jobject) {
        LOG_DEBUG("", nullptr)
        if(objProps) {
            env->DeleteGlobalRef(objProps);
            objProps = nullptr;
        }
        // Uninit GL
        ULA->gpu->uninitGL();
        ULA->cpu->shutdown();
    }

    jboolean zxIO(JNIEnv* env, jclass, jstring nm, jboolean load) {
        auto path = env->GetStringUTFChars(nm, nullptr);
        LOG_DEBUG("%s \"%s\"", load ? "load" : "save", path);
        auto type = parseExtension(path);
        auto ret = (jboolean)(load ? ULA->load(path, type) : ULA->save(path, type));
        if(!ret) {
            LOG_DEBUG("Не удалось загрузить/записать <%s>!", path)
        }
        else if(type > ZX_CMD_IO_STATE) {
            ULA->programName(path);
        }
        return ret;
    }

    void zxInit(JNIEnv *env, jobject, jobject asset, jstring savePath, jboolean error) {
        auto autoSavePath = env->GetStringUTFChars(savePath, nullptr);
        LOG_DEBUG("savePath: %s error: %i", autoSavePath, error);

        auto amgr = AAssetManager_fromJava(env, asset);
        if(opts[ZX_PROP_FIRST_LAUNCH]) {
            // перенести все игры из активов на диск
            std::string out("games/");
            std::string z80("Z80/");
            std::string tap("TAP/");
            std::string trdos("TRDOS/");
            std::string path("");
            zxFile::makeDir(zxFile::makePath("SAVERS", true).c_str());
            zxFile::makeDir(zxFile::makePath("Z80", true).c_str());
            zxFile::makeDir(zxFile::makePath("TAP", true).c_str());
            zxFile::makeDir(zxFile::makePath("TRDOS", true).c_str());
            auto dir = AAssetManager_openDir(amgr, "games");
            const char* file;
            while ((file = AAssetDir_getNextFileName(dir))) {
                switch(parseExtension(file)) {
                    case ZX_CMD_IO_Z80:  path    = z80 + file; break;
                    case ZX_CMD_IO_TAPE: path    = tap + file; break;
                    case ZX_CMD_IO_TRD:
                    case ZX_CMD_IO_SCL:
                    case ZX_CMD_IO_FDI:  path    = trdos + file; break;
                    default: path = file; break;
                }
                copyAssetsFile(amgr, (out + file).c_str(), path.c_str());
            }
            AAssetDir_close(dir);
            opts[ZX_PROP_FIRST_LAUNCH] = 0;
            copyAssetsFile(amgr, "tapLoad128.zx", "tapLoad128.zx");
            copyAssetsFile(amgr, "tapLoad48.zx", "tapLoad48.zx");
            copyAssetsFile(amgr, "trdosLoad128.zx", "trdosLoad128.zx");
            copyAssetsFile(amgr, "trdosLoad48.zx", "trdosLoad48.zx");
            copyAssetsFile(amgr, "rom.zx", "rom.zx");
        }
        copyAssetsFile(amgr, "labels.bin", nullptr, &labels);
        ULA->changeModel(opts[ZX_PROP_MODEL_TYPE], true);
        if(!error) ULA->load(autoSavePath, ZX_CMD_IO_STATE);
    }

    void zxProps(JNIEnv* env, jclass, jbyteArray props, jstring filesDir, jstring cacheDir) {
        LOG_DEBUG("", nullptr)
        if(!(objProps = env->NewGlobalRef(props))) {
            LOG_INFO("NewGlobalRef(props) error!", nullptr);
            abort();
        }
        opts = (uint8_t*) env->GetPrimitiveArrayCritical(props, nullptr);
        bps = (BREAK_POINT*)&opts[258];
        env->ReleasePrimitiveArrayCritical((jarray)props, opts, JNI_ABORT);
        ssh_memzero(opts, ZX_PROPS_COUNT);
        // инициализировать пути к системным папкам
        FOLDER_FILES = env->GetStringUTFChars(filesDir, nullptr);
        FOLDER_CACHE = env->GetStringUTFChars(cacheDir, nullptr);
        ULA = new zxULA();
        LOG_DEBUG("filesDir: %s cacheDir: %s", FOLDER_FILES.c_str(), FOLDER_CACHE.c_str());
    }

    jstring zxSetProp(JNIEnv* env, jclass, jint idx) {
        char* ret = nullptr;
        idx += ZX_PROP_FIRST_LAUNCH;
        uint32_t n = opts[idx];
        if(idx < ZX_PROP_BORDER_SIZE) ret = ssh_ntos(&n, RADIX_BOL);
        else if(idx < ZX_PROP_COLORS) ret = ssh_ntos(&n, RADIX_DEC);
        else if(idx < (ZX_PROP_COLORS + 22)) {
            n = *(uint32_t*)(opts + (ZX_PROP_COLORS + (idx - ZX_PROP_COLORS) * 4));
            ret = ssh_ntos(&n, RADIX_HEX);
        } else if(idx < (ZX_PROP_BPS + 8)) {
            auto bp = &bps[idx - ZX_PROP_BPS];
            auto tmp = ret = (char*)TMP_BUF;
            n = bp->address1; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->address2; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->msk; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->val; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->ops; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX)); *tmp++ = '#';
            n = bp->flg; ssh_strcpy(&tmp, ssh_ntos(&n, RADIX_HEX));
        }
        return env->NewStringUTF(ret);
    }

    void zxGetProp(JNIEnv* env, jclass, jstring key, jint idx) {
        auto kv = env->GetStringUTFChars(key, nullptr);
        if(kv) {
            idx += ZX_PROP_FIRST_LAUNCH;
            if (idx < ZX_PROP_BORDER_SIZE) opts[idx] = *(uint8_t *) ssh_ston(kv, RADIX_BOL);
            else if (idx < ZX_PROP_COLORS) opts[idx] = *(uint8_t *) ssh_ston(kv, RADIX_DEC);
            else if (idx < (ZX_PROP_COLORS + 22))
                *(uint32_t *) (opts + (ZX_PROP_COLORS + (idx - ZX_PROP_COLORS) * 4)) = *(uint32_t *) ssh_ston(kv, RADIX_HEX);
            else if(idx < (ZX_PROP_BPS + 8)) {
                auto bp = &bps[idx - ZX_PROP_BPS];
                bp->address1 = *(uint16_t *)ssh_ston(kv, RADIX_HEX, (char**)&kv); kv++;
                bp->address2 = *(uint16_t *)ssh_ston(kv, RADIX_HEX, (char**)&kv); kv++;
                bp->msk = *(uint8_t *)ssh_ston(kv, RADIX_HEX, (char**)&kv); kv++;
                bp->val = *(uint8_t *)ssh_ston(kv, RADIX_HEX, (char**)&kv); kv++;
                bp->ops = *(uint8_t *)ssh_ston(kv, RADIX_HEX, (char**)&kv); kv++;
                bp->flg = *(uint8_t *)ssh_ston(kv, RADIX_HEX, (char**)&kv);
            }
        }
    }

    long zxInt(JNIEnv*, jclass, jint idx, jint mask, jboolean read, jint value) {
        auto ret = (long)(*(uint32_t*)(opts + idx));
        if(!read) {
            ret &= ~mask; ret |= (value & mask);
            *(uint32_t*)(opts + idx) = (uint32_t)ret;
        }
        return (ret & mask);
    }

    jstring zxProgramName(JNIEnv* env, jclass, jstring name) {
        return env->NewStringUTF(ULA->programName(env->GetStringUTFChars(name, nullptr)));
    }

    jint zxCmd(JNIEnv* env, jclass, jint cmd, jint arg1, jint arg2, jstring arg3) {
        int ret(0);
        switch(cmd) {
            case ZX_CMD_POKE:      ::wm8(realPtr((uint16_t)arg1), (uint8_t)arg2); break;
            case ZX_CMD_UPDATE_KEY:ret = ULA->updateKeys(arg1, arg2); break;
            case ZX_CMD_PROPS:     ULA->updateProps(arg1); break;
            case ZX_CMD_MODEL:     ULA->changeModel(opts[ZX_PROP_MODEL_TYPE], true); break;
            case ZX_CMD_RESET:     ULA->signalRESET(true); break;
            case ZX_CMD_QUICK_BP:  ULA->quickBP((uint16_t)arg1); break;
            case ZX_CMD_TRACE_X:   ULA->debugger->trace(arg1); break;
            case ZX_CMD_STEP_DEBUG:ULA->stepDebug(); break;
            case ZX_CMD_MOVE_PC:   ret = ULA->debugger->move(arg1, arg2, 10); break;
            case ZX_CMD_JUMP:      ret = ULA->debugger->jump((uint16_t)arg1, arg2, true); break;
            case ZX_CMD_ASSEMBLER: ret = ULA->assembler->parser(arg1, env->GetStringUTFChars(arg3, nullptr)); break;
            case ZX_CMD_INIT_GL:   ULA->gpu->initGL(); break;
            case ZX_CMD_TAPE_COUNT:ret = ULA->tape->countBlocks; break;
            case ZX_CMD_MAGIC:     ULA->cpu->signalNMI(); break;
            case ZX_CMD_DISK_OPS:  ret = ULA->diskOperation(arg1, arg2, env->GetStringUTFChars(arg3, nullptr)); break;
            case ZX_CMD_QUICK_SAVE:ULA->quickSave(); break;
            case ZX_CMD_VALUE_REG: ret = ULA->getAddressCpuReg(env->GetStringUTFChars(arg3, nullptr)); break;
        }
        return ret;
    }

    jint zxUpdateAudio(JNIEnv* env, jobject, jobject byte_buffer) {
        auto buf = (uint8_t *)env->GetDirectBufferAddress(byte_buffer);
        return ULA->snd->update(buf);
    }

    jint zxStringToNumber(JNIEnv* env, jclass, jstring value, jint radix) {
        return *(jint*)ssh_ston(env->GetStringUTFChars(value, nullptr), (int)radix);
    }

    jstring zxDebuggerString(JNIEnv* env, jclass, jint cmd, jint data, jint flags) {
        return env->NewStringUTF(ULA->debugger->itemList(cmd, data, flags));
    }

    jstring zxTapeBlock(JNIEnv* env, jclass, jint idx, jshortArray data) {
        auto arr = (uint16_t *) env->GetPrimitiveArrayCritical(data, nullptr);
        auto name = ULA->tape->getBlockData(idx, arr);
        env->ReleasePrimitiveArrayCritical((jarray)data, arr, JNI_ABORT);
        return env->NewStringUTF(name);
    }

    jstring zxFormatNumber(JNIEnv* env, jclass, jint value, jint radix, jboolean force) {
        return env->NewStringUTF(ssh_fmtValue(value, radix, force));
    }

    static JNINativeMethod zxMethods[] = {
            { "zxInit", "(Landroid/content/res/AssetManager;Ljava/lang/String;Z)V", (void*)&zxInit },
            { "zxShutdown", "()V", (void*)&zxShutdown },
            { "zxProps", "([BLjava/lang/String;Ljava/lang/String;)V", (void*)&zxProps },
            { "zxGetProp", "(Ljava/lang/String;I)V", (void*)&zxGetProp },
            { "zxSetProp", "(I)Ljava/lang/String;", (void*)&zxSetProp },
            { "zxExecute", "()I", (void*)&zxExecute },
            { "zxCmd", "(IIILjava/lang/String;)I", (void*)&zxCmd },
            { "zxIO", "(Ljava/lang/String;Z)Z", (void*)&zxIO },
            { "zxInt", "(IIZI)J", (void*)&zxInt },
            { "zxProgramName", "(Ljava/lang/String;)Ljava/lang/String;", (void*)&zxProgramName },
            { "zxFormatNumber", "(IIZ)Ljava/lang/String;", (void*)&zxFormatNumber },
            { "zxDebuggerString", "(III)Ljava/lang/String;", (void*)&zxDebuggerString },
            { "zxStringToNumber", "(Ljava/lang/String;I)I", (void*)&zxStringToNumber },
            { "zxTapeBlock", "(I[S)Ljava/lang/String;", (void*)&zxTapeBlock },
            { "zxUpdateAudio", "(Ljava/nio/ByteBuffer;)I", (void*)&zxUpdateAudio }
    };

    JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
        JNIEnv *env;
        if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
            LOG_INFO("GetEnv() error!", nullptr);
            abort();
        }
        auto wnd = env->FindClass("ru/ostrovskal/zx/ZxWnd");
        env->RegisterNatives(wnd, zxMethods, (sizeof(zxMethods) / sizeof(JNINativeMethod)));

        TMP_BUF = new uint8_t[ZX_SIZE_TMP_BUF];

        return JNI_VERSION_1_6;
    }
}
