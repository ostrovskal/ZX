package ru.ostrovskal.zx.forms

import android.graphics.Color
import android.os.Message
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import ru.ostrovskal.sshstd.Common.*
import ru.ostrovskal.sshstd.TileDrawable
import ru.ostrovskal.sshstd.adapters.ArrayListAdapter
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.layouts.TabLayout
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Check
import ru.ostrovskal.sshstd.widgets.Seek
import ru.ostrovskal.sshstd.widgets.Text
import ru.ostrovskal.sshstd.widgets.lists.Spinner
import ru.ostrovskal.zx.R
import ru.ostrovskal.zx.ZxCommon.*
import ru.ostrovskal.zx.ZxWnd

@Suppress("unused")
class ZxFormSettings : Form() {
    private var curView         = 0
    private var updateJoy       = false
    private var updateBorder    = false
    private var updateKey       = false
    private val copyProps       = ByteArray(ZX_PROPS_COUNT)

    private val settingsAY      = listOf("ACB", "ABC", "NONE")
    private val settingsFreq    = listOf("44100", "22050", "11025")

    private val settingsJoyTypes= listOf("KEMPSTON", "SINCLAIR I", "SINCLAIR II", "CURSOR", "CUSTOM")

    private val keyButtons      = listOf("N/A", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "N/A", "Q", "W", "E", "R", "T", "Y", "U",
                                        "I", "O", "P", "ENTER", "CAPS", "A", "S", "D", "F", "G", "H", "J", "K", "L", "SYMBOL", "N/A", "Z",
                                        "X", "C", "SPACE", "V", "B", "N", "M", "←", "→", "↑", "↓", "K←", "K→", "K↑", "K↓", "K*")

    private val idNulls         = listOf(R.id.buttonNull1, R.id.buttonNull2, R.id.buttonNull3, R.id.buttonNull4, R.id.buttonNull5, R.id.buttonNull6,
                                            R.id.buttonNull7, R.id.buttonNull8)

    private val idSpinners      = listOf(R.id.spinnerKey1, R.id.spinnerKey2, R.id.spinnerKey3, R.id.spinnerKey4, R.id.spinnerKey5, R.id.spinnerKey6,
                                            R.id.spinnerKey7, R.id.spinnerKey8)

    private val idSeeks      = listOf(R.id.seekCommon1, R.id.seekCommon2, R.id.seekCommon3, R.id.seekCommon4, R.id.seekCommon5,
                                            R.id.seekSnd1, R.id.seekSnd2)

    override fun getTheme() = R.style.dialog_progress

    private val commonPage: TabLayout.Content.() -> View = {
        val ts = Theme.dimen(context, if(config.portrait) 8 else 10).toFloat()
        cellLayout(16, 21) {
            val textsCommon = wnd.loadResource("settingsCommonTexts", "array", IntArray(0))
            val rangeCommon = arrayOf(3..16, 1..6, 1..6, 0..4, 3..9)
            repeat(3) { y ->
                repeat(2) { x ->
                    val pos = y * 2 + x
                    if (pos < 5) {
                        text(textsCommon[pos], style_text_settings) {
                            textSize = Theme.dimen(context, if(config.portrait) 6 else 8).toFloat()
                        }.lps(1 + x * 8, 1 + y * 6, 8, 3)
                        seek(idSeeks[pos], rangeCommon[pos], true) {
                            setOnClickListener {
                                ZxWnd.props[settingsCommon[pos]] = progress.toByte()
                                when (pos) {
                                    3 -> updateBorder = true
                                    2 -> updateJoy = true
                                    4 -> updateKey = true
                                }
                            }
                        }.lps(x * 8, 3 + y * 6, 8, 4)
                    }
                }
            }
            check(R.id.buttonNull1, R.string.settingsFPS) {
                textSize = ts + 2f
                isChecked = ZxWnd.props[ZX_PROP_SHOW_FPS] != 0.toByte()
                setOnClickListener {
                    ZxWnd.props[ZX_PROP_SHOW_FPS] = if(isChecked) 1 else 0
                }
            }.lps(9, 15, 3, 3)
            check(R.id.buttonNull2, R.string.settingsFrames) {
                textSize = ts + 2f
                isChecked = ZxWnd.props[ZX_PROP_SKIP_FRAMES].toBoolean
                setOnClickListener {
                    ZxWnd.props[ZX_PROP_SKIP_FRAMES] = if(isChecked) 1 else 0
                }
            }.lps(12, 15, 5, 3)
        }
    }

