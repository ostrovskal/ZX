package ru.ostrovskal.zx

import android.content.Context
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.ColorDrawable
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.ViewManager
import ru.ostrovskal.sshstd.Common
import ru.ostrovskal.sshstd.Common.MATCH
import ru.ostrovskal.sshstd.Common.WRAP
import ru.ostrovskal.sshstd.Wnd
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.layouts.CommonLayout
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Edit
import ru.ostrovskal.sshstd.widgets.Text
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.zx.ZxCommon.*

class ZxDebugger {

    // главный список
    private var list: ListLayout?    = null

    // редактор асм комманды
    private lateinit var asm: Edit

    // ссылка на активити
    private lateinit var wnd: Wnd

    // корень
    private var root: ViewGroup?    = null

    // количество элементов в списке
    private var countItems          = 0

    // выделенный элемент списка
    private var currentItem         = 0

    // точка входа в дизасм
    private var entryPC             = 0

    // точка входа в стек
    private var entrySP             = 0

    // точка входа в данные
    private var entryData           = 0

    // режим отображения в списке(0 - дизасм, 1 - стек, 2 - данные)
    private var listMode            = 0

    // текущая позиция в навигации по истории ПС
    private var posPC               = 0

    // Количество элементов в истории ПС
    private var countPC               = 0

    // история ПС
    private val storyPC             = mutableListOf(0)

    private fun ViewManager.debuggerRibbon(init: ListLayout.() -> Unit) =
        uiView({ ListLayout(it) }, init)

    private fun ViewManager.debuggerView() = uiView({ ItemView(it) }, {} )

    private class ItemView(context: Context): Text(context, style_debugger_item) {
        init {
            id = R.id.button1
            typeface = Typeface.MONOSPACE
            background = ColorDrawable(0)
            setOnClickListener { (parent as? ListLayout)?.notify(this, true) }
            setOnLongClickListener {(parent as? ListLayout)?.notify(this, false); true }
        }
    }

