package ru.ostrovskal.zx

import android.content.Context
import android.content.res.AssetManager
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
import ru.ostrovskal.sshstd.Config
import ru.ostrovskal.sshstd.TileDrawable
import ru.ostrovskal.sshstd.Wnd
import ru.ostrovskal.sshstd.layouts.AbsoluteLayout
import ru.ostrovskal.sshstd.objects.Settings
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.sql.SQL
import ru.ostrovskal.sshstd.ui.UiComponent
import ru.ostrovskal.sshstd.ui.UiCtx
import ru.ostrovskal.sshstd.ui.absoluteLayout
import ru.ostrovskal.sshstd.ui.menuIcon
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.zx.ZxCommon.*
import ru.ostrovskal.zx.forms.ZxFormMain
import java.io.File
import java.io.FileNotFoundException
import java.nio.ByteBuffer
import kotlin.experimental.and
import kotlin.experimental.inv
import kotlin.experimental.or

class ZxWnd : Wnd() {

    enum class ZxMessages {
        // обновление параметров джойстика
        ACT_UPDATE_JOY,
        // обновление кнопок клавиатуры
        ACT_UPDATE_KEY_BUTTONS,
        // обновление отладчика(a1 - параметры)
        ACT_UPDATE_DEBUGGER,
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
        // обновление фильтра
        ACT_UPDATE_FILTER,
        // обновление смены пропуска кадров
        ACT_UPDATE_SKIP_FRAMES,
        // обновление главной разметки
        ACT_UPDATE_MAIN_LAYOUT,
        // длинный клик в списке отладчика
        ACT_DEBUGGER_LONG_CLOCK,
        // выделение элемента в списке отладчика
        ACT_DEBUGGER_SELECT_ITEM,
        // нажатие на кнопку MAGIC
        ACT_PRESS_MAGIC,
        // обновление параметров аудио
        ACT_UPDATE_AUDIO
    }

    // главная разметка
    lateinit var main: AbsoluteLayout

    // меню
    lateinit var menu: Menu

    // признак запуска эмулятора
    var zxInitialize               = false

    // массив установок
    var settings      = arrayOf("")

    // имя файла для авто сохранения
    private var nameAutoSave= ZX_AUTO_SAVE

