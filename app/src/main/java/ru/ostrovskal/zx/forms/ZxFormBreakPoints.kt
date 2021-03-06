package ru.ostrovskal.zx.forms

import android.content.Context
import android.graphics.Typeface
import android.view.LayoutInflater
import ru.ostrovskal.sshstd.Common.*
import ru.ostrovskal.sshstd.adapters.ArrayListAdapter
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Edit
import ru.ostrovskal.sshstd.widgets.lists.Ribbon
import ru.ostrovskal.sshstd.widgets.lists.Spinner
import ru.ostrovskal.zx.R
import ru.ostrovskal.zx.ZxCommon.*
import ru.ostrovskal.zx.ZxWnd

@Suppress("unused")
class ZxFormBreakPoints: Form() {
    companion object {
        val cond = listOf("==", "<>", ">", "<", ">=", "<=")
        val type = listOf("None", "Exec", "WMem", "RPort", "WPort")
    }

    private var isInner         = false
    private var isAddr2         = false

    override fun footer(btn: Int, param: Int) {
        wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_DEBUGGER.ordinal, a1 = 0, a2 = ZX_LIST)
        super.footer(btn, param)
    }

    override fun inflateContent(container: LayoutInflater): UiCtx {
        val idsBp = wnd.loadResource("idsBp", "array", IntArray(0))
        val hintBp = wnd.loadResource("hintsBp", "array", IntArray(0))
        return ui {
            linearLayout {
                root = cellLayout(12, 16) {
                    backgroundSet(style_form)
                    formHeader(R.string.headBps)
                    repeat(2) { y ->
                        repeat(2) { x ->
                            val pos = y * 2 + x
                            editEx(idsBp[pos], hintBp[pos], style_edit_zx) {
                                maxLength = if (pos > 1) 3 else 5
                                changeTextLintener = { _ ->
                                    if(!isInner) {
                                        if(pos == 1) isAddr2 = false
                                        updateBP()
                                    }
                                }
                            }.lps(x * 6, y * 2, 6, 2)
                        }
                    }
                    repeat(2) { x ->
                        spinner(idsBp[x + 4]) {
                            adapter = ArrayListAdapter(context, ZxFormSettings.Popup(), ZxFormSettings.Item(), if (x == 0) type else cond)
                            itemClickListener = { _, _, p, _ ->
                                if(x == 0) {
                                    val exec1 = p > 0
                                    val exec2 = p > 1
                                    root.byIdx<Edit>(1).isEnabled = exec1
                                    root.byIdx<Edit>(2).isEnabled = exec1
                                    root.byIdx<Edit>(3).isEnabled = exec2
                                    root.byIdx<Edit>(4).isEnabled = exec2
                                    root.byIdx<Spinner>(6).isEnabled = exec2
                                }
                                updateBP()
                            }
                        }.lps(x * 6, 4, 6, 2)
                    }
                    ribbon(R.id.ribbonMain, true, style_debugger_ribbon) {
                        adapter = BpAdapter(context, Item())
                        padding = 4.dp
                        backgroundSet(style_backgrnd_io)
                        selection = 0
                        itemClickListener = { _, _, p, _ ->
                            selection = p
                            clickRibbon()
                        }
                    }.lps(0, 6, 12, 6)
                    formFooter(BTN_OK, R.integer.I_YES)
                }.lps(Theme.dimen(ctx, R.dimen.widthBps), Theme.dimen(ctx, R.dimen.heightBps))
                clickRibbon()
            }
        }
    }

    private fun updateBP() {
        isInner = true

        val ribbon = root.byIdx<Ribbon>(7)
        val num = ribbon.selection
        val idx = num * 8 + 258

        val addr1 = root.byIdx<Edit>(1).text.toString()
        var addr2 = root.byIdx<Edit>(2).text.toString()
        val value = root.byIdx<Edit>(3).text.toString()
        val mask = root.byIdx<Edit>(4).text.toString()
        val flag = root.byIdx<Spinner>(5).selection.toByte()
        if(isAddr2) {
            addr2 = addr1
            root.byIdx<Edit>(2).setText(addr2)
        }
        val n1 = ZxWnd.zxStringToNumber(addr1, 0)
        val n2 = ZxWnd.zxStringToNumber(addr2, 0)
        var isError = (n2 < n1) || addr1.isBlank() || addr2.isBlank()
        if(flag != ZX_BP_EXEC && !isError) isError = value.isBlank() || mask.isBlank()
        ZxWnd.props[idx + 7] = if(isError) ZX_BP_NONE else {
            ZxWnd.zxInt(idx + 0, 0xffff, false, n1)
            ZxWnd.zxInt(idx + 2, 0xffff, false, n2)
            if(flag != ZX_BP_EXEC) {
                ZxWnd.props[idx + 4] = ZxWnd.zxStringToNumber(value, 0).toByte()
                ZxWnd.props[idx + 5] = ZxWnd.zxStringToNumber(mask, 0).toByte()
                ZxWnd.props[idx + 6] = root.byIdx<Spinner>(6).selection.toByte()
            }
            flag
        }
        // сразу записать
        "#bp$num".s = ZxWnd.zxSetProp((ZX_PROP_BPS - ZX_PROP_FIRST_LAUNCH) + num)
        ribbon.requestLayout()

        isInner = false
    }

    private fun clickRibbon() {
        isInner = true

        val pos = root.byIdx<Ribbon>(7).selection
        val idx = pos * 8 + 258
        val flag = ZxWnd.props[idx + 7]
        var addr1 = ""
        var addr2 = ""
        var value = ""
        var mask = ""
        var condition = 0
        if(flag != ZX_BP_NONE) {
            addr1 = ZxWnd.zxFormatNumber(ZxWnd.read16(idx), ZX_FV_SIMPLE, true)
            addr2 = ZxWnd.zxFormatNumber(ZxWnd.read16(idx + 2), ZX_FV_SIMPLE, true)
            if(flag != ZX_BP_EXEC) {
                value = ZxWnd.zxFormatNumber(ZxWnd.read8(idx + 4), ZX_FV_OPS8, true)
                mask = ZxWnd.zxFormatNumber(ZxWnd.read8(idx + 5), ZX_FV_OPS8, true)
                condition = ZxWnd.props[idx + 6].toInt()
            }
        }
        root.byIdx<Edit>(1).setText(addr1)
        root.byIdx<Edit>(2).setText(addr2)
        root.byIdx<Edit>(3).setText(value)
        root.byIdx<Edit>(4).setText(mask)
        root.byIdx<Spinner>(5).selection = flag.toInt()
        root.byIdx<Spinner>(6).selection = condition
        isAddr2 = true

        isInner = false
    }

    class Item : UiComponent() {
        override fun createView(ui: UiCtx) = ui.run { text(R.string.null_text, style_debugger_item).apply { padding = 3.dp; typeface = Typeface.MONOSPACE } }
    }

    inner class BpAdapter(context: Context, item: UiComponent) : ArrayListAdapter<String>(context, item, item, listOf("")) {
        override fun getItem(position: Int): String {
            val idx = position * 8 + 258
            val pos = position + 1
            val flag = ZxWnd.props[idx + 7]
            if(flag != ZX_BP_NONE) {
                val tp = type[flag.toInt()]
                val addr1 = ZxWnd.zxFormatNumber(ZxWnd.read16(idx), ZX_FV_OPS16, true)
                val addr2 = ZxWnd.zxFormatNumber(ZxWnd.read16(idx + 2), ZX_FV_OPS16, true)
                return if(flag == ZX_BP_EXEC) {
                    "$pos\t\t$addr1\t-\t$addr2\t\t---\t\t---\t\t--\t\t$tp"
                } else {
                    val value = ZxWnd.zxFormatNumber(ZxWnd.read8(idx + 4), ZX_FV_OPS8, true)
                    val mask = ZxWnd.zxFormatNumber(ZxWnd.read8(idx + 5), ZX_FV_OPS8, true)
                    val cond = cond[ZxWnd.props[idx + 6].toInt()]
                    "$pos\t\t$addr1\t-\t$addr2\t\t$value\t\t$mask\t\t$cond\t\t$tp"
                }
            }
            return "$pos\t\t-----\t-\t-----\t\t---\t\t---\t\t--"
        }

        override fun getCount() = 8

        override fun getItemId(position: Int) = -1L
    }
}
