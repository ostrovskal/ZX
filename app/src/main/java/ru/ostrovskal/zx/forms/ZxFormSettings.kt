@file:Suppress("DEPRECATION")

package ru.ostrovskal.zx.forms

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Color
import android.graphics.Typeface
import android.util.TypedValue
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.LinearLayout
import ru.ostrovskal.sshstd.Common.*
import ru.ostrovskal.sshstd.Config
import ru.ostrovskal.sshstd.TileDrawable
import ru.ostrovskal.sshstd.adapters.ArrayListAdapter
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.layouts.TabLayout
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Check
import ru.ostrovskal.sshstd.widgets.Seek
import ru.ostrovskal.sshstd.widgets.Text
import ru.ostrovskal.sshstd.widgets.html.Html
import ru.ostrovskal.sshstd.widgets.lists.Spinner
import ru.ostrovskal.zx.R
import ru.ostrovskal.zx.ZxCommon.*
import ru.ostrovskal.zx.ZxPreset
import ru.ostrovskal.zx.ZxWnd
import java.util.*

@Suppress("unused")
class ZxFormSettings : Form() {
    companion object {
        // количество блоков ленты
        private var countTapeBlocks            = 0

        // выбранный диск
        private var numDisk                     = 0

        // количество файлов на диске
        private var countDiskFiles              = 0

        // указатель на объект справки
        private var html: Html?                = null

        private val tapeTypes       = listOf("BASIC", "NumberArray", "StringArray", "Bytes")
        private val settingsAY      = listOf("MONO", "ABC", "ACB")
        private val settingsFreq    = listOf("44100", "22050", "11025")
        private val namesDisk       = listOf("A: ", "B: ", "C: ", "D: ")
        private val diskSectors                 = ByteArray(2048) { 0 }

        private val settingsJoyTypes= listOf("KEMPSTON", "SINCLAIR I", "SINCLAIR II", "CURSOR", "CUSTOM")

        private val keyButtons      = listOf("N/A", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
            // 11
            "DEL", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
            // 22
            "ENTER", "CAPS", "A", "S", "D", "F", "G", "H", "J", "K", "L",
            // 33
            "SYMBOL", "N/A", "Z", "X", "C", "SPACE", "V", "B", "N", "M", "RU/EN",
            // 44
            "↑", "↓", "←", "→",
            // 48
            "K←", "K→", "K↑", "K↓", "K*")

        private val idNulls         = listOf(R.id.button1, R.id.button2, R.id.button3, R.id.button4, R.id.button5, R.id.button6, R.id.button7, R.id.button8)

        private val idSpinners      = listOf(R.id.spinner5, R.id.spinner6, R.id.spinner7, R.id.spinner8, R.id.spinner9, R.id.spinner10, R.id.spinner11, R.id.spinner12)

        private val idSeeks         = listOf(R.id.seek1, R.id.seek2, R.id.seek3, R.id.seek4, R.id.seek5, R.id.seek6, R.id.seek7, R.id.seek8, R.id.seek9, R.id.seek10)

        private val rangeCommon= arrayOf(3..16, 2..7, 0..3, 3..9)
    }

    private var curView         = 0
    private var updateJoy       = false
    private var updateKey       = false
    private val copyProps       = ByteArray(ZX_PROPS_COUNT)

    override fun getTheme() = R.style.dialog_progress

    private val commonPage: TabLayout.Content.() -> View = {
        val textsCommon = context.loadResource("settingsCommonTexts", "array", IntArray(0))
        cellLayout(16, 21) {
            repeat(2) { y ->
                repeat(2) { x ->
                    val pos = y * 2 + x
                    text(textsCommon[pos], style_text_settings).lps(1 + x * 8, 2 + y * 7, 8, 3)
                    seek(idSeeks[pos + 5], rangeCommon[pos], true) {
                        setOnClickListener {
                            ZxWnd.props[settingsCommon[pos]] = progress.toByte()
                            when (pos) {
                                1 -> updateJoy = true
                                3 -> updateKey = true
                            }
                        }
                    }.lps(x * 8, 4 + y * 7, 8, 5)
                }
            }
        }
    }

