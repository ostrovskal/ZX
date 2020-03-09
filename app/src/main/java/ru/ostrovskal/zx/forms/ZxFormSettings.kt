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
import ru.ostrovskal.sshstd.widgets.lists.Ribbon
import ru.ostrovskal.sshstd.widgets.lists.Spinner
import ru.ostrovskal.zx.R
import ru.ostrovskal.zx.ZxCommon.*
import ru.ostrovskal.zx.ZxPreset
import ru.ostrovskal.zx.ZxWnd
import java.util.*

@Suppress("unused")
class ZxFormSettings : Form() {

    // структура каталога диска
    private class DiskCatalog(val name: String, val type: String, val addr: Int, val len: Int, val count: Int, val sec: Int, val track: Int)

    companion object {

        // каталог диска
        private val catalog= MutableList(0) { DiskCatalog("", "", 0, 0, 0, 0, 0) }

        // количество блоков ленты
        private var countTapeBlocks             = 0

        // выбранный диск
        private var numDisk                     = 0

        // указатель на объект справки
        private var html: Html?                = null

        private val tapeTypes       = listOf("BASIC", "NumberArray", "StringArray", "Bytes")
        private val settingsChannAY = listOf("MONO", "ABC", "ACB", "BAC", "BCA", "CAB", "CBA")
        private val settingsChipAY  = listOf("AY", "YM")
        private val settingsFreq    = listOf("48000", "44100", "22050", "11025")
        private val namesDisk       = listOf("A: ", "B: ", "C: ", "D: ")

        private val settingsJoyTypes= listOf("KEMPSTON", "SINCLAIR I", "SINCLAIR II", "CURSOR", "CUSTOM")

        private val keyButtons      = listOf(
            "N/A", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
            // 11
            "DEL", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
            // 22
            "ENTER", "CAPS", "A", "S", "D", "F", "G", "H", "J", "K", "L",
            // 33
            "SYMBOL", "N/A", "Z", "X", "C", "SPACE", "V", "B", "N", "M",
            // 43
            "↑", "↓", "←", "→",
            // 47
            "K←", "K→", "K↑", "K↓", "K*")

        private val idNulls         = listOf(R.id.button1, R.id.button2, R.id.button3, R.id.button4, R.id.button5, R.id.button6, R.id.button7, R.id.button8)

        private val idSpinners      = listOf(  R.id.spinner2, R.id.spinner3, R.id.spinner4, R.id.spinner5, R.id.spinner6,
                                                        R.id.spinner7, R.id.spinner8, R.id.spinner9, R.id.spinner10, R.id.spinner11, R.id.spinner12)

        private val idSeeks         = listOf(R.id.seek1, R.id.seek2, R.id.seek3, R.id.seek4, R.id.seek5, R.id.seek6, R.id.seek7, R.id.seek8, R.id.seek9, R.id.seek10)

        private val rangeCommon= arrayOf(3..16, 2..7, 0..3, 3..9)
    }

