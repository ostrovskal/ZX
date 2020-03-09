package ru.ostrovskal.zx

import android.content.Context
import android.view.ViewGroup
import ru.ostrovskal.sshstd.TileDrawable
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.ui.backgroundSet
import ru.ostrovskal.sshstd.ui.cellLayout
import ru.ostrovskal.sshstd.utils.addUiView
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.zx.ZxCommon.*

class ZxKeyboard {
    companion object {
        private val tiles           = arrayOfNulls<Tile?>(43)
        private val nameKeyb         = listOf(  "keyboard_k", "keyboard_l", "keyboard_c", "keyboard_e",
                                                            "keyboard_ss_e", "keyboard_ss", "keyboard_cs_l", "keyboard_cs_k",
                                                            "keyboard_cs_g", "keyboard_g", "keyboard_cs_e")
    }
    private var root: ViewGroup?            = null

    fun update() {
        (root?.background as? TileDrawable)?.setBitmap(nameKeyb[ZxWnd.props[ZX_PROP_KEY_CURSOR_MODE].toInt()])
    }

    private inner class KeyboardButton(context: Context) : Tile(context, style_key_button) {
        init {
            setOnTouchListener { v, event ->
                if(ZxWnd.zxCmd(ZX_CMD_UPDATE_KEY, ((parent as? CellLayout)?.indexOfChild(v) ?: -1) + 1, event.action, "") != 0)
                    update()
                false
            }
        }
    }

    fun layout(ui: CellLayout) = with(ui) {
        cellLayout(62, 4, 0, false) {
            backgroundSet { setBitmap("keyboard_l") }
            root = this
            repeat(42) { idx ->
                val pos = idx * 3
                addUiView(KeyboardButton(context).apply { tiles[idx] = this; lps(keyButtonPos[pos + 0], keyButtonPos[pos + 1], keyButtonPos[pos + 2], 1) } )
            }
        }
    }
}
