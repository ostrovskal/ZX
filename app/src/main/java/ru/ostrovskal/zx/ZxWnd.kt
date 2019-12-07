package ru.ostrovskal.zx

import android.content.Context
import android.content.res.AssetManager
import android.graphics.Bitmap
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Menu
import android.view.MenuItem
import android.view.MenuItem.SHOW_AS_ACTION_ALWAYS
import android.view.MenuItem.SHOW_AS_ACTION_NEVER
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import ru.ostrovskal.sshstd.Common.*
import ru.ostrovskal.sshstd.TileDrawable
import ru.ostrovskal.sshstd.Wnd
import ru.ostrovskal.sshstd.layouts.AbsoluteLayout
import ru.ostrovskal.sshstd.objects.Settings
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.ui.UiComponent
import ru.ostrovskal.sshstd.ui.UiCtx
import ru.ostrovskal.sshstd.ui.absoluteLayout
import ru.ostrovskal.sshstd.ui.menuIcon
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.zx.ZxCommon.*
import java.io.FileNotFoundException

class ZxWnd : Wnd() {

    enum class ZxMessages {
        // обновление параметров джойстика
        ACT_UPDATE_JOY,
        // обновление кнопок клавиатуры
        ACT_UPDATE_KEY_BUTTONS,
        // обновление фильтра отображения
        ACT_UPDATE_FILTER,
        // завершнение скачивания файлов из облака
        ACT_DROPBOX_DOWNLOAD_FINISH,
        // завершение запроса на получение списка файлов из облака
        ACT_DROPBOX_LIST_FILES_FINISH,
        // обновление имени текущей программы
        ACT_UPDATE_NAME_PROG,
        // запрос на загрузку
        ACT_IO_LOAD,
        // запрос на сохранение
        ACT_IO_SAVE,
        // запрос на сброс
        ACT_RESET,
        // запрос на смену модели памяти
        ACT_MODEL,
        // отображение диалога об ошибке
        ACT_IO_ERROR,
        // обновление поверхности
        ACT_UPDATE_SURFACE,
        // обновление смены пропуска кадров
        ACT_UPDATE_SKIP_FRAMES,
        // обновление главной разметки
        ACT_UPDATE_MAIN_LAYOUT,
        // обновление отладчика(a1 - параметры)
        ACT_UPDATE_DEBUGGER
    }

    // главная разметка
    lateinit var main: AbsoluteLayout

    // признак запуска эмулятора
    var zxInitialize            = false

    companion object {
        init {
            System.loadLibrary("zx-lib")
        }

        val menuProps               = listOf(ZX_PROP_SHOW_KEY, ZX_PROP_SHOW_JOY, ZX_PROP_SND_LAUNCH, ZX_PROP_TRAP_TAPE, 0,
                                            ZX_PROP_TURBO_MODE, ZX_PROP_EXECUTE, ZX_PROP_SHOW_DEBUGGER, ZX_PROP_SHOW_HEX)

        val modelNames              = listOf(R.string.menu48kk, R.string.menu48k, R.string.menu128k, R.string.menuPentagon, R.string.menuScorpion)

        val props                   = ByteArray(ZX_PROPS_COUNT)

        val menuItems               = listOf(   R.integer.MENU_KEYBOARD, R.integer.I_KEY, R.integer.MENU_CLOUD, R.integer.I_CLOUD,
                                                R.integer.MENU_IO, R.integer.I_OPEN, R.integer.MENU_SETTINGS, R.integer.I_SETTINGS,
                                                R.integer.MENU_PROPS, R.integer.I_PROPS, R.integer.MENU_DISKS, R.integer.I_DISKETTE,
                                                R.integer.MENU_MODEL, R.integer.I_MODEL, R.integer.MENU_RESET, R.integer.I_RESET,
                                                R.integer.MENU_RESTORE, R.integer.I_RESTORE, R.integer.MENU_EXIT, R.integer.I_EXIT,
                                                R.integer.MENU_PROPS_SOUND, R.integer.I_SOUND, R.integer.MENU_PROPS_TAPE, R.integer.I_CASSETE,
                                                R.integer.MENU_PROPS_FILTER, R.integer.I_FILTER, R.integer.MENU_PROPS_TURBO, R.integer.I_TURBO,
                                                R.integer.MENU_PROPS_EXECUTE, R.integer.I_COMPUTER, R.integer.MENU_PROPS_DEBUGGER, R.integer.I_DEBUGGER,
                                                R.integer.MENU_PROPS_HEX_DEC, R.integer.I_HEX)
        @JvmStatic
        external fun zxInit(asset: AssetManager, errors: Boolean)

        @JvmStatic
        external fun zxProps(props: ByteArray)

        @JvmStatic
        external fun zxShutdown()

        @JvmStatic
        external fun zxIO(name: String, load: Boolean): Boolean

        @JvmStatic
        external fun zxCmd(cmd: Int, arg1: Int, arg2: Int, arg3: String): Int

        @JvmStatic
        external fun zxGetProp(value: String, idx: Int)

        @JvmStatic
        external fun zxSetProp(idx: Int): String

        @JvmStatic
        external fun zxExecute(): Int

        @JvmStatic
        external fun zxSurface(bmp: Bitmap)

        @JvmStatic
        external fun zxInt(idx: Int, read: Boolean, value: Int): Long

        @JvmStatic
        external fun zxPresets(cmd: Int): String

/*
        @JvmStatic
        external fun zxSaveState(): ByteArray

        @JvmStatic
        external fun zxLoadState(mem: ByteArray)

        @JvmStatic
        external fun zxStringToNumber(value: String, radix: Int): Int

        @JvmStatic
        external fun zxNumberToString(value: Int, radix: Int): String
*/
    }

