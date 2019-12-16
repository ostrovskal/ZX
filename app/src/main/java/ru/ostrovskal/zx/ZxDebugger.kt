package ru.ostrovskal.zx

import android.content.Context
import android.graphics.Color
import android.graphics.Typeface
import android.os.Bundle
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import ru.ostrovskal.sshstd.Common.*
import ru.ostrovskal.sshstd.STORAGE
import ru.ostrovskal.sshstd.Wnd
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Edit
import ru.ostrovskal.sshstd.widgets.Text
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.sshstd.widgets.lists.CommonRibbon
import ru.ostrovskal.zx.ZxCommon.*
import kotlin.math.abs
import kotlin.math.roundToInt
import kotlin.math.sign

class ZxDebugger {

    // главный список
    private var list: ListLayout?    = null

    // редактор асм комманды
    private lateinit var asm: Edit

    // ссылка на активити
    private lateinit var wnd: Wnd

    // корень
    private var root: ViewGroup?    = null

    // выделенное представление
    private var selView: ItemView?  = null

    // количество элементов на строку
    private var countData           = 0

    // количество элементов в списке
    private var countItems          = 0

    // ПС в поле ввода
    @STORAGE
    private var nPC                 = 0

    // ПС в поле ввода
    @STORAGE
    private var nSP                 = 0

    // выделенный элемент списка
    @STORAGE
    private var selItem             = 0

    // точка входа в дизасм
    @STORAGE
    private var entryPC             = 0

    // точка входа в стек
    @STORAGE
    private var entrySP             = 0

    // точка входа в данные
    @STORAGE
    private var entryData           = 0

    // режим отображения в списке(0 - дизасм, 1 - стек, 2 - данные)
    @STORAGE
    private var listMode            = 0

    // текущая позиция в навигации по истории ПС
    @STORAGE
    private var posPC               = -1

    // Количество элементов в истории ПС
    @STORAGE
    private var countPC             = 0

    // история ПС
    @STORAGE
    private val storyPC             = IntArray(32) { 0 }

    // ПС элементов
    @STORAGE
    private val itemAddr            = IntArray(32) { 0 }

    private class ItemView(context: Context): Text(context, style_debugger_item) {
        init {
            id = R.id.button1
            typeface = Typeface.MONOSPACE
            setOnClickListener { notify(false) }
            setOnLongClickListener { notify(true); true }
        }

        fun notify(long: Boolean) { (parent as? ListLayout)?.notify(this, long) }
    }