    private val joyPage: TabLayout.Content.() -> View = {
        val textsJoy = wnd.loadResource("settingsJoyTexts", "array", IntArray(0))
        val ts = Theme.dimen(context, if(config.portrait) 8 else 10).toFloat()
        var inner = false
        cellLayout(20, 20) {
            text(R.string.settingsJoyType, style_text_settings) { textSize = ts }.lps(0, 0, 3, 4)
            spinner(R.id.settingsJoyType) {
                adapter = ArrayListAdapter(context, Popup(), Item(), settingsJoyTypes)
                itemClickListener = { _, _, p, _ ->
                    if(!inner) {
                        ZxWnd.props[ZX_PROP_JOY_TYPE] = p.toByte()
                        (root as? TabLayout)?.apply {
                            currentContent.apply {
                                repeat(5) {
                                    byIdx<Spinner>(5 + it * 2).apply {
                                        selectionString = joyButtons[p * 5 + it]
                                        ZxWnd.props[ZX_PROP_JOY_KEYS + it] = selection.toByte()
                                        isEnabled = p == 4
                                    }
                                }
                            }
                        }
                    }
                }
            }.lps(2, 0, 7, 4)
            text(R.string.settingsJoyPreset, style_text_settings) { textSize = ts }.lps(10, 0, 10, 4)
            spinner(R.id.settingsJoyPreset) {
                adapter = ArrayListAdapter(context, Popup(), Item(), ZxWnd.zxPresets(ZX_CMD_PRESETS_LIST).split(','))
                mIsSelection = false
                itemClickListener = { _, v, _, _ ->
                    (v as? Text)?.apply {
                        inner = true
                        ZxWnd.zxCmd(ZX_CMD_PRESETS, ZX_CMD_PRESETS_LOAD, 0, text.toString())
                        (root as? TabLayout)?.apply {
                            currentContent.byIdx<Spinner>(1).selection = ZxWnd.props[ZX_PROP_JOY_TYPE].toInt()
                            repeat(8) {
                                currentContent.byIdx<Spinner>(it * 2 + 5).selection = ZxWnd.props[ZX_PROP_JOY_KEYS + it].toInt()
                            }
                        }
                        inner = false
                    }
                }
            }.lps(13, 0, 7, 4)
            repeat(2) { y ->
                repeat(4) { x ->
                    val pos = y * 4 + x
                    text(textsJoy[pos], style_text_settings) { textSize = ts }.lps(x * 5 + 1, 4 + y * 7, 3, 3)
                    spinner(idSpinners[pos]) {
                        adapter = ArrayListAdapter(context, Popup(), Item(), keyButtons)
                        selection = ZxWnd.props[ZX_PROP_JOY_KEYS + pos].toInt()
                        itemClickListener = { _, _, p, _ ->
                            ZxWnd.props[ZX_PROP_JOY_KEYS + pos] = p.toByte()
                        }
                    }.lps(x * 5, 7 + y * 7, 4, 4)
                }
            }
        }
    }

    private val soundPage: TabLayout.Content.() -> View = {
        val textsSnd = context.loadResource("settingsSndTexts", "array", IntArray(0))
        val ts = Theme.dimen(context, if(config.portrait) 8 else 10).toFloat()
        var volBp: Seek? = null
        var volAy: Seek? = null
        cellLayout(10, 21) {
            repeat(2) { y ->
                repeat(2) { x ->
                    val pos = y * 2 + x
                    text(textsSnd[pos], style_text_settings) { textSize = ts }.lps(1 + x * 5, y * 9, 4, 3)
                    if(y == 0) {
                        spinner(if(x == 0) R.id.settingsTypeAY else R.id.settingsFreq) {
                            adapter = ArrayListAdapter(context, Popup(), Item(), if(x == 0) settingsAY else settingsFreq)
                            itemClickListener = { _, _, p, _ ->
                                ZxWnd.props[settingsSnd[pos]] = p.toByte()
                            }
                        }.lps(x * 5, 3, 4, 4)
                    } else {
                        val sk = seek(idSeeks[( pos and 1 ) + 5], if(x == 0) 0..15 else 0..100, true) {
                            setOnClickListener {
                                ZxWnd.props[settingsSnd[pos]] = progress.toByte()
                            }
                        }.lps(x * 5, 12, 5, 4)
                        if(x == 0) volBp = sk else volAy = sk
                    }
                }
            }
            repeat(4) { x ->
                check(idNulls[x + 2], textsSnd[x + 4]) {
                    textSize = ts + 2f
                    setOnClickListener {
                        ZxWnd.props[settingsCheckSnd[x]] = if(isChecked) 1 else 0
                        if(x == 0) volBp?.apply { isEnabled = this@check.isChecked }
                        else if(x == 1) volAy?.apply { isEnabled = this@check.isChecked }
                    }
                }.lps(1 + x * 2, 16, 3, 4)
            }
        }
    }

