package ru.ostrovskal.zx

import android.graphics.Color
import android.os.Bundle
import android.text.InputType
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import ru.ostrovskal.sshstd.Common.*
import ru.ostrovskal.sshstd.Config
import ru.ostrovskal.sshstd.STORAGE
import ru.ostrovskal.sshstd.TileDrawable
import ru.ostrovskal.sshstd.Wnd
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Edit
import ru.ostrovskal.sshstd.widgets.Text
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.zx.ZxCommon.*

class ZxDebugger {
    companion object {
        val iconAction = listOf(R.integer.I_HEX, R.integer.I_UNDO, R.integer.I_PLAY, R.integer.I_REDO,
                                        R.integer.I_TRACE_IN, R.integer.I_TRACE_OUT, R.integer.I_TRACE_OVER,
                                        R.integer.I_BP, R.integer.I_LIST_BP, R.integer.I_PLAY)

        private val registers = arrayOf(
            ZxRegister(ZX_FV_SIMPLE, 0, R.string.flagS,     ZX_CPU_F,    0x80, 7),
            ZxRegister(ZX_FV_SIMPLE, 0, R.string.flagZ,     ZX_CPU_F,    0x40, 6),
            ZxRegister(ZX_FV_SIMPLE, 0, R.string.flagY,     ZX_CPU_F,    0x20, 5),
            ZxRegister(ZX_FV_SIMPLE, 0, R.string.flagH,     ZX_CPU_F,    0x10, 4),
            ZxRegister(ZX_FV_SIMPLE, 0, R.string.flagX,     ZX_CPU_F,    0x08, 3),
            ZxRegister(ZX_FV_SIMPLE, 0, R.string.flagP,     ZX_CPU_F,    0x04, 2),
            ZxRegister(ZX_FV_SIMPLE, 0, R.string.flagN,     ZX_CPU_F,    0x02, 1),
            ZxRegister(ZX_FV_SIMPLE, 0, R.string.flagC,     ZX_CPU_F,    0x01, 0),
            ZxRegister(ZX_FV_OPS8, R.id.edit2,  R.string.rom, ZX_CPU_ROM,0xff),
            ZxRegister(ZX_FV_OPS8, R.id.edit1,  R.string.ram,   ZX_CPU_RAM, 0xff),
            ZxRegister(ZX_FV_OPS8, R.id.edit2,  R.string.ri,   ZX_CPU_RI, 0xff),
            ZxRegister(ZX_FV_OPS16, R.id.edit3, R.string.raf,  ZX_CPU_AF),
            ZxRegister(ZX_FV_OPS16, R.id.edit4, R.string.rhl,  ZX_CPU_HL),
            ZxRegister(ZX_FV_OPS16, R.id.edit5, R.string.rde,  ZX_CPU_DE),
            ZxRegister(ZX_FV_OPS16, R.id.edit6, R.string.rbc,  ZX_CPU_BC),
            ZxRegister(ZX_FV_OPS16, R.id.edit7, R.string.rix,  ZX_CPU_IX),
            ZxRegister(ZX_FV_OPS16, R.id.edit8, R.string.riy,  ZX_CPU_IY),
            ZxRegister(ZX_FV_OPS16, R.id.edit9, R.string.rpc,  ZX_CPU_PC),
            ZxRegister(ZX_FV_OPS16, R.id.edit10, R.string.rsp,  ZX_CPU_SP)
        )
    }

    private class ZxRegister(@JvmField val fmt: Int, @JvmField val id: Int, @JvmField val nm: Int,
                             @JvmField val offs: Int, @JvmField val msk: Int = 0xffff, @JvmField val shift: Int = 0,
                             @JvmField var oval: Int = 0,
                             @JvmField var text: Text? = null, @JvmField var edit: Edit? = null)

    //
    private lateinit var wnd: Wnd

    // корень
    private lateinit var root: ViewGroup

    // селектор для флага
    private var selector: TileDrawable?     = null

    // редактор асм комманды
    private val asm                    by lazy { root.byIdx<Edit>(41) }

    // главный список
    private val list              by lazy { root.byIdx<ZxListView>(10) }

    // текущая позиция в навигации по истории ПС
    @STORAGE
    private var posPC                       = -1

    // Количество элементов в истории ПС
    @STORAGE
    private var countPC                     = 0

    // история ПС
    @STORAGE
    private val storyPC                     = IntArray(32) { 0 }

