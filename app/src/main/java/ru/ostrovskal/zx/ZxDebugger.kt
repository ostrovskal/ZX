package ru.ostrovskal.zx

import android.graphics.Typeface
import android.view.Gravity
import android.view.View
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.dp
import ru.ostrovskal.sshstd.utils.maxLength
import ru.ostrovskal.sshstd.utils.padding
import ru.ostrovskal.sshstd.utils.test
import ru.ostrovskal.sshstd.widgets.Edit
import ru.ostrovskal.sshstd.widgets.Text
import ru.ostrovskal.zx.ZxCommon.*

class ZxDebugger {
    private val iconAction  = listOf(R.integer.I_PREV, R.integer.I_BP, R.integer.I_NEXT, R.integer.I_LIST_BP, R.integer.I_PLAY,
                                     R.integer.I_UNDO, R.integer.I_DA, R.integer.I_REDO, R.integer.I_TRACE_IN, R.integer.I_TRACE_OUT)

    private class Item : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) {
            text(R.string.null_text, style_debugger_item).apply { typeface = Typeface.MONOSPACE }
        }
    }

    private class ZxRegister(@JvmField val id: Int, @JvmField val nm: Int, @JvmField val offs: Int,
                             @JvmField val msk: Int = 0xffff, @JvmField val shift: Int = 0,
                             @JvmField var value: Int = 0, @JvmField var text: Text? = null,
                             @JvmField var edit: Edit? = null)

    private val registers = arrayOf(
        ZxRegister(R.id.idS,   R.string.flagS,  ZX_CPU_F,    0x80, 7),
        ZxRegister(R.id.idZ,   R.string.flagZ,  ZX_CPU_F,    0x40, 6),
        ZxRegister(R.id.idY,   R.string.flagY,  ZX_CPU_F,    0x20, 5),
        ZxRegister(R.id.idH,   R.string.flagH,  ZX_CPU_F,    0x10, 4),
        ZxRegister(R.id.idX,   R.string.flagX,  ZX_CPU_F,    0x08, 3),
        ZxRegister(R.id.idP,   R.string.flagP,  ZX_CPU_F,    0x04, 2),
        ZxRegister(R.id.idN,   R.string.flagN,  ZX_CPU_F,    0x02, 1),
        ZxRegister(R.id.idC,   R.string.flagC,  ZX_CPU_F,    0x01, 0),
        ZxRegister(R.id.idRAM, R.string.ram,    ZX_CPU_RAM,  0xff),
        ZxRegister(R.id.idROM, R.string.rom,    ZX_CPU_ROM,  0xff),
        ZxRegister(R.id.idVID, R.string.vid,    ZX_CPU_VID,  0xff),
        ZxRegister(R.id.idRR,  R.string.rr,     ZX_CPU_RR,   0xff),
        ZxRegister(R.id.idAY,  R.string.ay,     ZX_CPU_AY,   0xff),
        ZxRegister(R.id.idIM,  R.string.state,  ZX_CPU_STATE,0xff),
        ZxRegister(R.id.idAF1, R.string.raf1,   ZX_CPU_AF1),
        ZxRegister(R.id.idAF2, R.string.raf2,   ZX_CPU_AF2),
        ZxRegister(R.id.idHL1, R.string.rhl1,   ZX_CPU_HL1),
        ZxRegister(R.id.idHL2, R.string.rhl2,   ZX_CPU_HL2),
        ZxRegister(R.id.idDE1, R.string.rde1,   ZX_CPU_DE1),
        ZxRegister(R.id.idDE2, R.string.rde2,   ZX_CPU_DE2),
        ZxRegister(R.id.idBC1, R.string.rbc1,   ZX_CPU_BC1),
        ZxRegister(R.id.idBC2, R.string.rbc2,   ZX_CPU_BC2),
        ZxRegister(R.id.idIX,  R.string.rix,    ZX_CPU_IX),
        ZxRegister(R.id.idIY,  R.string.riy,    ZX_CPU_IY),
        ZxRegister(R.id.idPC,  R.string.rpc,    ZX_CPU_PC),
        ZxRegister(R.id.idSP,  R.string.rsp,    ZX_CPU_SP)
    )

    fun layout(ui: CellLayout) = with(ui) {
        cellLayout(34, 15) {
            ribbon(R.id.ribbonMain, true, style_debugger_ribbon) {
                itemClickListener = {_, _, _, _ ->
                }
                padding = 7.dp
                backgroundSet(style_backgrnd_io)
//                adapter = ArrayDebuggerAdapter(context, Item(), ZxFormIO.ItemIO(), files)
            }.lps(0, 0, 18, 15)
            // button action
            repeat(2) { y ->
                var dx = 0
                repeat(5) { x ->
                    if(x == 2) dx = 1
                    button(style_debugger_action) {
                        setOnClickListener { clickAction(it);  }
                        iconResource = iconAction[y * 5 + x]
                    }.lps(18 + dx + x * 3, y * 2, if(x == 1) 4 else 3, 2)
                }
            }
            // flags
            repeat(8) {
                registers[it].text = text(registers[it].nm, style_debugger_flags).lps(18 + it * 2, 4, 2, 1)
                registers[it].edit = edit(registers[it].id, R.string.null_text, style_debugger_edit) { maxLength = 1 }.lps(18 + it * 2, 5, 2, 1)
            }
            // registers
            // AF AF' HL HL' DE DE' BC BC' IX IY PC SP
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
            // AF AF' HL HL' DE DE' BC BC' IX IY PC SP
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
            edit(R.id.editValue, R.string.null_text, style_debugger_edit) { maxLength = 40; gravity = Gravity.CENTER_VERTICAL }.lps(18, 14, 14, 2)
            button(style_debugger_action) {
                setOnClickListener { clickAction(it);  }
                iconResource = R.integer.I_PLAY }.lps(24, 13, 2, 1)
            button(style_debugger_action) {
                setOnClickListener { clickAction(it);  }
                iconResource = R.integer.I_PLAY }.lps(32, 13, 2, 1)
            button(style_debugger_action) {
                setOnClickListener { clickAction(it);  }
                iconResource = R.integer.I_PLAY }.lps(32, 14, 2, 1)
            update(255)
        }
    }

    fun update(flags: Int) {
        if(flags test ZX_REG) {

        }
        if(flags test ZX_PC) {

        }
        if(flags test ZX_SP) {

        }
        if(flags test ZX_SEL) {

        }
    }

    private fun clickAction(view: View) {

    }

}