    private val joyPage: TabLayout.Content.() -> View = {
        val textsJoy = context.loadResource("settingsJoyTexts", "array", IntArray(0))
        var inner = false
        cellLayout(20, 20) {
            spinner(R.id.spinner1) {
                adapter = ArrayListAdapter(context, Popup(), Item(), settingsJoyTypes)
                itemClickListener = { _, _, p, _ ->
                    if(!inner) {
                        ZxWnd.props[ZX_PROP_JOY_TYPE] = p.toByte()
                        (root as? TabLayout)?.apply {
                            currentContent.apply {
                                repeat(5) {
                                    byIdx<Spinner>(it * 2 + 3).apply {
                                        selectionString = joyButtons[p * 5 + it]
                                        ZxWnd.props[ZX_PROP_JOY_KEYS + it] = selection.toByte()
                                        isEnabled = p == 4
                                    }
                                }
                            }
                        }
                    }
                }
            }.lps(0, 0, 9, 4)
            spinner(R.id.spinner2) {
                adapter = ArrayListAdapter(context, Popup(), Item(), ZxPreset.list())
                mIsSelection = false
                itemClickListener = { _, v, _, _ ->
                    (v as? Text)?.apply {
                        inner = true
                        ZxPreset.load(text.toString())
                        (root as? TabLayout)?.apply {
                            currentContent.byIdx<Spinner>(0).selection = ZxWnd.props[ZX_PROP_JOY_TYPE].toInt()
                            repeat(8) {
                                currentContent.byIdx<Spinner>(it * 2 + 3).selection = ZxWnd.props[ZX_PROP_JOY_KEYS + it].toInt()
                            }
                        }
                        inner = false
                    }
                }
            }.lps(9, 0, 11, 4)
            repeat(2) { y ->
                repeat(4) { x ->
                    val pos = y * 4 + x
                    text(textsJoy[pos], style_text_settings).lps(x * 5 + 1, 4 + y * 7, 4, 3)
                    spinner(idSpinners[pos]) {
                        adapter = ArrayListAdapter(context, Popup(), Item(), keyButtons)
                        selection = ZxWnd.props[ZX_PROP_JOY_KEYS + pos].toInt()
                        itemClickListener = { _, _, p, _ ->
                            ZxWnd.props[ZX_PROP_JOY_KEYS + pos] = p.toByte()
                        }
                    }.lps(x * 5, 7 + y * 7, 5, 4)
                }
            }
        }
    }

    private val soundPage: TabLayout.Content.() -> View = {
        val textsSnd = context.loadResource("settingsSndTexts", "array", IntArray(0))
        var volBp: Seek? = null
        var volAy: Seek? = null
        cellLayout(10, 21) {
            repeat(2) { x ->
                text(textsSnd[x], style_text_settings).lps(1 + x * 5, 0, 4, 3)
                spinner(if(x == 0) R.id.spinner3 else R.id.spinner4) {
                    adapter = ArrayListAdapter(context, Popup(), Item(), if(x == 0) settingsAY else settingsFreq)
                    itemClickListener = { _, _, p, _ ->
                        ZxWnd.props[settingsSnd[x]] = p.toByte()
                    }
                }.lps(x * 5, 3, 4, 4)
            }
            repeat(2) { x ->
                text(textsSnd[x + 2], style_text_settings).lps(x * 5, 9, 4, 3)
                val sk = seek(idSeeks[x], if(x == 0) 0..16 else 0..28, true) {
                    setOnClickListener {
                        ZxWnd.props[settingsSnd[x + 2]] = progress.toByte()
                    }
                }.lps(x * 5, 12, 5, 4)
                if(x == 0) volBp = sk else volAy = sk
            }
            repeat(2) { x ->
                check(idNulls[x + 2], textsSnd[x + 4]) {
                    setOnClickListener {
                        ZxWnd.props[settingsCheckSnd[x]] = if(isChecked) 1 else 0
                        if(x == 0) volBp?.apply { isEnabled = this@check.isChecked }
                        else if(x == 1) volAy?.apply { isEnabled = this@check.isChecked }
                    }
                }.lps(1 + x * 5, 16, 4, 4)
            }
        }
    }