    fun layout(wnd: Wnd, ui: CellLayout) = with(ui) {
        this@ZxDebugger.wnd = wnd
        selector = TileDrawable(context, style_debugger_selector)
        val coord = if(Config.isPortrait) debuggerPort else debuggerLand
        var pos = 0; var idx = 0
        cellLayout(coord[pos++], coord[pos++]) {
            backgroundSet(style_form)
            root = this
            // button action
            repeat(10) {
                button(style_debugger_action) {
                    setOnClickListener { clickAction(it)  }
                    iconResource = iconAction[idx++]
                }.lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
                pos += 4
            }
            addUiView(ZxListView(context, true).apply {
                padding = 2.dp
                onNotifyParent = { _, event, data, sdata ->
                    when(event) {
                        ZxWnd.ZxMessages.ACT_DEBUGGER_LONG_CLOCK.ordinal    -> fromItem(data)
                        ZxWnd.ZxMessages.ACT_UPDATE_DEBUGGER.ordinal        -> this@ZxDebugger.update(0, ZX_LIST)
                        ZxWnd.ZxMessages.ACT_DEBUGGER_SELECT_ITEM.ordinal   -> { enabledButtons(); asm.setText(sdata) }
                    }
                }
                backgroundSet(style_backgrnd_io)
                addUiView(ZxListView.ItemView(R.id.button1, context).lps(MATCH, WRAP))
            }.lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3]))
            pos += 4
            // execFlags
            repeat(8) {
                registers[it].text = text(registers[it].nm, style_debugger_flags).
                    lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
                pos += 4
            }
            // registers 8 bit
            idx = 8
            repeat(3) {
                registers[idx].text = text(registers[idx].nm, style_debugger_text).
                    lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
                registers[idx].edit = edit(registers[idx].id, R.string.null_text, style_debugger_edit).
                    lps(coord[pos + 4], coord[pos + 5], coord[pos + 6], coord[pos + 7])
                pos += 8
                idx++
            }
            // registers 16 bit
            repeat(8) {
                registers[idx].text = text(registers[idx].nm, style_debugger_text).
                    lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
                registers[idx].edit = edit(registers[idx].id, R.string.null_text, style_debugger_edit).
                    lps(coord[pos + 4], coord[pos + 5], coord[pos + 6], coord[pos + 7])
                pos += 8
                idx++
            }
            // assembler
            edit(R.id.edit11, R.string.null_text, style_debugger_edit) {
                inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS or InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS
                maxLength = 40
                gravity = Gravity.CENTER_VERTICAL
            }.lps(coord[pos], coord[pos + 1], coord[pos + 2], coord[pos + 3])
        }
    }

    fun update(data: Int, flags: Int) {
        // обновление регистров
        if(flags test ZX_REG) {
            registers.forEach {reg ->
                val nval = (ZxWnd.read16(reg.offs) and reg.msk) shr reg.shift
                // ставим признак модификации регистра
                reg.text?.setTextColor(if(nval == reg.oval) Color.WHITE else Color.RED)
                reg.oval = nval
                // устанавливаем значение регистра
                if(reg.fmt == ZX_FV_SIMPLE) {
                    // execFlags
                    reg.text?.background = if(nval == 1) selector else null
                } else {
                    val sval = ZxWnd.zxFormatNumber(nval, reg.fmt, true)
                    reg.edit?.setText(sval)
                }
            }
        }
        list.update(data, flags)
        if(flags test ZX_STORY) setStoryPC(data)
        // блокировка/разблокировка кнопок
        enabledButtons()
    }

    private fun setStoryPC(data: Int) {
        // сначала найти такой же
        //if(storyPC.any { it == data }) return
        // добавить
        val size = storyPC.size - 1
        if(posPC >= size) {
            repeat(size) { storyPC[it] = storyPC[it + 1] }
            posPC = size - 1
        }
        storyPC[++posPC] = data
        countPC = posPC
    }

    private fun clickAction(view: View) {
        var flags = 0
        var data = 0
        when(val cmd = root.indexOfChild(view)) {
            DEBUGGER_ACT_NEXT,
            DEBUGGER_ACT_PREV      -> {
                posPC += if(cmd == DEBUGGER_ACT_PREV) -1 else 1
                data = storyPC[posPC]
                flags = ZX_PSL
            }
            DEBUGGER_ACT_BP        -> {
                ZxWnd.zxCmd(ZX_CMD_QUICK_BP, list.selItem, 0, "")
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
                    data = ZxWnd.read16(ZX_CPU_PC)
                    flags = ZX_ALL
                } else flags = ZX_STORY
                wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_NAME_PROG.ordinal)
            }
            DEBUGGER_ACT_TRACE_IN,
            DEBUGGER_ACT_TRACE_OUT,
            DEBUGGER_ACT_TRACE_OVER -> {
                ZxWnd.zxCmd(ZX_CMD_TRACE_X, cmd - DEBUGGER_ACT_TRACE_IN, 0, "")
                data = ZxWnd.read16(ZX_CPU_PC)
                flags = ZX_RSL or ZX_TRACE
            }
            DEBUGGER_ACT_HEX_DEC    -> {
                ZxWnd.props[ZX_PROP_SHOW_HEX] = if(ZxWnd.props[ZX_PROP_SHOW_HEX].toBoolean) 0 else 1
                flags = ZX_RL
            }
            DEBUGGER_ACT_SET_ASM   -> {
                val txt = asm.text.toString()
                if(list.mode == ZX_DEBUGGER_MODE_DT) {
                    val addr = txt.substringBefore(',')
                    // если адрес - это регистр
                    data = ZxWnd.zxCmd(ZX_CMD_VALUE_REG, 0, 0, addr)
                    if(addr != txt) {
                        // address, value
                        val value = txt.substringAfter(',')
                        val v = ZxWnd.zxStringToNumber(value, 0)
                        ZxWnd.zxCmd(ZX_CMD_POKE, data, v, "")
                    }
                    // если адрес лежит вне видимого диапазона - установить его
                    flags = ZX_DSL
                } else {
                    if(ZxWnd.zxCmd(ZX_CMD_ASSEMBLER, list.selItem, 0, txt) != 1) {
                        // error
                        val s = ZxWnd.read8(ZX_PROP_JNI_RETURN_VALUE)
                        val e = ZxWnd.read8(ZX_PROP_JNI_RETURN_VALUE + 1)
                        // выделить ошибку
                        asm.setSelection(s, e)
                    } else flags = ZX_LIST
                }
            }
        }
        if(flags != 0) update(data, flags)
    }

    private fun enabledButtons() {
        val isPlay = !ZxWnd.props[ZX_PROP_EXECUTE].toBoolean
        val isDA = list.mode == 0
        root.apply {
            byIdx<Tile>(DEBUGGER_ACT_ACTION).iconResource = if(isPlay) R.integer.I_PLAY else R.integer.I_PAUSE
            byIdx<Tile>(DEBUGGER_ACT_HEX_DEC).iconResource = if(ZxWnd.props[ZX_PROP_SHOW_HEX].toBoolean) R.integer.I_HEX else R.integer.I_DEC
            byIdx<Tile>(DEBUGGER_ACT_TRACE_IN).isEnabled = isPlay && isDA
            byIdx<Tile>(DEBUGGER_ACT_TRACE_OUT).isEnabled = isPlay && isDA
            byIdx<Tile>(DEBUGGER_ACT_TRACE_OVER).isEnabled = isPlay && isDA
            byIdx<Tile>(DEBUGGER_ACT_PREV).isEnabled = isDA && posPC > 0
            byIdx<Tile>(DEBUGGER_ACT_NEXT).isEnabled = isDA && posPC < countPC
            byIdx<Tile>(DEBUGGER_ACT_BP).isEnabled = isDA && list.selView != null
        }
    }

    private fun fromItem(data: Int) {
        var mode = list.mode
        val ret = ZxWnd.zxCmd(ZX_CMD_JUMP, data, mode, "")
        val dat = ZxWnd.read16(ZX_PROP_JNI_RETURN_VALUE)
        when(ret) {
            ZX_DEBUGGER_MODE_PC       -> {
                mode = ret
                update(dat, ZX_PYSL)
            }
            ZX_DEBUGGER_MODE_DT       -> {
                mode = ret
                update(dat, ZX_DSL)
            }
            else                      -> { update(data, ZX_SL); return }
        }
        list.mode = mode
    }

    fun save(state: Bundle) {
        state.putByteArray("debugger", marshall())
        state.putByteArray("list_debugger", list.marshall())
    }

    fun restore(state: Bundle) {
        state.getByteArray("debugger")?.let { unmarshall(it) }
        state.getByteArray("list_debugger")?.let { list.unmarshall(it) }
    }
}

/*

    fun showRegisters(canvas: Canvas, paint: Paint) {
        paint.textSize = 14f.sp
        val xx = 70f.sp
        val yy = 20f.sp
        val x1 = 47f.sp
        val x2 = 10f.sp
        repeat(2) { y ->
            val dy = 20f.sp + y * yy
            repeat(4) { x ->
                val idx = 11 + y * 4 + x
                registers[idx].apply {
                    paint.color = text?.currentTextColor ?: Color.GREEN
                    val dx = x * xx
                    canvas.drawText(edit?.text.toString(), x1 + dx, dy, paint)
                    paint.color = Color.GREEN
                    canvas.drawText(text?.text.toString(), x2 + dx, dy, paint)
                }
            }
        }
        paint.textSize = 24f.sp
    }
*/