    private var curView         = 0
    private var updateJoy       = false
    private var updateKey       = false
    private var updateSnd       = false
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
                    spinner(idSpinners[pos + 3]) {
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
        val spinns   = listOf(0, 2, 3, 3, 7, 3)
        var volBp: Seek? = null
        var volAy: Seek? = null
        cellLayout(10, 21) {
            repeat(3) { x ->
                text(textsSnd[x], style_text_settings).lps(x * 4, 0, 4, 3)
                spinner(idSpinners[x]) {
                    adapter = ArrayListAdapter(context, Popup(), Item(), when(x) { 0 -> settingsChipAY; 1-> settingsChannAY; else -> settingsFreq } )
                    itemClickListener = { _, _, p, _ ->
                        ZxWnd.props[settingsSnd[x]] = p.toByte()
                        updateSnd = true
                    }
                }.lps(spinns[x * 2], 3, spinns[x * 2 + 1], 4)
            }
            repeat(2) { x ->
                text(textsSnd[x + 3], style_text_settings).lps(x * 5, 9, 4, 3)
                val sk = seek(idSeeks[x], if(x == 0) 0..12 else 0..15, true) {
                    setOnClickListener {
                        ZxWnd.props[settingsSnd[x + 3]] = progress.toByte()
                    }
                }.lps(x * 5, 12, 5, 4)
                if(x == 0) volBp = sk else volAy = sk
            }
            repeat(2) { x ->
                check(idNulls[x + 2], textsSnd[x + 5]) {
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
                    catalog.clear()
                    var path = "disk$p".s
                    val isEmpty = path.isBlank()
                    if (isEmpty) path = context.getString(R.string.diskEmpty)
                    else {
                        // прочитать каталог
                        makeCatalog()
                    }
                    // в текст поставить выбранный диск
                    (root as? TabLayout)?.apply {
                        currentContent.byIdx<Text>(1).text = path
                        currentContent.byIdx<Ribbon>(2).adapter = DiskAdapter(context, DiskItem())
                    }
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

    private fun makeCatalog() {
        repeat(8) { sec ->
            ZxWnd.zxCmd(ZX_CMD_DISK_OPS, numDisk or (sec shl 3), ZX_DISK_OPS_RSECTOR, "")
            repeat(16) { idx ->
                val pos = idx * 16
                val marker = ZxWnd.props[ZX_PROP_VALUES_SECTOR + pos]
                if(marker == 0.toByte()) return
                if(marker >= 32.toByte()) {
                    // файл есть - 0-7 имя 8-тип 9,10-адрес 11,12-размер 13-количество секторов 14-первый сектор 15-первая дорожка
                    val name = StringBuilder(8)
                    repeat(8) { name.append(ZxWnd.props[ZX_PROP_VALUES_SECTOR + pos + it].toChar()) }
                    val type   = "<${ZxWnd.props[ZX_PROP_VALUES_SECTOR + pos + 8].toChar()}>"
                    val addr   = ZxWnd.read16(ZX_PROP_VALUES_SECTOR + pos + 9)
                    val length = ZxWnd.read16(ZX_PROP_VALUES_SECTOR + pos + 11)
                    val count  = ZxWnd.read8(ZX_PROP_VALUES_SECTOR + pos + 13)
                    val sector = ZxWnd.read8(ZX_PROP_VALUES_SECTOR + pos + 14)
                    val track  = ZxWnd.read8(ZX_PROP_VALUES_SECTOR + pos + 15)
                    catalog.add(DiskCatalog(name.toString(), type, addr, length, count, sector, track))
                }
            }
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
        countTapeBlocks = ZxWnd.zxCmd(ZX_CMD_TAPE_OPS, ZX_TAPE_OPS_COUNT, 0, "")
        cellLayout(10, 10) {
            ribbon(R.id.ribbonMain, false, style_debugger_ribbon) {
                adapter = TapeAdapter(context, TapeItem())
                padding = 4.dp
                backgroundSet(style_backgrnd_io)
                selection = 0
                itemClickListener = { _, _, p, _ ->
					ZxWnd.zxCmd(ZX_CMD_TAPE_OPS, ZX_TAPE_OPS_RESET, p, "");
					requestLayout()
                }
            }.lps(0, 0, 10, 8)
            check(R.id.button7, R.string.checkAutoStart) {
                isChecked = ZxWnd.props[ZX_PROP_BASIC_AUTOSTART].toBoolean
                setOnClickListener { ZxWnd.props[ZX_PROP_BASIC_AUTOSTART] = isChecked.toByte }
            }.lps(0, 8, 6, 2)
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
                    if (updateSnd) send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_AUDIO.ordinal)
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
        for (idx in 11 downTo 0) {
            val opt = settingsAllSnd[idx]
            if(opt > 0) {
                if(reset) {
                    val o = opt - ZX_PROP_FIRST_LAUNCH
                    ZxWnd.zxGetProp(settings[o].substringAfterLast(','), o)
                }
                val v = ZxWnd.props[opt].toInt()
                when (idx) {
                    1, 3, 5  -> content.byIdx<Spinner>(idx).selection   = v
                    7, 9     -> content.byIdx<Seek>(idx).progress       = v
                    10, 11   -> content.byIdx<Check>(idx).isChecked     = v != 0
                }
            }
        }
        if(reset) { updateSnd = true }
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

        //private val block       = ShortArray(10) { 0 }
        //private val pblock      = ShortArray(10) { 0 }

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
                ZxWnd.zxCmd(ZX_CMD_TAPE_OPS, ZX_TAPE_OPS_BLOCKC, position, "")
                ZxWnd.zxCmd(ZX_CMD_TAPE_OPS, ZX_TAPE_OPS_BLOCKP, position - 1, "")
                val flag    = ZxWnd.read16(ZX_PROP_VALUES_TAPE + 4)//block[2].toInt()
                val blk     = ZxWnd.read16(ZX_PROP_VALUES_TAPE + 6)
                val pflag   = ZxWnd.read16(ZX_PROP_VALUES_TAPE + 132)
                val pblk    = ZxWnd.read16(ZX_PROP_VALUES_TAPE + 134)
                byIdx<Text>(0).apply {
                    var sflag = ""
                    val caption = getString(when(flag) {
                        0   -> R.string.settingsTapeCaption
                        255 -> typeBlock(pblk, pflag, ZxWnd.read16(ZX_PROP_VALUES_TAPE + 138))
                        else-> { sflag = "($flag)"; R.string.settingsTapeBlock }
                    })
                    text = "$caption$sflag"
                    textColor = if(ZxWnd.read16(ZX_PROP_VALUES_TAPE + 14) != 0) Color.GRAY else Color.YELLOW
                }
                byIdx<Text>(1).text = ZxWnd.read16(ZX_PROP_VALUES_TAPE).toString()
                byIdx<Text>(2).text = ZxWnd.read16(ZX_PROP_VALUES_TAPE + 2).toString(16).toUpperCase(Locale.ROOT)
                byIdx<Text>(3).text = ""
                byIdx<Text>(4).text = ""
                byIdx<Text>(5).text = ""
                if(flag == 0) {
                    val nameP = String(ZxWnd.props, ZX_PROP_VALUES_TAPE + 20, 10)
                    byIdx<Text>(3).text = nameP
                    byIdx<Text>(4).text = getString(typeBlock(blk, flag, ZxWnd.read16(ZX_PROP_VALUES_TAPE + 10)))
                    byIdx<Text>(5).text = ZxWnd.read16(ZX_PROP_VALUES_TAPE + 8).toString()
                } else if(flag == 255) {
                    if(pflag == 0) {
                        val param = ZxWnd.read16(ZX_PROP_VALUES_TAPE + 138)
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
            if(position < catalog.size) {
                val file = catalog[position]
                val pos = position + 1
                val addr = ZxWnd.zxFormatNumber(file.addr, ZX_FV_OPS16, false)
                val length = ZxWnd.zxFormatNumber(file.addr, ZX_FV_OPS16, false)
                val count = ZxWnd.zxFormatNumber(file.count, ZX_FV_OPS8, false)
                val sector = ZxWnd.zxFormatNumber(file.sec, ZX_FV_OPS8, false)
                val track = ZxWnd.zxFormatNumber(file.track, ZX_FV_OPS8, false)
                if(Config.isPortrait) {
                    return "$pos\t${file.name}\t\t${file.type}\t\t$addr - $length\t\t$count\t$sector\t$track"
                }
                return "$pos\t\t${file.name}\t\t${file.type}\t\t$addr\t - \t$length\t\t$count\t\t$sector\t\t$track"
            }
            return ""
        }

        override fun getCount() = catalog.size

        override fun getItemId(position: Int) = -1L
    }
}