    private inner class ListLayout(context: Context) : CommonRibbon(context, true, style_debugger_ribbon) {

        private var mDeltaX              = 0

        init {
            setOnTouchListener { _, ev ->
                mTracker.addMovement(ev)
                touch.event(ev).drag(mDragSensitive) { offs, finish ->
                    mDeltaX += offs.w
                    val isVert = abs(offs.h) > abs(mDeltaX)
                    if (touch.isUnpressed) touch.flags = 0
                    if (!isVert) {
                        if(!finish) {
                            // меняем список
                            val delta = sign(mDeltaX.toFloat()).toInt()
                            listMode += delta
                            if (listMode < 0) listMode = 2
                            else if (listMode > 2) listMode = 0
                            update(0, ZX_LIST)
                            mDeltaX = 0
                        }
                    } else {
                        if(!finish) {
                            // запускаем прокрутку
                            mTouchId = touch.id
                            mTracker.computeCurrentVelocity(1000, mMaxVelocity)
                            val delta = mTracker.getYVelocity(mTouchId).toInt()
                            if (abs(delta) > mMinVelocity) mFling.start(-delta)
                        } else {
                            scrolling(offs.h / mDragSensitive.h)
                        }
                        mDeltaX = 0
                    }
                }
                true
            }
        }

        override fun onInterceptTouchEvent(ev: MotionEvent): Boolean {
            var delta = 0
            touch.event(ev).drag(mDragSensitive) { offs, _ ->
                delta = offs.w or offs.h
            }
            return delta != 0
        }

        override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec)
            // определить количество элементов
            val hItem = getChildAt(0).measuredHeight.toFloat()
            countItems = ((measuredHeight - verticalPadding) / hItem).roundToInt()
            mCount = childCount
            rectWithPadding(mRectList)
            mEdgeStart = mRectList.top
            mEdgeEnd = mRectList.bottom
            countData = ((((measuredWidth - horizontalPadding) / hItem) - 8) / 2f).roundToInt()
        }

        override fun scrolling(delta: Int): Boolean {
            var finish = false
            when(listMode) {
                ZX_DEBUGGER_MODE_PC       -> {
                    finish = ZxWnd.zxCmd(ZX_CMD_MOVE, entryPC, delta, "").toByte().toBoolean
                    entryPC = ZxWnd.read16(ZX_PROP_JNI_RETURN_VALUE)
                }
                ZX_DEBUGGER_MODE_SP       -> {
                    entrySP -= delta * 2
                    finish = correctSP()
                }
                ZX_DEBUGGER_MODE_DT       -> {
                    entryData -= delta * countData
                    finish = correctDT()
                }
            }
            update(0, ZX_LIST)
            return finish
        }

        fun notify(item: ItemView, long: Boolean) {
            val data = itemAddr[indexOfChild(item)]
            if(long) fromItem(data)
            else update(data, ZX_LIST or ZX_SEL)
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

    private val coordsLand = intArrayOf(
        33,
        18, 0, 3, 2,   21, 0, 3, 2,   24, 0, 3, 2,  27, 0, 3, 2,  30, 0, 3, 2,
        18, 2, 3, 2,   21, 2, 3, 2,   24, 2, 3, 2,  27, 2, 3, 2,
        24, 13, 2, 1,  31, 13, 2, 1,  31, 14, 2, 1,
        0, 0, 18, 15,
        18, 4, 2, 1,   18, 5, 2, 1,   20, 4, 2, 1,  20, 5, 2, 1,  22, 4, 2, 1, 22, 5, 2, 1,
        24, 4, 2, 1,   24, 5, 2, 1,   26, 4, 2, 1,  26, 5, 2, 1,  28, 4, 2, 1, 28, 5, 2, 1,
        30, 4, 2, 1,   30, 5, 2, 1,   32, 4, 2, 1,  32, 5, 2, 1,
        18, 6, 2, 1,   20, 6, 3, 1,   23, 6, 2, 1,  25, 6, 3, 1,  28, 6, 2, 1, 30, 6, 3, 1,
        18, 7, 2, 1,   20, 7, 3, 1,   23, 7, 2, 1,  25, 7, 3, 1,  28, 7, 2, 1, 30, 7, 3, 1,
        18, 8, 2, 1,   20, 8, 6, 1,   26, 8, 2, 1,  28, 8, 6, 1,
        18, 9, 2, 1,   20, 9, 6, 1,   26, 9, 2, 1,  28, 9, 6, 1,
        18, 10, 2, 1,  20, 10, 6, 1,  26, 10, 2, 1, 28, 10, 6, 1,
        18, 11, 2, 1,  20, 11, 6, 1,  26, 11, 2, 1, 28, 11, 6, 1,
        18, 12, 2, 1,  20, 12, 6, 1,  26, 12, 2, 1, 28, 12, 6, 1,
        18, 13, 2, 1,  20, 13, 4, 1,  26, 13, 2, 1, 28, 13, 3, 1,
        18, 14, 13, 2)

    private val coordsPort = intArrayOf(
        32,
        0, 0, 3, 2,    3, 0, 3, 2,    6, 0, 4, 2,   10, 0, 3, 2,  13, 0, 3, 2,
        16, 0, 4, 2,   20, 0, 4, 2,   24, 0, 4, 2,  28, 0, 4, 2,
        22, 6, 2, 1,   30, 6, 2, 1,   29, 7, 3, 1,
        0, 8, 34, 7,
        0, 2, 2, 1,    2, 2, 2, 1,    4, 2, 2, 1,   6, 2, 2, 1,   8, 2, 2, 1,   10, 2, 2, 1,
        12, 2, 2, 1,   14, 2, 2, 1,   16, 2, 2, 1,  18, 2, 2, 1,  20, 2, 2, 1,  22, 2, 2, 1,
        24, 2, 2, 1,   26, 2, 2, 1,   28, 2, 2, 1,  30, 2, 2, 1,

        1, 3, 2, 1,    3, 3, 3, 1,    6, 3, 2, 1,   8, 3, 3, 1,   11, 3, 2, 1,  13, 3, 3, 1,
        16, 3, 2, 1,   18, 3, 3, 1,   21, 3, 2, 1,  23, 3, 3, 1,  26, 3, 2, 1,  28, 3, 3, 1,

        0, 4, 2, 1,    2, 4, 6, 1,    8, 4, 2, 1,   10, 4, 6, 1,
        16, 4, 2, 1,   18, 4, 6, 1,   24, 4, 2, 1,  26, 4, 6, 1,
        0, 5, 2, 1,    2, 5, 6, 1,    8, 5, 2, 1,   10, 5, 6, 1,
        16, 5, 2, 1,   18, 5, 6, 1,   24, 5, 2, 1,  26, 5, 6, 1,
        0, 6, 2, 1,    2, 6, 6, 1,    8, 6, 2, 1,   10, 6, 6, 1,
        16, 6, 2, 1,   18, 6, 4, 1,   24, 6, 2, 1,  26, 6, 4, 1,
        0, 7, 29, 1)

    fun layout(wnd: Wnd, ui: CellLayout) = with(ui) {
        val iconAction = listOf(R.integer.I_HEX, R.integer.I_PREV, R.integer.I_BP, R.integer.I_NEXT, R.integer.I_LIST_BP,
                                R.integer.I_PLAY, R.integer.I_TRACE_IN, R.integer.I_TRACE_OUT, R.integer.I_TRACE_OVER,
                                R.integer.I_PLAY, R.integer.I_PLAY, R.integer.I_PLAY)
        this@ZxDebugger.wnd = wnd
        val coord = if(config.portrait) coordsPort else coordsLand
        var pos = 0; var idx = 0
        cellLayout(coord[pos++], 15) {
            backgroundSet(style_form)
            root = this
            // button action
            repeat(12) {
                button(style_debugger_action) {
                    setOnClickListener { clickAction(it)  }
                    iconResource = iconAction[idx++]
                }.lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
                pos += 4
            }
            addUiView(ListLayout(context).apply {
                list = this
                padding = 6.dp
                backgroundSet(style_backgrnd_io)
                repeat(15) { addUiView(ItemView(context).lps(MATCH, WRAP) ) }
            }.lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3]))
            pos += 4
            // flags
            repeat(8) {
                registers[it].text = text(registers[it].nm, style_debugger_text).
                    lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
                registers[it].edit = edit(registers[it].id, R.string.null_text, style_debugger_edit) { maxLength = 1 }.
                    lps(coord[pos + 4], coord[pos + 5], coord[pos + 6], coord[pos + 7])
                pos += 8
            }
            // registers 8 bit
            idx = 8
            repeat(6) {
                registers[idx].text = text(registers[idx].nm, style_debugger_text).
                    lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
                registers[idx].edit = edit(registers[idx].id, R.string.null_text, style_debugger_edit) { maxLength = 4 }.
                    lps(coord[pos + 4], coord[pos + 5], coord[pos + 6], coord[pos + 7])
                pos += 8
                idx++
            }
            // registers 16 bit
            idx = 14
            repeat(12) {
                registers[idx].text = text(registers[idx].nm, style_debugger_text).
                    lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
                registers[idx].edit = edit(registers[idx].id, R.string.null_text, style_debugger_edit) { maxLength = 5 }.
                    lps(coord[pos + 4], coord[pos + 5], coord[pos + 6], coord[pos + 7])
                pos += 8
                idx++
            }
            // assembler
            asm = edit(R.id.edit27, R.string.null_text, style_debugger_edit) { maxLength = 40; gravity = Gravity.CENTER_VERTICAL }.
                lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
        }
    }

    fun update(data: Int, flags: Int) {
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
                if(reg.id == R.id.edit25) nPC = nval
                else if(reg.id == R.id.edit26) nSP = nval
            }
            entrySP = nSP
            entrySP -= ((countItems + 1) and -2)
            correctSP()
        }
        if(flags test ZX_PC) {
            // читаем текущий PC
            entryPC = data
            // добавляем в историю
            if(flags test ZX_STORY) setStoryPC(data)
        }
        if(flags test ZX_DT) {
            entryData = data
            correctDT()
        }
        if(flags test ZX_SEL) {
            selItem = data
        }
        if(flags test ZX_LIST) {
            list?.let {
                val count = it.childCount
                var flg = 0
                selView?.setBackgroundColor(0)
                selView = null
                var entry = when(listMode) {
                    // 1. строка дизасма - ПС, флаги - новый ПС
                    ZX_DEBUGGER_MODE_PC     -> {
                        flg =   (ZxWnd.props[ZX_PROP_SHOW_ADDRESS].toInt() * DA_PC) or
                                (ZxWnd.props[ZX_PROP_SHOW_CODE].toInt() * DA_CODE) or
                                (ZxWnd.props[ZX_PROP_SHOW_CODE_VALUE].toInt() * (DA_PNN or DA_PN))
                        entryPC
                    }
                    // 2. стек - СП - новый СП
                    ZX_DEBUGGER_MODE_SP     -> { entrySP }
                    // 3. данные - адрес
                    ZX_DEBUGGER_MODE_DT    -> { flg = countData; entryData }
                    else                   -> error("Wrong debugger mode!")
                }
                repeat(count) { item ->
                    val str = ZxWnd.zxDebuggerString(listMode, entry, flg)
                    (it.getChildAt(item) as? Text)?.text = str
                    itemAddr[item] = entry
                    if(entry == selItem) selectItem(item)
                    entry += ZxWnd.read8(ZX_PROP_JNI_RETURN_VALUE)
                }
            }
        }
        list?.invalidate()
        // блокировка/разблокировка кнопок
        enabledButtons()
    }

    private fun correctSP(): Boolean {
        val entry = entrySP
        val delta = entry % 2
        if(entrySP < 0) entrySP = 0 + delta
        else if((entrySP + countItems * 2) > 65535)
            entrySP = (65536 - countItems * 2) + delta
        return entry != entrySP
    }

    private fun correctDT(): Boolean {
        val entry = entryData
        if(entryData < 0) entryData %= countData
        else if((entryData + countItems * countData) > 65535)
            entryData = (65536 - countItems * countData)
        return entry != entryData
    }

    private fun selectItem(idx: Int) {
/*        if(idx > (countItems / 2)) {
            entryPC = itemAddr[1]
            update(0, ZX_LIST)
        } else */
            selItem = itemAddr[idx]
            selView = list?.getChildAt(idx) as? ItemView
            selView?.setBackgroundColor(Color.BLUE)
    }

    private fun setStoryPC(data: Int) {
        // сначала найти такой же
        if(storyPC.any { it == data }) return
        // добавить
        val size = storyPC.size - 1
        if(posPC > size) {
            repeat(size) { storyPC[it] = storyPC[it + 1] }
            posPC = size
        }
        storyPC[++posPC] = data
        countPC = posPC
        "setStoryPC data: $data pos: $posPC count: $countPC list: ${storyPC.display()}".info()
    }

    private fun clickAction(view: View) {
        var flags = 0
        var data = 0
        when(val cmd = root?.indexOfChild(view) ?: -1) {
            DEBUGGER_ACT_NEXT,
            DEBUGGER_ACT_PREV      -> {
                posPC += if(cmd == DEBUGGER_ACT_PREV) -1 else 1
                data = storyPC[posPC]
                flags = ZX_PC or ZX_SEL or ZX_LIST
            }
            DEBUGGER_ACT_BP        -> {
                ZxWnd.zxCmd(ZX_CMD_QUICK_BP, selItem, 0, "")
                flags = ZX_LIST
            }
            DEBUGGER_ACT_BP_LIST   -> {
                wnd.instanceForm(FORM_BREAK_POINTS)
            }
            DEBUGGER_ACT_ACTION    -> {
                val isExecute = ZxWnd.props[ZX_PROP_EXECUTE].toBoolean
                ZxWnd.props[ZX_PROP_EXECUTE] = if(isExecute) 0 else 1
                if(!isExecute) ZxWnd.zxCmd(ZX_CMD_STEP_DEBUG, 0, 0, "")
                if(isExecute) {
                    data = nPC//ZxWnd.read16(ZX_CPU_PC)
                    flags = ZX_ALL
                } else flags = ZX_STORY
            }
            DEBUGGER_ACT_TRACE_IN,
            DEBUGGER_ACT_TRACE_OUT,
            DEBUGGER_ACT_TRACE_OVER -> {
                ZxWnd.zxCmd(ZX_CMD_TRACE_X, cmd - DEBUGGER_ACT_TRACE_IN, 0, "")
                data = nPC//ZxWnd.read16(ZX_CPU_PC)
                flags = ZX_LIST or ZX_REG or ZX_SEL
            }
            DEBUGGER_ACT_HEX_DEC    -> {
                ZxWnd.props[ZX_PROP_SHOW_HEX] = if(ZxWnd.props[ZX_PROP_SHOW_HEX].toBoolean) 0 else 1
                flags = ZX_LIST or ZX_REG
            }
            DEBUGGER_ACT_SET_PC    -> {

            }
            DEBUGGER_ACT_SET_SP    -> {

            }
            DEBUGGER_ACT_SET_ASM   -> {

            }
        }
        if(flags != 0) update(data, flags)
    }

    private fun enabledButtons() {
        val isPlay = !ZxWnd.props[ZX_PROP_EXECUTE].toBoolean
        val isDA = listMode == 0
        root?.apply {
            byIdx<Tile>(DEBUGGER_ACT_ACTION).iconResource = if(isPlay) R.integer.I_PLAY else R.integer.I_PAUSE
            byIdx<Tile>(DEBUGGER_ACT_HEX_DEC).iconResource = if(ZxWnd.props[ZX_PROP_SHOW_HEX].toBoolean) R.integer.I_HEX else R.integer.I_DEC
            byIdx<Tile>(DEBUGGER_ACT_TRACE_IN).isEnabled = isPlay && isDA
            byIdx<Tile>(DEBUGGER_ACT_TRACE_OUT).isEnabled = isPlay && isDA
            byIdx<Tile>(DEBUGGER_ACT_TRACE_OVER).isEnabled = isPlay && isDA
            byIdx<Tile>(DEBUGGER_ACT_PREV).isEnabled = isDA && posPC > 0
            byIdx<Tile>(DEBUGGER_ACT_NEXT).isEnabled = isDA && posPC < countPC
            byIdx<Tile>(DEBUGGER_ACT_BP).isEnabled = isDA && selView != null
        }
    }

    fun fromItem(data: Int) {
        val ret = ZxWnd.zxCmd(ZX_CMD_JUMP, data, listMode, "")
        val dat = ZxWnd.read16(ZX_PROP_JNI_RETURN_VALUE)
        when(ret) {
            ZX_DEBUGGER_MODE_PC       -> {
                listMode = ret
                update(dat, ZX_PC or ZX_LIST or ZX_SEL)
            }
            ZX_DEBUGGER_MODE_DT       -> {
                listMode = ret
                update(dat, ZX_DT or ZX_LIST or ZX_SEL)
            }
        }
        update(data, ZX_SEL or ZX_LIST)
    }

    fun save(state: Bundle) {
        val s = this.marshall()
        state.putByteArray("debugger", s)
    }

    fun restore(state: Bundle) {
        state.getByteArray("debugger")?.apply {
            this.unmarshall(this)
        }
    }
}