    private inner class ListLayout(context: Context) : CommonLayout(context, true) {

        override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec)
            // определить количество элементов
            val hItem = getChildAt(0).measuredHeight
            countItems = ((measuredHeight - verticalPadding) / hItem) + 1
            "onMeasure $hItem $countItems".info()
        }

        fun notify(item: ItemView, click: Boolean) {
            val idx = indexOfChild(item)
            update(ZX_SEL)
        }
    }

    private class ZxRegister(@JvmField val fmt: Int, @JvmField val id: Int, @JvmField val nm: Int,
                             @JvmField val offs: Int, @JvmField val msk: Int = 0xffff, @JvmField val shift: Int = 0,
                             @JvmField var oval: Int = 0,
                             @JvmField var text: Text? = null, @JvmField var edit: Edit? = null)

    private val registers = arrayOf(
        ZxRegister(ZX_FV_SIMPLE, R.id.edit1, R.string.flagS,     ZX_CPU_F,    0x80, 7),
        ZxRegister(ZX_FV_SIMPLE, R.id.edit2, R.string.flagZ,     ZX_CPU_F,    0x40, 6),
        ZxRegister(ZX_FV_SIMPLE, R.id.edit3, R.string.flagY,     ZX_CPU_F,    0x20, 5),
        ZxRegister(ZX_FV_SIMPLE, R.id.edit4, R.string.flagH,     ZX_CPU_F,    0x10, 4),
        ZxRegister(ZX_FV_SIMPLE, R.id.edit5, R.string.flagX,     ZX_CPU_F,    0x08, 3),
        ZxRegister(ZX_FV_SIMPLE, R.id.edit6, R.string.flagP,     ZX_CPU_F,    0x04, 2),
        ZxRegister(ZX_FV_SIMPLE, R.id.edit7, R.string.flagN,     ZX_CPU_F,    0x02, 1),
        ZxRegister(ZX_FV_SIMPLE, R.id.edit8, R.string.flagC,     ZX_CPU_F,    0x01, 0),
        ZxRegister(ZX_FV_OPS8, R.id.edit9,   R.string.ram,   ZX_CPU_RAM,  0xff),
        ZxRegister(ZX_FV_OPS8, R.id.edit10,  R.string.rom,   ZX_CPU_ROM,  0xff),
        ZxRegister(ZX_FV_OPS8, R.id.edit11,  R.string.vid,   ZX_CPU_VID,  0xff),
        ZxRegister(ZX_FV_OPS8, R.id.edit12,  R.string.rr,     ZX_CPU_RR,   0xff),
        ZxRegister(ZX_FV_OPS8, R.id.edit13,  R.string.ay,    ZX_CPU_AY,   0xff),
        ZxRegister(ZX_FV_OPS8, R.id.edit14,  R.string.ri,     ZX_CPU_RI,   0xff),
        ZxRegister(ZX_FV_OPS16, R.id.edit15, R.string.raf1,    ZX_CPU_AF1),
        ZxRegister(ZX_FV_OPS16, R.id.edit16, R.string.raf2,   ZX_CPU_AF2),
        ZxRegister(ZX_FV_OPS16, R.id.edit17, R.string.rhl1,    ZX_CPU_HL1),
        ZxRegister(ZX_FV_OPS16, R.id.edit18, R.string.rhl2,   ZX_CPU_HL2),
        ZxRegister(ZX_FV_OPS16, R.id.edit19, R.string.rde1,    ZX_CPU_DE1),
        ZxRegister(ZX_FV_OPS16, R.id.edit20, R.string.rde2,   ZX_CPU_DE2),
        ZxRegister(ZX_FV_OPS16, R.id.edit21, R.string.rbc1,    ZX_CPU_BC1),
        ZxRegister(ZX_FV_OPS16, R.id.edit22, R.string.rbc2,   ZX_CPU_BC2),
        ZxRegister(ZX_FV_OPS16, R.id.edit23, R.string.rix,    ZX_CPU_IX),
        ZxRegister(ZX_FV_OPS16, R.id.edit24, R.string.riy,    ZX_CPU_IY),
        ZxRegister(ZX_FV_OPS16, R.id.edit25, R.string.rpc,    ZX_CPU_PC),
        ZxRegister(ZX_FV_OPS16, R.id.edit26, R.string.rsp,    ZX_CPU_SP)
    )

    private val coordsLand = byteArrayOf(  33, 0, 3, 2,    36, 0, 4, 2,     40, 0, 3, 2,     43, 0, 3, 2,     46, 0, 3, 2,
                                                    33, 2, 3, 2,    36, 2, 4, 2,    40, 2, 3, 2,    43, 2, 3, 2,    46, 0, 3, 2,
                                                    24, 13, 2, 1,   32, 13, 2, 1,   32, 14, 2, 1,
                                                    0, 0, 18, 15,
                                                    18, 4, 2, 1,   18, 5, 2, 1,   20, 4, 2, 1,   20, 5, 2, 1,   22, 4, 2, 1,   22, 5, 2, 1,
                                                    24, 4, 2, 1,   24, 5, 2, 1,   26, 4, 2, 1,   26, 5, 2, 1,   28, 4, 2, 1,   28, 5, 2, 1,
                                                    30, 4, 2, 1,   30, 5, 2, 1,   32, 4, 2, 1,   32, 5, 2, 1,   34, 4, 2, 1,   34, 5, 2, 1,
                                                    18, 6, 2, 1,    20, 6, 3, 1,    23, 6, 2, 1,    25, 6, 3, 1,    28, 6, 2, 1,    30, 6, 3, 1,
                                                    18, 7, 2, 1,    20, 7, 3, 1,    23, 7, 2, 1,    25, 7, 3, 1,    28, 7, 2, 1,    30, 7, 3, 1,
                                                    18, 8, 2, 1,    20, 8, 6, 1,    26, 8, 2, 1,    28, 8, 6, 1,
                                                    18, 9, 2, 1,    20, 9, 6, 1,    26, 9, 2, 1,    28, 9, 6, 1,
                                                    18, 10, 2, 1,   20, 10, 6, 1,   26, 10, 2, 1,   28, 10, 6, 1,
                                                    18, 11, 2, 1,   20, 11, 6, 1,   26, 11, 2, 1,   28, 11, 6, 1,
                                                    18, 12, 2, 1,   20, 12, 6, 1,   26, 12, 2, 1,   28, 12, 6, 1,
                                                    18, 13, 2, 1,   20, 13, 4, 1,   26, 13, 2, 1,   28, 13, 4, 1,
                                                    18, 14, 14, 2)
    private val coordsPort = byteArrayOf(0)

    fun layout(wnd: Wnd, ui: CellLayout) = with(ui) {
        val iconAction = listOf(R.integer.I_PREV, R.integer.I_BP, R.integer.I_NEXT, R.integer.I_LIST_BP, R.integer.I_PLAY,
                                R.integer.I_HEX, R.integer.I_DA, R.integer.I_TRACE_IN, R.integer.I_TRACE_OUT, R.integer.I_TRACE_OVER)
        this@ZxDebugger.wnd = wnd
        cellLayout(34, 15) {
            backgroundSet(Common.style_form)
            root = this
            val coord = if(config.portrait) coordsLand else coordsLand
            // button action
            repeat(2) { y ->
                var dx = 0
                repeat(5) { x ->
                    if(x == 2) dx = 1
                    button(style_debugger_action) {
                        setOnClickListener { clickAction(it)  }
                        iconResource = iconAction[y * 5 + x]
                    }.lps(18 + dx + x * 3, y * 2, if(x == 1) 4 else 3, 2)
                }
            }
            button(style_debugger_action) {
                setOnClickListener { clickAction(it)  }
                iconResource = R.integer.I_PLAY }.lps(24, 13, 2, 1)
            button(style_debugger_action) {
                setOnClickListener { clickAction(it)  }
                iconResource = R.integer.I_PLAY }.lps(32, 13, 2, 1)
            button(style_debugger_action) {
                setOnClickListener { clickAction(it)  }
                iconResource = R.integer.I_PLAY }.lps(32, 14, 2, 1)
            list = debuggerRibbon {
                padding = 6.dp
                backgroundSet(style_backgrnd_io)
                repeat(25) { debuggerView().lps(MATCH, WRAP) }
            }.lps(0, 0, 18, 15)
            // flags
            repeat(8) {
                registers[it].text = text(registers[it].nm, style_debugger_text).lps(18 + it * 2, 4, 2, 1)
                registers[it].edit = edit(registers[it].id, R.string.null_text, style_debugger_edit) { maxLength = 1 }.lps(18 + it * 2, 5, 2, 1)
            }
            // registers
            // 8 bit
            repeat(2) { y ->
                repeat(3) { x ->
                    // text // edit
                    val pos = 8 + y * 3 + x
                    registers[pos].text = text(registers[pos].nm, style_debugger_text).lps(18 + x * 5, 6 + y, 2, 1)
                    registers[pos].edit = edit(registers[pos].id, R.string.null_text, style_debugger_edit) { maxLength = 4 }.lps(20 + x * 5, 6 + y, 3, 1)
                }
            }
            // RAM ROM VID  | I IM R | AY q
            // 16 bit
            repeat(6) { y ->
                repeat(2) { x ->
                    // text // edit
                    val pos = 14 + y * 2 + x
                    registers[pos].text = text(registers[pos].nm, style_debugger_text).lps(18 + x * 8, 8 + y, 2, 1)
                    registers[pos].edit = edit(registers[pos].id, R.string.null_text, style_debugger_edit) { maxLength = 5 }.lps(20 + x * 8, 8 + y, if(y == 5) 4 else 6, 1)
                }
            }
            // assembler
            asm = edit(R.id.edit27, R.string.null_text, style_debugger_edit) { maxLength = 40; gravity = Gravity.CENTER_VERTICAL }.lps(18, 14, 14, 2)
        }
    }

    fun update(flags: Int) {
        "debugger.update($flags)".info()
        // блокировка/разблокировка кнопок
        enabledButtons()
        // обновление регистров
        if(flags test ZX_REG) {
            registers.forEach {reg ->
                var nval = ZxWnd.zxInt(reg.offs, reg.msk,true, 0).toInt()
                nval = nval shr reg.shift
                // ставим признак модификации регистра
                reg.text?.setTextColor(if(nval == reg.oval) Color.WHITE else Color.RED)
                reg.oval = nval
                // устанавливаем значение регистра
                val sval = ZxWnd.zxFormatNumber(nval, reg.fmt, true)
                reg.edit?.setText(sval)
            }
        }
        if(flags test ZX_PC) {
            // читаем текущий PC
            entryPC = ZxWnd.zxInt(ZX_CPU_PC, 0xffff, true, 0).toInt()
            currentItem = 0
            // добавляем в историю
        }
        if(flags test ZX_SP) {
            entrySP = ZxWnd.zxInt(ZX_CPU_SP, 0xffff, true, 0).toInt()
        }
        if(flags test ZX_SEL) {
//            curSel = SEL;
        }
        if(flags test ZX_LIST) {
            var pc = entryPC
            val count = list?.childCount ?: 0
            repeat(count) { item ->
                // возврат строки текста, в зависимости от назначения:
                // 1. строка дизасма - ПС, флаги - новый ПС
                // 2. стек - СП - новый СП
                // 3. данные - адрес
                val flags = (ZxWnd.props[ZX_PROP_SHOW_ADDRESS].toInt() * DA_PC) or
                            (ZxWnd.props[ZX_PROP_SHOW_CODE].toInt() * DA_CODE) or
                            (ZxWnd.props[ZX_PROP_SHOW_CODE_VALUE].toInt() * (DA_PNN or DA_PN))
                val str = ZxWnd.zxDebuggerString(0, pc, flags)
                pc += ZxWnd.props[ZX_PROP_JNI_RETURN_VALUE].toInt()
                (list?.getChildAt(item) as? Text)?.text = str
            }
        }
        list?.invalidate()
    }

    private fun clickAction(view: View) {
        when(val cmd = root?.indexOfChild(view) ?: -1) {
            DEBUGGER_ACT_PREV      -> {

            }
            DEBUGGER_ACT_NEXT      -> {

            }
            DEBUGGER_ACT_BP        -> {
            }
            DEBUGGER_ACT_BP_LIST   -> {
                wnd.instanceForm(FORM_BREAK_POINTS)
            }
            DEBUGGER_ACT_ACTION    -> {
                val isExecute = ZxWnd.props[ZX_PROP_EXECUTE].toBoolean
                ZxWnd.props[ZX_PROP_EXECUTE] = if(isExecute) 0 else 1
                if(!isExecute) ZxWnd.zxCmd(ZX_CMD_STEP_DEBUG, 0, 0, "")
            }
            DEBUGGER_ACT_MODE      -> {
                listMode += 1
                if(listMode >= 3) listMode = 0
            }
            DEBUGGER_ACT_TRACE_IN,
            DEBUGGER_ACT_TRACE_OUT,
            DEBUGGER_ACT_TRACE_OVER -> {
                ZxWnd.zxCmd(ZX_CMD_TRACE_X, cmd - DEBUGGER_ACT_TRACE_IN, 0, "")
            }
            DEBUGGER_ACT_HEX_DEC    -> {
                ZxWnd.props[ZX_PROP_SHOW_HEX] = if(ZxWnd.props[ZX_PROP_SHOW_HEX].toBoolean) 0 else 1
            }
            DEBUGGER_ACT_SET_PC    -> {

            }
            DEBUGGER_ACT_SET_SP    -> {

            }
            DEBUGGER_ACT_SET_ASM   -> {

            }
        }
        update(ZX_ALL)
    }

    private fun enabledButtons() {
        val isPlay = !ZxWnd.props[ZX_PROP_EXECUTE].toBoolean
        val isDA = listMode == 0
        root?.apply {
            "debugger $isPlay".info()
            byIdx<Tile>(DEBUGGER_ACT_ACTION).iconResource = if(isPlay) R.integer.I_PLAY else R.integer.I_PAUSE
            byIdx<Tile>(DEBUGGER_ACT_MODE).tileIcon = listMode + 50
            byIdx<Tile>(DEBUGGER_ACT_HEX_DEC).iconResource = if(ZxWnd.props[ZX_PROP_SHOW_HEX].toBoolean) R.integer.I_HEX else R.integer.I_DEC
            byIdx<Tile>(DEBUGGER_ACT_TRACE_IN).isEnabled = isPlay && isDA
            byIdx<Tile>(DEBUGGER_ACT_TRACE_OUT).isEnabled = isPlay && isDA
            byIdx<Tile>(DEBUGGER_ACT_TRACE_OVER).isEnabled = isPlay && isDA
            byIdx<Tile>(DEBUGGER_ACT_PREV).isEnabled = isDA && posPC > 0
            byIdx<Tile>(DEBUGGER_ACT_NEXT).isEnabled = isDA && posPC < countPC
            byIdx<Tile>(DEBUGGER_ACT_BP).isEnabled = isDA
        }
   }
}