    private val diskPage: TabLayout.Content.() -> View = {
        cellLayout(10, 10) {
            spinner(R.id.spinner1) {
                adapter = ArrayListAdapter(context, Popup(), Item(), namesDisk)
                itemClickListener = { _, _, p, _ ->
                    numDisk = p
                    var path = "disk$p".s
                    val isEmpty = path.isBlank()
                    if (isEmpty) path = context.getString(R.string.diskEmpty)
                    else {
                        // прочитать количество файлов
                        countDiskFiles = ZxWnd.zxCmd(ZX_CMD_DISK_OPS, numDisk, ZX_DISK_COUNT_FILES, "")
                        // прочитать сектор каталог
                        repeat(8) { dsk ->
                            ZxWnd.zxCmd(ZX_CMD_DISK_OPS, numDisk or (dsk shl 3), ZX_DISK_OPS_RSECTOR, "")
                            ZxWnd.props.copyInto(diskSectors, dsk shl 8, ZX_PROP_VALUES_SECTOR, ZX_PROP_VALUES_SECTOR + 255)
                        }
                    }
                    // в текст поставить выбранный диск
                    (root as? TabLayout)?.apply { currentContent.byIdx<Text>(1).text = path }
                }
            }.lps(0, 0, 3, 2)
            text(R.string.app_name).lps(3, 0, 10, 2)
            ribbon(R.id.ribbonMain, true, style_debugger_ribbon) {
                adapter = DiskAdapter(context, DiskItem())
                padding = 4.dp
                backgroundSet(style_backgrnd_io)
            }.lps(0, 2, 10, 8)
        }
    }

    private val helpPage: TabLayout.Content.() -> View = {
        cellLayout(30, 20) {
            html(style_zx_help) {
                id = R.id.htmlHelp
                html = this
                root = "html/game/" + Config.language
                var key = "tool"
                aliases["tool"] = Html.BitmapAlias(key, context.bitmapGetCache(key), 3, 1, 0, 0, 30, 30)
                key = "sprites"
                aliases["main"] = Html.BitmapAlias(key, context.bitmapGetCache(key), 10, 4, 0, 0, 23, 23)
                key = "zx_icons"
                aliases["icons"] = Html.BitmapAlias(key, context.bitmapGetCache(key), 10, 3, 0, 0, 30, 30)
                key = "controller_zx_cross"
                aliases["cursor"] = Html.BitmapAlias(key, context.bitmapGetCache(key), 6, 1, 0, 0, 45, 45)
                key = "droid"
                aliases["game_panel"] = Html.BitmapAlias(key, context.bitmapGetCache(key))
                key = "menu_common"
                aliases["game_record"] = Html.BitmapAlias(key, context.bitmapGetCache(key))
            }.lps(0, 0, 30, 20)
            button(style_debugger_action) {
                iconResource = R.integer.I_LEFT
                setOnClickListener { html?.back() }
            }.lps(0, 0, 5, 4)
        }
    }

    private val tapePage: TabLayout.Content.() -> View = {
        countTapeBlocks = ZxWnd.zxCmd(ZX_CMD_TAPE_COUNT, 0, 0, "")
        cellLayout(10, 10) {
            ribbon(R.id.ribbonMain, false, style_debugger_ribbon) {
                adapter = TapeAdapter(context, TapeItem())
                padding = 4.dp
                backgroundSet(style_backgrnd_io)
                selection = 0
                itemClickListener = { _, _, p, _ ->
                    selection = p
                    //clickRibbon()
                }
            }.lps(0, 0, 10, 10)
        }
    }

