package ru.ostrovskal.zx.forms

import android.view.LayoutInflater
import ru.ostrovskal.sshstd.Common.BTN_OK
import ru.ostrovskal.sshstd.Common.style_form
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Edit
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.zx.R
import ru.ostrovskal.zx.ZxCommon.ZX_CMD_POKE
import ru.ostrovskal.zx.ZxCommon.style_edit_zx
import ru.ostrovskal.zx.ZxWnd

@Suppress("unused")
class ZxFormPokes : Form() {

    private fun enabledSet() {
        val result = root.byIdx<Edit>(1).text.isNotBlank() && root.byIdx<Edit>(2).text.isNotBlank()
        root.byIdx<Tile>(3).isEnabled = result
    }

    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                root = cellLayout(12, 6, 1.dp) {
                    formHeader(R.string.pokeHead)
                    backgroundSet(style_form)
                    edit(0, R.string.hintAddress, style_edit_zx) {
                        maxLength = 5
                        changeTextLintener = { enabledSet() }
                    }.lps(0, 0, 7, 2)
                    edit(0, R.string.hintValue, style_edit_zx) {
                        changeTextLintener = { enabledSet() }
                        maxLength = 3
                    }.lps(7, 0, 5, 2)
                    button {
                        iconResource = R.integer.I_SET
                        isEnabled = false
                        setOnClickListener {
                            val a = root.byIdx<Edit>(1).text.toString().ival(0, 10)
                            val v = root.byIdx<Edit>(2).text.toString().ival(0, 10)
                            ZxWnd.zxCmd(ZX_CMD_POKE, a, v, "")
                        }
                    }.lps(0, 2, 6, 2)
                    button {
                        iconResource = R.integer.I_CLOSE
                        setOnClickListener {
                            footer(BTN_OK, 0)
                        }
                    }.lps(6, 2, 6, 2)
                }.lps(270.dp, 150.dp)
            }
        }
    }
}