package ru.ostrovskal.zx

import android.content.Context
import android.content.res.AssetManager
import android.graphics.Bitmap
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Menu
import android.view.MenuItem
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import ru.ostrovskal.sshstd.Common.*
import ru.ostrovskal.sshstd.Wnd
import ru.ostrovskal.sshstd.layouts.AbsoluteLayout
import ru.ostrovskal.sshstd.objects.Settings
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.ui.UiComponent
import ru.ostrovskal.sshstd.ui.UiCtx
import ru.ostrovskal.sshstd.ui.absoluteLayout
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

    private var idsMap          = mapOf<Int, Int>()

    private fun menuId(id: Int) = idsMap[id] ?: error("ID not found!")

    companion object {
        init {
            System.loadLibrary("zx-lib")
        }

        val modelNames  = listOf(R.string.menu48kk, R.string.menu48k, R.string.menu128k, R.string.menuPentagon, R.string.menuScorpion)

        val props       = ByteArray(ZX_PROPS_COUNT)

        @JvmStatic
        external fun zxInit(asset: AssetManager)

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
        external fun zxStringToNumber(value: String, radix: Int): Int

        @JvmStatic
        external fun zxNumberToString(value: Int, radix: Int): String
*/
    }

    override fun onPause() {
        exit()
        super.onPause()
    }

    override fun applyTheme() {
        Theme.setTheme(this, themeDef)
    }

    override fun initialize(restart: Boolean) {
        if (hand == null) {
            // Создаем UI хэндлер
            hand = Handler(Looper.getMainLooper(), this)
            // Инициализируем установки
            Settings.initialize(getSharedPreferences(logTag, Context.MODE_PRIVATE), loadResource("settings", "array_str", arrayOf("")))
            // Применяем тему
            applyTheme()
            // Инициализация эмулятора
            runBlocking {
                withContext(Dispatchers.IO) {
                    zxProps(props)
                    loadResource("settings", "array_str", arrayOf("")).forEachIndexed { i, key ->
                        if(i < ZX_PROPS_INIT_COUNT) zxGetProp(key.substringBeforeLast(',').s, i)
                    }
                    zxInit(assets)
                }
            }
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
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        startLog(this, "ZX", true, BuildConfig.VERSION_CODE, BuildConfig.VERSION_NAME, BuildConfig.DEBUG, enumNames(ZxMessages.values()))
        super.onCreate(savedInstanceState)
        Main().setContent(this, SSH_APP_MODE_GAME or SSH_APP_MODE_TITLE or SSH_APP_MODE_FULLSCREEN)
        // загружаем фрагмент
        if (savedInstanceState == null) instanceForm(FORM_MAIN)
    }

    override fun onMenuItemSelected(featureId: Int, item: MenuItem): Boolean {
        item.subMenu.apply {
            when (item.itemId) {
                R.id.menu_model -> getItem(props[ZX_PROP_MODEL_TYPE].toInt()).isChecked = true
                R.id.menu_disk  -> getItem(props[ZX_PROP_ACTIVE_DISK].toInt()).isChecked = true
                R.id.menu_props -> {
                    getItem(0).isChecked = props[ZX_PROP_SND_LAUNCH].toBoolean
                    getItem(1).isChecked = props[ZX_PROP_SHOW_KEY].toBoolean
                    getItem(2).isChecked = props[ZX_PROP_SHOW_JOY].toBoolean
                    getItem(3).isChecked = props[ZX_PROP_TRAP_TAPE].toBoolean
                    getItem(4).isChecked = "filter".b
                    getItem(5).isChecked = props[ZX_PROP_TURBO_MODE].toBoolean
                }
                R.id.menu_mru   -> repeat(10) { getItem(it).title = "#mru${it + 1}".s }
            }
        }
        return super.onMenuItemSelected(featureId, item)
    }

    @Suppress("NULLABILITY_MISMATCH_BASED_ON_JAVA_ANNOTATIONS")
    override fun onMenuOpened(featureId: Int, menu: Menu?): Boolean {
        menu?.findItem(R.id.menu_execute)?.isChecked = props[ZX_PROP_EXECUTE].toBoolean
        return super.onMenuOpened(featureId, menu)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.main_menu, menu)
        idsMap = mapOf( R.id.menu_disk1 to 0, R.id.menu_disk2 to 1, R.id.menu_disk3 to 2, R.id.menu_disk4 to 3,
                        R.id.menu_48kk to 0, R.id.menu_48k to 1, R.id.menu_128k to 2, R.id.menu_pentagon to 3, R.id.menu_scorpion to 4,
                        R.id.menu_mru1 to 0, R.id.menu_mru2 to 1, R.id.menu_mru3 to 2, R.id.menu_mru4 to 3, R.id.menu_mru5 to 4,
                        R.id.menu_mru6 to 5, R.id.menu_mru7 to 6, R.id.menu_mru8 to 7, R.id.menu_mru9 to 8, R.id.menu_mru10 to 9,
                        R.id.menu_sound to ZX_PROP_SND_LAUNCH, R.id.menu_tape to ZX_PROP_TRAP_TAPE, R.id.menu_turbo to ZX_PROP_TURBO_MODE,
                        R.id.menu_keyboard to ZX_PROP_SHOW_KEY, R.id.menu_joystick to ZX_PROP_SHOW_JOY, R.id.menu_execute to ZX_PROP_EXECUTE
        )
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when(val id = item.itemId) {
            R.id.menu_pokes                     -> instanceForm(FORM_POKES)
            R.id.menu_cloud                     -> instanceForm(FORM_LOADING)
            R.id.menu_io                        -> instanceForm(FORM_IO, "filter", ".z80,.tap,.tga")
            R.id.menu_settings                  -> instanceForm(FORM_OPTIONS)
            R.id.menu_restore                   -> hand?.send(RECEPIENT_SURFACE_BG, ZxMessages.ACT_IO_LOAD.ordinal, o = ZX_AUTO_SAVE)
            R.id.menu_reset                     -> hand?.send(RECEPIENT_SURFACE_BG, ZxMessages.ACT_RESET.ordinal)
            R.id.menu_exit                      -> finish()
            R.id.menu_keyboard, R.id.menu_joystick,
            R.id.menu_sound, R.id.menu_turbo,
            R.id.menu_tape, R.id.menu_execute   -> updatePropsMenuItem(id)
            R.id.menu_disk1, R.id.menu_disk2,
            R.id.menu_disk3, R.id.menu_disk4    -> {
                props[ZX_PROP_ACTIVE_DISK] = menuId(id).toByte()
                instanceForm(FORM_IO, "filter", ".trd")
            }
            R.id.menu_48kk, R.id.menu_48k,
            R.id.menu_128k, R.id.menu_pentagon,
            R.id.menu_scorpion                  -> {
                props[ZX_PROP_MODEL_TYPE] = menuId(id).toByte()
                hand?.send(RECEPIENT_SURFACE_BG, ZxMessages.ACT_MODEL.ordinal)
            }
            R.id.menu_filter                    -> {
                "filter".b = !"filter".b
                hand?.send(RECEPIENT_SURFACE_UI, ZxMessages.ACT_UPDATE_FILTER.ordinal)
            }
            R.id.menu_mru1, R.id.menu_mru2,
            R.id.menu_mru3, R.id.menu_mru4,
            R.id.menu_mru5, R.id.menu_mru6,
            R.id.menu_mru7, R.id.menu_mru8,
            R.id.menu_mru9, R.id.menu_mru10     -> openMRU(id, item.title.toString(), false)
        }
        return super.onOptionsItemSelected(item)
    }

    private fun updatePropsMenuItem(id: Int) {
        val prop = menuId(id)

        val isChecked = !props[prop].toBoolean

        if(id == R.id.menu_keyboard || id == R.id.menu_joystick) {
            props[ZX_PROP_SHOW_KEY] = 0.toByte()
            props[ZX_PROP_SHOW_JOY] = 0.toByte()
            hand?.send(RECEPIENT_FORM, ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal, 200)
            hand?.send(RECEPIENT_SURFACE_UI, ZxMessages.ACT_UPDATE_JOY.ordinal, 200)
        }
        props[prop] = isChecked.toByte
        zxCmd(ZX_CMD_PROPS, 0, 0, "")
    }

    fun openMRU(id: Int, title: String, error: Boolean) {
        val idx = menuId(id) + 1
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
1. проверить установку флагов
2. диасм инструкции
3. трассировать после запуска
4. панель навигации после восстановления

 */