    override fun onStop() {
        exit()
        super.onStop()
    }

    override fun applyTheme() {
        Theme.setTheme(this, themeDef)
    }

    override fun initialize(restart: Boolean) {
        if (hand == null) {
            // Создаем UI хэндлер
            hand = Handler(Looper.getMainLooper(), this)
            // Инициализируем установки
            val settings = loadResource("settings", "array_str", arrayOf(""))
            Settings.initialize(getSharedPreferences(logTag, Context.MODE_PRIVATE), settings)
            val errors = if(restart) false else "#errors".b
            if(errors) Settings.default()
            // Применяем тему
            applyTheme()
            // Инициализация эмулятора
            runBlocking {
                withContext(Dispatchers.IO) {
                    zxProps(props)
                    settings.forEachIndexed { i, key -> if (i < ZX_PROPS_INIT_COUNT) zxGetProp(key.substringBeforeLast(',').s, i) }
                    zxInit(assets, errors)
                    if(restart) hand?.send(RECEPIENT_SURFACE_UI, ZxMessages.ACT_UPDATE_SURFACE.ordinal, a1 = 1)
                }
            }
            "#errors".b = true
        }
    }

    private fun exit() {
        // Сохраняем состояние
        zxIO(ZX_AUTO_SAVE, false)
        // Сохраняем установки
        loadResource("settings", "array_str", arrayOf("")).forEachIndexed { i, key ->
            if (i < ZX_PROPS_INIT_COUNT) key.substringBeforeLast(',').s = zxSetProp(i)
        }
        zxShutdown()
        "#errors".b = false
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        startLog(this, "ZX", true, BuildConfig.VERSION_CODE, BuildConfig.VERSION_NAME, BuildConfig.DEBUG, enumNames(ZxMessages.values()))
        super.onCreate(savedInstanceState)
        Main().setContent(this, SSH_APP_MODE_GAME or SSH_APP_MODE_TITLE or SSH_APP_MODE_FULLSCREEN)
        // загружаем фрагмент
        if(savedInstanceState == null) instanceForm(FORM_MAIN)
    }