    companion object {
        init { System.loadLibrary("zx-lib") }

        fun modifyState(add: Byte, del: Byte) {
            val state = props[ZX_CPU_STATE] and del.inv()
            props[ZX_CPU_STATE] = state or add
        }

        val modelNames              = listOf(  R.string.menuKompanion, R.string.menuSinclair48, R.string.menuSinclair48,
                                                        R.string.menuSinclair128, R.string.menuPlus2, R.string.menuPlus3,
                                                        R.string.menuPentagon, R.string.menuScorpion)

        val props                            = ByteArray(ZX_PROPS_COUNT)

        val menuItems               = listOf(   R.integer.MENU_KEYBOARD, R.integer.I_KEY,
                                                R.integer.MENU_IO, R.integer.I_OPEN, R.integer.MENU_SETTINGS, R.integer.I_SETTINGS, R.integer.MENU_PROPS, R.integer.I_PROPS, R.integer.MENU_DISKS, R.integer.I_DISK,
                                                R.integer.MENU_MODEL, R.integer.I_MODEL, R.integer.MENU_RESET, R.integer.I_RESET, R.integer.MENU_RESTORE, R.integer.I_RESTORE, R.integer.MENU_EXIT, R.integer.I_EXIT,
                                                R.integer.MENU_PROPS_SOUND, R.integer.I_SOUND, R.integer.MENU_PROPS_TAPE, R.integer.I_CASSETE, R.integer.MENU_PROPS_FILTER, R.integer.I_FILTER, R.integer.MENU_PROPS_TURBO, R.integer.I_TURBO,
                                                R.integer.MENU_PROPS_EXECUTE, R.integer.I_COMPUTER, R.integer.MENU_PROPS_DEBUGGER, R.integer.I_DEBUGGER, R.integer.MENU_DEBUGGER1, R.integer.I_DEBUGGER,
                                                R.integer.MENU_MRU, R.integer.I_MRU, R.integer.MENU_POKES, R.integer.I_POKES,
                                                R.integer.MENU_DEBUGGER_LABEL, R.integer.I_ADDRESS, R.integer.MENU_DEBUGGER_CODE, R.integer.I_CODE,
                                                R.integer.MENU_DEBUGGER_VALUE, R.integer.I_VALUE)
        @JvmStatic
        external fun zxUpdateAudio(buf: ByteBuffer): Int

        @JvmStatic
        external fun zxInit(asset: AssetManager, savePath: String, error: Boolean)

        @JvmStatic
        external fun zxProps(props: ByteArray, filesDir: String, cacheDir: String)

        @JvmStatic
        external fun zxShutdown()

        @JvmStatic
        external fun zxIO(name: String, load: Boolean): Boolean

        @JvmStatic
        external fun zxCmd(cmd: Int, arg1: Int, arg2: Int, arg3: String): Int

        @JvmStatic
        external fun zxGetProp(value: String, idx: Int)

        @JvmStatic
        external fun zxProgramName(name: String): String

        @JvmStatic
        external fun zxSetProp(idx: Int): String

        @JvmStatic
        external fun zxExecute(): Int

        @JvmStatic
        external fun zxInt(idx: Int, mask: Int, read: Boolean, value: Int): Long

        @JvmStatic
        external fun zxFormatNumber(value: Int, fmt: Int, force: Boolean): String

        @JvmStatic
        external fun zxStringToNumber(value: String, radix: Int): Int

        @JvmStatic
        external fun zxDebuggerString(cmd: Int, data: Int, flags: Int): String

        @JvmStatic
        external fun zxTapeBlock(block: Int, data: ShortArray): String

        fun read16(idx: Int) = ((props[idx].toInt() and 0xff) or ((props[idx + 1].toInt() and 0xff) shl 8))

        fun read8(idx: Int) = (props[idx].toInt() and 0xff)
    }

    override fun onResume() {
        super.onResume()
        findForm<ZxFormMain>("main")?.activeGL(true)
    }

    override fun onPause() {
        findForm<ZxFormMain>("main")?.activeGL(false)
        super.onPause()
    }

    override fun onStop() {
        exit()
        super.onStop()
    }

    override fun restoreState(state: Bundle) {
        nameAutoSave = state.getString("autoSave") ?: ZX_AUTO_SAVE
    }

    override fun saveState(state: Bundle) {
        nameAutoSave = ZX_TMP_SAVE
        state.putString("autoSave", nameAutoSave)
    }

    override fun applyTheme() {
        Theme.setTheme(this, themeDef)
    }