    private val tapePage: TabLayout.Content.() -> View = {
        cellLayout(10, 10) {}
    }

    private val screenPage: TabLayout.Content.() -> View = {
        val nameColors = context.loadResource("settingsColorNames", "array", IntArray(0))
        val ts = Theme.dimen(context, if(config.portrait) 6 else 8).toFloat()
        cellLayout(32, 20, 1) {
            repeat(2) {y ->
                repeat(8) { x ->
                    val pos = y * 8 + x
                    text(nameColors[pos], style_color_text_settings) {
                        textSize = ts
                        backgroundSet {
                            selectorColor = Color.WHITE
                            selectorWidth = 3f
                            if(pos == 0) shape = TILE_SHAPE_ROUND
                        }
                        setOnClickListener {
                            (it.background as? TileDrawable)?.apply {
                                shape = TILE_SHAPE_ROUND
                                val color = solid
                                val b = color and 0x000000ff
                                val g = color and 0x0000ff00
                                val r = color and 0x00ff0000
                                (root as? TabLayout)?.apply {
                                    currentContent.apply {
                                        getChildAt(curView).apply {
                                            (background as? TileDrawable)?.shape = TILE_SHAPE_EMPTY
                                            invalidate()
                                        }
                                        curView = indexOfChild(it)
                                        byIdx<Seek>(16).progress = b
                                        byIdx<Seek>(17).progress = g shr 8
                                        byIdx<Seek>(18).progress = r shr 16
                                    }
                                }
                                it.invalidate()
                            }
                        }
                    }.lps(x * 4, y * 4, 4, 4)
                }
            }
            repeat(3) {
                seek(0, 0..255, true) {
                    setOnClickListener { seek ->
                        val mask = arrayOf(0xffffff00, 0xffff00ff, 0xff00ffff)
                        (root as? TabLayout)?.apply {
                            val self = currentContent.indexOfChild(seek) - 16
                            (seek as? Seek)?.apply {
                                val value = progress
                                (currentContent.getChildAt(curView).background as? TileDrawable)?.apply {
                                    solid = ((solid and mask[self].toInt()) or (value shl (self * 8)))
                                    ZxWnd.props[ZX_PROP_COLORS + curView * 4 + ((self xor 3) - 1)] = value.toByte()
                                }
                            }
                        }
                    }
                }.lps(2, 8 + it * 4, 28, 4)
            }
        }
    }

