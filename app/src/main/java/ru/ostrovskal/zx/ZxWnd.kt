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
        ACT_UPDATE_MAIN_LAYOUT
    }

    // главная разметка
    lateinit var main: AbsoluteLayout

    // признак запуска эмулятора
    var zxInitialize            = false

    companion object {
        init {
            System.loadLibrary("zx-lib")
        }

        private val menuProps       = listOf(ZX_PROP_SHOW_KEY, ZX_PROP_SHOW_JOY, ZX_PROP_SND_LAUNCH, ZX_PROP_TRAP_TAPE, 0, ZX_PROP_TURBO_MODE, ZX_PROP_EXECUTE, ZX_PROP_SHOW_DEBUGGER)

        val modelNames              = listOf(R.string.menu48kk, R.string.menu48k, R.string.menu128k, R.string.menuPentagon, R.string.menuScorpion)

        val props                   = ByteArray(ZX_PROPS_COUNT)

        val menuItems               = listOf(   R.integer.MENU_KEYBOARD, R.integer.I_KEY, R.integer.MENU_CLOUD, R.integer.I_CLOUD,
                                                R.integer.MENU_IO, R.integer.I_OPEN, R.integer.MENU_SETTINGS, R.integer.I_SETTINGS,
                                                R.integer.MENU_PROPS, R.integer.I_PROPS, R.integer.MENU_DISKS, R.integer.I_DISKETTE,
                                                R.integer.MENU_MODEL, R.integer.I_MODEL, R.integer.MENU_RESET, R.integer.I_RESET,
                                                R.integer.MENU_RESTORE, R.integer.I_RESTORE, R.integer.MENU_EXIT, R.integer.I_EXIT,
                                                R.integer.MENU_PROPS_SOUND, R.integer.I_SOUND, R.integer.MENU_PROPS_TAPE, R.integer.I_CASSETE,
                                                R.integer.MENU_PROPS_FILTER, R.integer.I_FILTER, R.integer.MENU_PROPS_TURBO, R.integer.I_TURBO,
                                                R.integer.MENU_PROPS_EXECUTE, R.integer.I_COMPUTER, R.integer.MENU_PROPS_DEBUGGER, R.integer.I_DEBUGGER)
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

        @JvmStatic
        external fun zxSaveState(): ByteArray

        @JvmStatic
        external fun zxLoadState(mem: ByteArray)

        @JvmStatic
        external fun zxStringToNumber(value: String, radix: Int): Int

        @JvmStatic
        external fun zxNumberToString(value: Int, radix: Int): String
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
                    if(restart) hand?.send(RECEPIENT_SURFACE_UI, ZxMessages.ACT_UPDATE_SURFACE.ordinal)
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
            when (resources.getInteger(item.itemId)) {
                MENU_MODEL  -> getItem(props[ZX_PROP_MODEL_TYPE].toInt()).isChecked = true
                MENU_DISKS  -> getItem(props[ZX_PROP_ACTIVE_DISK].toInt()).isChecked = true
                MENU_PROPS  -> repeat(6) { getItem(it).isChecked = if(it == 2) "filter".b else props[menuProps[it + 2]].toBoolean }
                MENU_MRU    -> repeat(10) { getItem(it).title = "#mru${it + 1}".s }
            }
        }
        return super.onMenuItemSelected(featureId, item)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        // установить иконки на элементы меню
        for(idx in 0 until 32 step 2) {
            menu.findItem(menuItems[idx])?.let {
                menuIcon(it, style_zx_toolbar) {
                    val idTile = if (idx == 0) { if(props[ZX_PROP_SHOW_KEY].toBoolean) R.integer.I_KEY else R.integer.I_JOY } else menuItems[idx + 1]
                    tile = resources.getInteger(idTile)
                    //resolveTile(tile, bounds)
                    setBounds(0, 0, 40, 40)
                }
            }
        }
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
            MENU_PROPS_DEBUGGER                     -> {
                hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal, 200)
                updatePropsMenuItem(id)
            }
            MENU_EXIT                               -> finish()
            MENU_PROPS_KEYBOARD                     -> {
                (item.icon as? TileDrawable)?.apply {
                    val isKey = tile == 38
                    tile = if(isKey) 32 else 38
                    updatePropsMenuItem(if(isKey) MENU_PROPS_JOYSTICK else MENU_PROPS_KEYBOARD)
                }
            }
            MENU_PROPS_SOUND, MENU_PROPS_TURBO,
            MENU_PROPS_TAPE, MENU_PROPS_EXECUTE   -> updatePropsMenuItem(id)
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

    private fun updatePropsMenuItem(id: Int) {
        val prop = menuProps[id - MENU_PROPS_KEYBOARD]
        val isChecked = !props[prop].toBoolean

        if(id == MENU_PROPS_KEYBOARD || id == MENU_PROPS_JOYSTICK) {
            props[ZX_PROP_SHOW_KEY] = 0.toByte()
            props[ZX_PROP_SHOW_JOY] = 0.toByte()
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal, 100)
            hand?.send(RECEPIENT_SURFACE_UI, ZxMessages.ACT_UPDATE_JOY.ordinal, 200)
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

/*
23 ноября 2019 - 12:00
осталось:
1. сообщение об ошибке                      +
2. проверить на всех платформах и на моем   +
3. MRU                                      +
4. native:
    4.1. presets
    4.2. open/save
    4.3. changeModel                +-
    4.4. signalRESET                +-
    4.5. updateKEY                  +
5. лента в настройках

29 ноября 2019- 0:10
сейчас:
1. декомпозиция загрузки из облака  +
2. native:
    2.1. open/save                  +
    2.2. продумать changeModel      +
    2.3. продумать signalRESET      +
    2.4. presets

3:00
сейчас:
1. presets          +
2. writePort        +
3. readPort         +
4. updateFrame      +
5. step             +
6. updateCPU        +
7. execute          +

осталось:
1. изменить имена AY регистров
2. убрать промежуточный массив цветов             +

8:00
осталось:
1. новый дешифратор процессора:
разрядность(8,16)
операция(jump,aripth,shift,bits,assign,load,save,spec,rst,)
регистр
флаги
длина??
такты

29 ноября 2019 - 14:30
сейчас:
1. определить структуру инструкции      +
2. сделать массив структур инструкций   +-
3. выполнение операций                  +-

23:00
сейчас:
1. арифметика                           +
2. логические                           +

30 ноября 2019 - 0:10
сейчас:
1. переходы с флагом
2. сдвиги
3. [hl]                                 +
4. проверить префиксы                   +
5. вычисление флагов
6. расставить такты
7. порты

2:20
осталось:
1. сдвиги                               +
2. расставить такты                     +
3. порты
4. битовые операции                     +
5. префикс + битовые операции
6. реорганизовать структуру             +

5:40
осталось:
1. порты                                +
2. префикс + битовые операции
3. переходы с флагом
4. расставить маски флагов
5. операции с PREF_ED                   +
6. убрать условие в инкр/декр           +
7. строковые операции                   +
8. RLD/RRD                              +

8:00
осталось:
1. при смене [HL] [IX/IY + D] менять такты  +
2. все проверить                            +-?
3. префикс + битовые операции
4. переходы с флагом                        +

9:30
осталось:
1. префикс + битовые операции               +
2. все проверить
3. запустить систему
4. расставить маски флагов                  +
5. расставить вычисление флагов
6. панель навигации после восстановления
7. поставить zxemu и проверить некоторые инструкции +
8. changeModel при загрузке .z80 и меню     +

1 декабря 2019 - 7:00
осталось:
1. панель навигации после восстановления
2. все проверить
3. запустить систему
4. проверить флаги
5. сделать нормальный дизасм

19:10
сейчас:
1. косметические изменения          +
2. трассировать после запуска
3. сделать нормальный дизасм
4. панель навигации после восстановления

2 декабря 2019 - 0:0
сейчас:
1. убрать флаги                         +
2. переделать структуры                 +
3. переделать установку флагов          +
4. определить сбрасываемые флаги        +
5. трассировать после запуска
6. сделать нормальный дизасм
7. панель навигации после восстановления
8. установка флагов в одном месте       +

2 декабря 2019 - 17:00
сейчас:
1. проверить установку флагов           +
2. диасм инструкции                     +
3. трассировать после запуска           +
4. панель навигации после восстановления

4 декабря 2019 - 18:00
осталось:
1. панель навигации после восстановления
2. режим отладчика в вертикальной ориентации
3. в дизасм добавить метки с описанием ПЗУ
    и при прямом чтении/записи в операнд            +
4. клавиатура                                       +
5. мелькание курсора
6. DAA
7. в тулбар - клава/джойстик
8. ошибка в сохранении\загрузке состояния           +
9. список всех инструций - проверить                +

5 декабря 2019 - 16:00
осталось:
1. в тулбар - клава/джойстик
2. сравнить работу двух дешифраторов
3. вставить вычисление флагов в виндовс эмулятор            +
4. переделать сохранение/загрузку - опять какая-то хрень    +
5. форма установок                                          +

6 декабря 2019 - 0:10
осталось:
1. в тулбар - клава/джойстик
2. сравнить работу двух дешифраторов                        +
3. панель навигации после восстановления
4. режим отладчика в вертикальной ориентации
5.

сейчас - 4:30
1. в тулбар - клава/джойстик                                +

6:00
2. управляющие кнопки клавы
2. изменение ориентации                                     +
3. разметка для отладчика
4. опять настройки слитают
5. опрос порта FC у компаньона
6. openZ80
7. габариты форм для разных ориентаций                      +
8. ид элементов, чтобы не падала при смене ориентации       +
9. сделать одну главную форму - остальные от нее            +
10. DAA

8:00
осталось:
1. управляющие кнопки клавы                                 +
2. openZ80                                                  +
3. DAA
4. опрос порта FC у компаньона
5. проброс сообщений в основную форму                       +
6. поворот экрана без ИД                                    +
7. при сбросе программы(PC = 0) - активировать сброс        +

6 декабря 2019 - 17:00
осталось:
1. проверить Z80                                            +
2. DAA
3. еше сравнить два дешифратора                             +
4. восстановление onResume                                  +
5. смена ориентации - состояние эмулятора                   +
6. изменение границы - блокировать поток                    +
7. иконка облака
8. вылет облака без инета
9. все-таки есть ошибка - exolon

18:30
сейчас:
1. иконка облака                                            +
2. вылет облака без инета
3. иконки actionBar - в тайлы?
4. все-таки есть ошибка - exolon+, dizzy+, пзу++-
5. сброс при ошибке в проге
6. исправить иконку - установки                             +
7. после записи файла - ошибка без ошибки                   +
8. ошибка в виджете прогресса
9, надписи в списке в одну строку                           +
10. подписывать все билды одним сертификатом                +

22:00
осталось:
1. вылет облака без инета
2. иконки actionBar - в тайлы?
3. сброс при ошибке в проге
4. ошибка в пзу - числа                                     +
5. размер текста на разных экранах
6. пропуск кадров - неверно
7. звук тапа убрать нах???
8. задержка при выделении в списке                          +
9. клава компаньона на fc неверна



 */