    override fun initialize(restart: Boolean) {
        if (hand == null) {
            // Создаем UI хэндлер
            hand = Handler(Looper.getMainLooper(), this)
            // Инициализируем БД
            if(!SQL.connection(this, false, ZxPreset)) {
                // инициализировать таблицы
                ZxPreset.setDefaults(this)
                //ZxPokes.setDefaults()
            }
            // Инициализируем установки
            settings = loadResource("settings", "array_str", arrayOf(""))
            Settings.initialize(getSharedPreferences(logTag, Context.MODE_PRIVATE), settings)
            val errors = false//"#errors".b and !restart
            if(errors) Settings.default()
            // Применяем тему
            applyTheme()
            // запускаем инициализацию
            runBlocking {
                withContext(Dispatchers.IO) {
                    zxProps(props, folderFiles, folderCache)
                    // восстанавливаем файлы образов дискет
                    repeat(4) { dsk ->
                        val path = "disk$dsk".s
                        if(path.isNotBlank()) {
                            if(zxCmd(ZX_CMD_DISK_OPS, dsk, ZX_DISK_OPS_OPEN, path) == 0)
                                "disk$dsk".s = ""
                        }
                    }
                    // настройки
                    settings.forEachIndexed { i, key -> if (i < ZX_PROPS_INIT_COUNT) zxGetProp(key.substringBeforeLast(',').s, i) }
                    // восстановление предыдущего сеанса
                    zxInit(assets, nameAutoSave, errors)
                }
            }
            ZxPreset.load("")
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_NAME_PROG.ordinal)
            "#errors".b = true
        }
    }

    private fun exit() {
        // Сохраняем состояние
        zxIO(nameAutoSave, false)
        // Сохраняем установки
        settings.forEachIndexed { i, key ->
            if (i < ZX_PROPS_INIT_COUNT) key.substringBeforeLast(',').s = zxSetProp(i)
        }
        zxShutdown()
        "#errors".b = false
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        startLog(this, "ZX", true, BuildConfig.VERSION_CODE, BuildConfig.VERSION_NAME, BuildConfig.DEBUG, enumNames(ZxMessages.values()))
        super.onCreate(savedInstanceState)
        Main().setContent(this, SSH_APP_MODE_GAME or SSH_APP_MODE_TITLE or SSH_APP_MODE_FULLSCREEN)
        // загружаем фрагмент/либо восстанавливаемся после поворота
        if(savedInstanceState == null) instanceForm(FORM_MAIN)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        // установить иконки на элементы меню
        for(idx in 0 until 42 step 2) {
            menu.findItem(menuItems[idx])?.let {
                menuIcon(it, style_zx_toolbar) {
                    tile = resources.getInteger(menuItems[idx + 1])
                    setBounds(0, 0, 40.dp, 40.dp)
                }
            }
        }
        val isDebugger = props[ZX_PROP_SHOW_DEBUGGER].toBoolean
        menu.findItem(R.integer.MENU_DEBUGGER1)?.isVisible = isDebugger
        menu.findItem(R.integer.MENU_KEYBOARD)?.apply { (icon as? TileDrawable)?.tile =
            resources.getInteger(if(props[ZX_PROP_SHOW_KEY].toBoolean) R.integer.I_KEY else R.integer.I_JOY); isVisible = !isDebugger }
        menu.findItem(R.integer.MENU_SETTINGS)?.setShowAsAction(if(Config.isPortrait) SHOW_AS_ACTION_NEVER else SHOW_AS_ACTION_ALWAYS)
        this.menu = menu
        return true
    }

    override fun onMenuItemSelected(featureId: Int, item: MenuItem): Boolean {
        item.subMenu?.apply {
            when (resources.getInteger(item.itemId)) {
                MENU_MODEL      -> getItem(props[ZX_PROP_MODEL_TYPE].toInt()).isChecked = true
                MENU_PROPS      -> repeat(6) { getItem(it).isChecked = if(it == 2) "filter".b else (props[menuProps[it + 2]].toInt() and 1) != 0 }
                MENU_MRU        -> repeat(10) { getItem(it).title = "#mru${it + 1}".s }
                MENU_DEBUGGER1  -> repeat(3) { getItem(it).isChecked = props[menuProps[it + 8]].toBoolean }
            }
        }
        return super.onMenuItemSelected(featureId, item)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when(val id = resources.getInteger(item.itemId)) {
            MENU_POKES                              -> instanceForm(FORM_POKES)
            MENU_IO                                 -> instanceForm(FORM_IO)
            MENU_SETTINGS                           -> instanceForm(FORM_OPTIONS)
            MENU_DISKS                              -> instanceForm(FORM_DISK)
            MENU_RESTORE                            -> hand?.send(RECEPIENT_FORM, ZxMessages.ACT_IO_LOAD.ordinal, o = ZX_AUTO_SAVE)
            MENU_RESET                              -> hand?.send(RECEPIENT_FORM, ZxMessages.ACT_RESET.ordinal)
            MENU_EXIT                               -> { nameAutoSave = ZX_AUTO_SAVE; finish() }
            MENU_MAGIC                              -> hand?.send(RECEPIENT_FORM, ZxMessages.ACT_PRESS_MAGIC.ordinal)
            MENU_SHOW_DEBUGGER                      -> {
                val dbg = !props[ZX_PROP_SHOW_DEBUGGER].toBoolean
                props[ZX_PROP_SHOW_DEBUGGER] = dbg.toByte
                modifyState(if(props[ZX_PROP_ACTIVE_DEBUGGING].toBoolean) ZX_DEBUGGER else 0, ZX_DEBUGGER)
                hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_DEBUGGER.ordinal, a1 = read16(ZX_CPU_PC), a2 = ZX_ALL, o = dbg.toString())
                if(!dbg) {
                    props[ZX_PROP_EXECUTE] = 1
                    // закрываем отладчик
                    hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_NAME_PROG.ordinal)
                }
            }
            MENU_DEBUGGER_LABEL, MENU_DEBUGGER_CODE,
            MENU_DEBUGGER_VALUE, MENU_PROPS_KEYBOARD,
            MENU_PROPS_SOUND, MENU_PROPS_TURBO,
            MENU_PROPS_DEBUGGER, MENU_PROPS_TAPE,
            MENU_PROPS_EXECUTE                      -> updatePropsMenuItem(item)
            MENU_MODEL_48KK, MENU_MODEL_48KS,
            MENU_MODEL_48KSN, MENU_MODEL_128K,
            MENU_MODEL_PENTAGON, MENU_MODEL_SCORPION,
            MENU_MODEL_PLUS2, MENU_MODEL_PLUS3      -> {
                props[ZX_PROP_MODEL_TYPE] = (id - MENU_MODEL_48KK).toByte()
                hand?.send(RECEPIENT_FORM, ZxMessages.ACT_MODEL.ordinal)
            }
            MENU_PROPS_FILTER                    -> {
                "filter".b = !"filter".b
                hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_FILTER.ordinal)
            }
            MENU_MRU_1, MENU_MRU_2,
            MENU_MRU_3, MENU_MRU_4,
            MENU_MRU_5, MENU_MRU_6,
            MENU_MRU_7, MENU_MRU_8,
            MENU_MRU_9, MENU_MRU_10             -> openMRU(id, item.title.toString(), false)
        }
        return super.onOptionsItemSelected(item)
    }

    private fun updatePropsMenuItem(item: MenuItem): Boolean {
        var id = resources.getInteger(item.itemId)
        val drawable = (item.icon as? TileDrawable)
        val tile = drawable?.tile ?: -1

        if(id == MENU_PROPS_KEYBOARD) {
            if(tile != 81) id = MENU_PROPS_JOYSTICK
        }
        val prop = menuProps[id - MENU_PROPS_KEYBOARD]
        val isChecked = !props[prop].toBoolean

        if(id == MENU_PROPS_KEYBOARD || id == MENU_PROPS_JOYSTICK) {
            props[ZX_PROP_SHOW_KEY] = 0.toByte()
            props[ZX_PROP_SHOW_JOY] = 0.toByte()
            drawable?.tile = when(tile) { 81 -> 46; 46 -> 40; else -> 81 }
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_JOY.ordinal, 50)
        }
        if(id == MENU_PROPS_DEBUGGER) modifyState(if(isChecked) ZX_DEBUGGER else 0, ZX_DEBUGGER)
        props[prop] = isChecked.toByte
        zxCmd(ZX_CMD_PROPS, 0, 0, "")
        if(id == MENU_PROPS_KEYBOARD || id == MENU_PROPS_JOYSTICK) {
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal)
        }
        if(id == MENU_DEBUGGER_LABEL || id == MENU_DEBUGGER_CODE || id == MENU_DEBUGGER_VALUE)
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_DEBUGGER.ordinal, a1 = 0, a2 = ZX_RL)
        // остановка/запуск эмуляции
        if(id == MENU_PROPS_EXECUTE)
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_NAME_PROG.ordinal)
        return isChecked
    }

    fun openMRU(id: Int, title: String, error: Boolean) {
        val idx = (id - MENU_MRU_1) + 1
        var pos = idx
        try {
            if(error || !File(folderFiles + title).isFile) throw FileNotFoundException()
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
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_IO_LOAD.ordinal, o = title)
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