    private val screenPage: TabLayout.Content.() -> View = {
        val nameColors = context.loadResource("settingsColorNames", "array", IntArray(0))
        cellLayout(32, 20, 1) {
            repeat(2) {y ->
                repeat(8) { x ->
                    val pos = y * 8 + x
                    text(nameColors[pos], style_color_text_settings) {
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
                seek(idSeeks[it + 2], 0..255, true) {
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
                    if (updateJoy) send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_JOY.ordinal)
                    if (updateKey) send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal)
                }
                ZxPreset.save()
                ZxWnd.zxCmd(ZX_CMD_PROPS, "filter".i, 0, "")
            }
            BTN_DEF -> {
                val tab = (root as? TabLayout)?.currentTab ?: 0
                setTabValues(tab, true)
                return
            }
            else -> {
                super.footer(btn, param)
                copyProps.copyInto(ZxWnd.props, ZX_PROP_JOY_TYPE, ZX_PROP_JOY_TYPE, ZX_PROPS_COUNT)
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
                    formHeader(R.string.headSettings)
                    backgroundSet(style_form) { alpha = 192 }
                    root = tabLayout(sizeCaption = 18, style = style_tab_settings) {
                        tabChangeListener = {tab, _ ->
                            if(tab == 6) html?.setArticle("", 0)
                        }
                        caption.apply {
                            backgroundSet { alpha = 192; gradient = xyInt; xyInt[0] = Color.BLACK; xyInt[1] = Color.BLUE }
                        }
                        content.apply {
                            padding = 2.dp
                        }
                        page(R.id.pageCommon, nIcon = R.integer.I_TOOL, init = commonPage)
                        page(R.id.pageSound, nIcon = R.integer.I_SOUND, init = soundPage)
                        page(R.id.pageJoy, nIcon = R.integer.I_JOY, init = joyPage)
                        page(R.id.pageScreen, nIcon = R.integer.I_DISPLAY, init = screenPage)
                        page(R.id.pageTape, nIcon = R.integer.I_CASSETE, init = tapePage)
                        page(R.id.pageDisk, nIcon = R.integer.I_DISK, init = diskPage)
                        page(R.id.pageHelp, nIcon = R.integer.I_HELP, init = helpPage)
                    }.lps(0, 0, 10, 12)
                    formFooter(BTN_OK, R.integer.I_YES, BTN_NO, R.integer.I_NO, BTN_DEF, R.integer.I_DEFAULT)
                }.lps(Theme.dimen(ctx, R.dimen.settingsWidth), Theme.dimen(ctx, R.dimen.settingsHeight))
            }
        }
    }

    private fun setTabValues(tab: Int, reset: Boolean) {
        (root as? TabLayout)?.content?.apply {
            val c = getChildAt(tab)
            val s = (wnd as ZxWnd).settings
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
                    val color = ZxWnd.zxInt(ZX_PROP_COLORS + idx * 4, 0xffffffff.toInt(), true, 0).toInt()
                    val b = (color and 0x00ff0000) shr 16
                    val g = color and 0x0000ff00
                    val r = (color and 0x000000ff) shl 16
                    solid = r or g or b or 0xff000000.toInt()
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
        content.byIdx<Spinner>(0).selection = ZxWnd.props[ZX_PROP_JOY_TYPE].toInt()
        content.byIdx<Spinner>(1).selectionString = ZxWnd.zxProgramName("")
    }

    private fun defaultSound(content: View, settings: Array<String>, reset: Boolean) {
        for (idx in 9 downTo 0) {
            val opt = settingsAllSnd[idx]
            if(opt > 0) {
                if(reset) {
                    val o = opt - ZX_PROP_FIRST_LAUNCH
                    ZxWnd.zxGetProp(settings[o].substringAfterLast(','), o)
                }
                val v = ZxWnd.props[opt].toInt()
                when (idx) {
                    1, 3     -> content.byIdx<Spinner>(idx).selection   = v
                    5, 7     -> content.byIdx<Seek>(idx).progress       = v
                    8, 9     -> content.byIdx<Check>(idx).isChecked     = v != 0
                }
            }
        }
    }

    private fun defaultCommon(content: View, settings: Array<String>, reset: Boolean) {
        repeat(4) {
            val opt = settingsCommon[it]
            if(reset) {
                val o = opt - ZX_PROP_FIRST_LAUNCH
                ZxWnd.zxGetProp(settings[o].substringAfterLast(','), o)
            }
            content.byIdx<Seek>(it * 2 + 1).progress = ZxWnd.props[opt].toInt()
        }
        if(reset) { updateJoy = true; updateKey = true }
    }

    class Item : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) {
            text(R.string.null_text, style_spinner_item_settings) {
                backgroundSet(style_spinner_item) }
        }
    }

    class Popup : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) {
            text(R.string.null_text, style_spinner_title_settings)
        }
    }

    class TapeItem : UiComponent() {
        override fun createView(ui: UiCtx) = ui.run {
            cellLayout(10, 12) {
                backgroundSet(style_spinner_item_settings)
                // шапка - заголовок/данные/экран/блок
                text(R.string.null_text, style_text_header) {
                    backgroundSet {
                        selectorWidth = 5f
                        alpha = 192; gradient = xyInt; xyInt[0] = Color.BLACK; xyInt[1] = Color.BLUE
                    }
                }.lps(0, 0, 10, 2)
                // длина
                text(R.string.null_text, style_text_tape) { setTextSize(TypedValue.COMPLEX_UNIT_PX, 22f.dp) }.lps(0, 2, 10, 2)
                // crc
                text(R.string.null_text, style_text_tape).lps(0, 4, 10, 2)
                // имя
                text(R.string.null_text, style_text_tape) {
                    textColor = Theme.integer(context, style.themeAttrValue(ATTR_SSH_COLOR_HEADER, ATTR_SSH_COLOR_HEADER or THEME))
                }.lps(0, 6, 10, 2)
                // тип
                text(R.string.null_text, style_text_tape).lps(0, 8, 10, 2)
                // параметер
                text(R.string.null_text, style_text_tape).lps(0, 10, 10, 2)
            }
        }
    }

    inner class TapeAdapter(context: Context, item: UiComponent) : ArrayListAdapter<ViewGroup>(context, item, item, listOf()) {

        private val block       = ShortArray(10) { 0 }
        private val pblock      = ShortArray(10) { 0 }

        override fun getCount() = countTapeBlocks

        override fun getItemId(position: Int) = -1L

        override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
            return createView(position, convertView, title, parent, false) ?: error("")
        }

        private fun typeBlock(blk: Int, flag: Int, addr: Int): Int {
            return when(blk) {
                0       -> R.string.settingsTapeBasic
                1       -> R.string.settingsTapeNumArray
                2       -> R.string.settingsTapeStringArray
                else    -> if(flag == 0 && addr in 16384..22527) R.string.settingsTapeScreen else R.string.settingsTapeBytes
            }
        }
        @SuppressLint("SetTextI18n")
        override fun createView(position: Int, convertView: View?, resource: UiComponent, parent: ViewGroup, color: Boolean): View? {
            return ((convertView ?: resource.createView(UiCtx(context))) as? CellLayout)?.apply {
                val nameBlock = ZxWnd.zxTapeBlock(position, block)
                ZxWnd.zxTapeBlock(position - 1, pblock)
                val flag    = block[2].toInt()
                val blk     = block[3].toInt()
                val pflag   = pblock[2].toInt()
                val pblk    = pblock[3].toInt()
                byIdx<Text>(0).apply {
                    var sflag = ""
                    val caption = getString(when(flag) {
                        0   -> R.string.settingsTapeCaption
                        255 -> typeBlock(pblk, pflag, pblock[5].toInt())
                        else-> { sflag = "($flag)"; R.string.settingsTapeBlock }
                    })
                    text = "$caption$sflag"
                }
                byIdx<Text>(1).text = (block[0].toInt() and 0xffff).toString()
                byIdx<Text>(2).text = block[1].toString(16).toUpperCase(Locale.ROOT)
                byIdx<Text>(3).text = ""
                byIdx<Text>(4).text = ""
                byIdx<Text>(5).text = ""
                if(flag == 0) {
                    byIdx<Text>(3).text = nameBlock
                    byIdx<Text>(4).text = getString(typeBlock(blk, flag, block[5].toInt()))
                    byIdx<Text>(5).text = (block[4].toInt() and 0xffff).toString()
                } else if(flag == 255) {
                    if(pflag == 0) {
                        val param = (pblock[5].toInt() and 0xffff)
                        val sblk = getString(when (pblk) {
                            0    -> R.string.settingsTapeLine
                            1, 2 -> R.string.settingsTapeArray
                            else -> R.string.settingsTapeAddress
                        })
                        byIdx<Text>(5).text = "$sblk ($param)"
                    }
                }
                layoutParams = LinearLayout.LayoutParams(110.dp, MATCH)
            }
        }
    }

    class DiskItem : UiComponent() {
        override fun createView(ui: UiCtx) = ui.run { text(R.string.null_text, style_debugger_item).apply { padding = 3.dp; typeface = Typeface.MONOSPACE } }
    }

    inner class DiskAdapter(context: Context, item: UiComponent) : ArrayListAdapter<String>(context, item, item, listOf("")) {
        override fun getItem(position: Int): String {
/*
            val idx = position * 8 + 258
            val pos = position + 1
            val flag = ZxWnd.props[idx + 7]
            if(flag != ZX_BP_NONE) {
                val tp = ZxFormBreakPoints.type[flag.toInt()]
                val addr1 = ZxWnd.zxFormatNumber(ZxWnd.read16(idx), ZX_FV_OPS16, true)
                val addr2 = ZxWnd.zxFormatNumber(ZxWnd.read16(idx + 2), ZX_FV_OPS16, true)
                return if(flag == ZX_BP_EXEC) {
                    "$pos\t\t$addr1\t-\t$addr2\t\t---\t\t---\t\t--\t\t$tp"
                } else {
                    val value = ZxWnd.zxFormatNumber(ZxWnd.read8(idx + 4), ZX_FV_OPS8, true)
                    val mask = ZxWnd.zxFormatNumber(ZxWnd.read8(idx + 5), ZX_FV_OPS8, true)
                    val cond = ZxFormBreakPoints.cond[ZxWnd.props[idx + 6].toInt()]
                    "$pos\t\t$addr1\t-\t$addr2\t\t$value\t\t$mask\t\t$cond\t\t$tp"
                }
            }
            return "$pos\t\t-----\t-\t-----\t\t---\t\t---\t\t--"
*/
            return "serg"
        }
        override fun getCount() = countDiskFiles

        override fun getItemId(position: Int) = -1L
    }
}