    override fun footer(btn: Int, param: Int) {
        when (btn) {
            BTN_OK -> {
                wnd.hand?.apply {
                    super.footer(btn, param)
                    if (updateBorder) send(RECEPIENT_SURFACE_UI, ZxWnd.ZxMessages.ACT_UPDATE_SURFACE.ordinal, a1 = 0)
                    if (updateJoy) send(RECEPIENT_SURFACE_UI, ZxWnd.ZxMessages.ACT_UPDATE_JOY.ordinal)
                    if (updateKey) send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal)
                    send(RECEPIENT_SURFACE_UI, ZxWnd.ZxMessages.ACT_UPDATE_SKIP_FRAMES.ordinal)
                }
                ZxWnd.zxCmd(ZX_CMD_PRESETS, ZX_CMD_PRESETS_SAVE, 0, ZxWnd.zxPresets(ZX_CMD_PRESETS_NAME))
                ZxWnd.zxCmd(ZX_CMD_PROPS, 0, 0, "")
            }
            BTN_DEF -> {
                val tab = (root as? TabLayout)?.currentTab ?: 0
                setTabValues(tab, true)
                return
            }
            else -> {
                super.footer(btn, param)
                copyProps.copyInto(ZxWnd.props)
            }
        }
    }

    override fun initContent(content: ViewGroup) {
        ZxWnd.props.copyInto(copyProps)
        repeat(4) { setTabValues(it, false) }
        super.initContent(content)
    }

    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                cellLayout(10, 16) {
                    formHeader(R.string.settingsHead)
                    backgroundSet { alpha =  255; solid = Color.WHITE }
                    root = tabLayout(sizeCaption = 18, style = style_tab_settings) {
                        content.apply {
                            padding = 2.dp
                            backgroundSet(style_form) { alpha = 128 }
                        }
                        page(R.id.pageCommon, nIcon = R.integer.I_TOOL, init = commonPage)
                        page(R.id.pageSound, nIcon = R.integer.I_SOUND, init = soundPage)
                        page(R.id.pageJoy, nIcon = R.integer.I_JOY, init = joyPage)
                        page(R.id.pageScreen, nIcon = R.integer.I_DISPLAY, init = screenPage)
                        page(R.id.pageTape, nIcon = R.integer.I_CASSETE, init = tapePage)
                    }.lps(0, 0, 10, 12)
                    formFooter(BTN_OK, R.integer.I_YES, BTN_NO, R.integer.I_NO, BTN_DEF, R.integer.I_DEFAULT)
                }.lps(Theme.dimen(ctx, R.dimen.settingsWidth), Theme.dimen(ctx, R.dimen.settingsHeight))
            }
        }
    }

    private fun setTabValues(tab: Int, reset: Boolean) {
        (root as? TabLayout)?.content?.apply {
            val c = getChildAt(tab)
            val s = wnd.loadResource("settings", "array_str", arrayOf(""))
            when(tab) {
                0 -> defaultCommon(c, s, reset)
                1 -> defaultSound(c, s, reset)
                2 -> defaultJoy(c, reset)
                3 -> defaultScreen(c, s, reset)
            }
        }
    }

    private fun defaultScreen(content: View, settings: Array<String>, reset: Boolean) {
        repeat(16) { idx ->
            if(reset) {
                val o = (idx + ZX_PROP_COLORS) - ZX_PROP_FIRST_LAUNCH
                ZxWnd.zxGetProp(settings[o].substringAfterLast(','), o)
            }
            content.byIdx<Text>(idx).apply {
                (background as? TileDrawable)?.apply {
                    val color = ZxWnd.zxInt(ZX_PROP_COLORS + idx * 4, true, 0).toInt()
                    val b = (color and 0x00ff0000) shr 16
                    val g = color and 0x0000ff00
                    val r = (color and 0x000000ff) shl 16
                    val a = color and 0xff000000.toInt()
                    solid = r or g or b or a
                    if(curView == idx) {
                        content.byIdx<Seek>(16).progress = b
                        content.byIdx<Seek>(17).progress = g shr 8
                        content.byIdx<Seek>(18).progress = r shr 16
                    }
                }
            }
        }
    }

    private fun defaultJoy(content: View, reset: Boolean) {
        if(reset) ZxWnd.props[ZX_PROP_JOY_TYPE] = 0
        content.byIdx<Spinner>(1).selection = ZxWnd.props[ZX_PROP_JOY_TYPE].toInt()
    }

    private fun defaultSound(content: View, settings: Array<String>, reset: Boolean) {
        for (idx in 11 downTo 0) {
            val opt = settingsAllSnd[idx]
            if(opt > 0) {
                if(reset) {
                    val o = opt - ZX_PROP_FIRST_LAUNCH
                    ZxWnd.zxGetProp(settings[o].substringAfterLast(','), o)
                }
                val v = ZxWnd.props[opt].toInt()
                when (idx) {
                    1, 3        -> content.byIdx<Spinner>(idx).selection   = v
                    5, 7        -> content.byIdx<Seek>(idx).progress       = v
                    8, 9, 10, 11-> content.byIdx<Check>(idx).isChecked     = v != 0
                }
            }
        }
    }

    private fun defaultCommon(content: View, settings: Array<String>, reset: Boolean) {
        repeat(7) {
            val opt = settingsCommon[it]
            if(reset) {
                val o = opt - ZX_PROP_FIRST_LAUNCH
                ZxWnd.zxGetProp(settings[o].substringAfterLast(','), o)
            }
            if(it < 5) {
                content.byIdx<Seek>(it * 2 + 1).progress = ZxWnd.props[opt].toInt()
            } else {
                content.byIdx<Check>(it + 5).isChecked = ZxWnd.props[opt].toBoolean
            }
        }
        if(reset) { updateJoy = true; updateBorder = true; updateKey = true }
    }

    class Item : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) {
            text(R.string.null_text, style_spinner_item_settings) {
                textSize = Theme.dimen(ctx, if(config.portrait) 10 else 14).toFloat()
                backgroundSet(style_spinner_item) }
        }
    }

    class Popup : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) {
            text(R.string.null_text, style_spinner_title_settings) {
                textSize = Theme.dimen(ctx, if(config.portrait) 10 else 14).toFloat()
            }
        }
    }

    override fun handleMessage(msg: Message): Boolean {
        wnd.findForm<ZxFormMain>("main")?.handleMessage(msg)
        return super.handleMessage(msg)
    }
}