    override fun onMenuItemSelected(featureId: Int, item: MenuItem): Boolean {
        item.subMenu.apply {
            when (item.itemId) {
                R.integer.MENU_MODEL  -> getItem(props[ZX_PROP_MODEL_TYPE].toInt()).isChecked = true
                R.integer.MENU_DISKS  -> getItem(props[ZX_PROP_ACTIVE_DISK].toInt()).isChecked = true
                R.integer.MENU_PROPS  -> repeat(7) { getItem(it).isChecked = if(it == 2) "filter".b else props[menuProps[it + 2]].toBoolean }
                R.integer.MENU_MRU    -> repeat(10) { getItem(it).title = "#mru${it + 1}".s }
            }
        }
        return super.onMenuItemSelected(featureId, item)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        // установить иконки на элементы меню
        for(idx in 0 until 34 step 2) {
            menu.findItem(menuItems[idx])?.let {
                menuIcon(it, style_zx_toolbar) {
                    tile = resources.getInteger(menuItems[idx + 1])
                    //resolveTile(tile, bounds)
                    setBounds(0, 0, 40, 40)
                }
            }
        }
        menu.findItem(R.integer.MENU_KEYBOARD)?.apply { (icon as? TileDrawable)?.tile =
            resources.getInteger(if(props[ZX_PROP_SHOW_KEY].toBoolean) R.integer.I_KEY else R.integer.I_JOY) }
        menu.findItem(R.integer.MENU_PROPS_HEX_DEC)?.apply { (icon as? TileDrawable)?.tile =
            resources.getInteger(if(props[ZX_PROP_SHOW_HEX].toBoolean) R.integer.I_HEX else R.integer.I_DEC) }
        menu.findItem(R.integer.MENU_IO)?.setShowAsAction(if(config.portrait) SHOW_AS_ACTION_NEVER else SHOW_AS_ACTION_ALWAYS)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when(val id = resources.getInteger(item.itemId)) {
            MENU_POKES                              -> instanceForm(FORM_POKES)
            MENU_CLOUD                              -> instanceForm(FORM_LOADING)
            MENU_IO                                 -> instanceForm(FORM_IO, "filter", ".z80,.tap,.tga")
            MENU_SETTINGS                           -> instanceForm(FORM_OPTIONS)
            MENU_RESTORE                            -> hand?.send(RECEPIENT_SURFACE_BG, ZxMessages.ACT_IO_LOAD.ordinal, o = ZX_AUTO_SAVE)
            MENU_RESET                              -> hand?.send(RECEPIENT_SURFACE_BG, ZxMessages.ACT_RESET.ordinal)
            MENU_EXIT                               -> finish()
            MENU_PROPS_DEBUGGER,
            MENU_PROPS_KEYBOARD, MENU_PROPS_SOUND,
            MENU_PROPS_TURBO, MENU_PROPS_HEX_DEC,
            MENU_PROPS_TAPE, MENU_PROPS_EXECUTE   -> updatePropsMenuItem(item)
            MENU_DISK_A, MENU_DISK_B,
            MENU_DISK_C, MENU_DISK_D              -> {
                props[ZX_PROP_ACTIVE_DISK] = (id - MENU_DISK_A).toByte()
                instanceForm(FORM_IO, "filter", ".trd")
            }
            MENU_MODEL_48KK, MENU_MODEL_48KS,
            MENU_MODEL_128K, MENU_MODEL_PENTAGON,
            MENU_MODEL_SCORPION                  -> {
                props[ZX_PROP_MODEL_TYPE] = (id - MENU_MODEL_48KK).toByte()
                hand?.send(RECEPIENT_SURFACE_BG, ZxMessages.ACT_MODEL.ordinal)
            }
            MENU_PROPS_FILTER                    -> {
                "filter".b = !"filter".b
                hand?.send(RECEPIENT_SURFACE_UI, ZxMessages.ACT_UPDATE_FILTER.ordinal)
            }
            MENU_MRU_1, MENU_MRU_2,
            MENU_MRU_3, MENU_MRU_4,
            MENU_MRU_5, MENU_MRU_6,
            MENU_MRU_7, MENU_MRU_8,
            MENU_MRU_9, MENU_MRU_10             -> openMRU(id, item.title.toString(), false)
        }
        return super.onOptionsItemSelected(item)
    }

    private fun updatePropsMenuItem(item: MenuItem) {
        var id = resources.getInteger(item.itemId)
        val drawable = (item.icon as? TileDrawable)
        val tile = drawable?.tile ?: -1

        if(id == MENU_PROPS_KEYBOARD) {
            if(tile == 38) id = MENU_PROPS_JOYSTICK
        }
        val prop = menuProps[id - MENU_PROPS_KEYBOARD]
        val isChecked = !props[prop].toBoolean

        if(id == MENU_PROPS_DEBUGGER)
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal)

        if(id == MENU_PROPS_HEX_DEC) {
            drawable?.tile = if(isChecked) 43 else 44
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_DEBUGGER.ordinal, 255)
        }
        if(id == MENU_PROPS_KEYBOARD || id == MENU_PROPS_JOYSTICK) {
            props[ZX_PROP_SHOW_KEY] = 0.toByte()
            props[ZX_PROP_SHOW_JOY] = 0.toByte()
            drawable?.tile = if(tile == 38) 32 else 38
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal)
            hand?.send(RECEPIENT_SURFACE_UI, ZxMessages.ACT_UPDATE_JOY.ordinal)
        }
        props[prop] = isChecked.toByte
        zxCmd(ZX_CMD_PROPS, 0, 0, "")
    }

    fun openMRU(id: Int, title: String, error: Boolean) {
        val idx = (id - MENU_MRU_1) + 1
        var pos = idx
        try {
            if(error) throw FileNotFoundException()
            makeDirectories("", FOLDER_FILES, title).readBytes()
            // найти такую же строку
            repeat(9) {
                val i = it + 1
                if("#mru$i".s == title) {
                    pos = i
                    return@repeat
                }
            }
            // переставить
            repeat(pos - 1) {
                val i = pos - it
                "#mru$i".s = "#mru${i - 1}".s
            }
            "#mru1".s = title
            hand?.send(RECEPIENT_SURFACE_BG, ZxMessages.ACT_IO_LOAD.ordinal, o = title)
        } catch (e: FileNotFoundException) {
            "Файл не найден - <$title>!".debug()
            repeat(10 - pos) {
                val i = pos + it
                "#mru$i".s = "#mru${i + 1}".s
            }
            "#mru10".s = getString(R.string.menuMruEmpty)
        }
    }

    inner class Main : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) {
            main = absoluteLayout { this.id = R.id.main }
            main
        }
    }
}
