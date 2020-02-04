package ru.ostrovskal.zx

import android.content.Context
import android.util.TypedValue
import android.view.ViewGroup
import ru.ostrovskal.sshstd.Common
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.ui.backgroundSet
import ru.ostrovskal.sshstd.ui.cellLayout
import ru.ostrovskal.sshstd.utils.addUiView
import ru.ostrovskal.sshstd.utils.sp
import ru.ostrovskal.sshstd.utils.sp2px
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.zx.ZxCommon.*

class ZxKeyboard {
    companion object {
        private val tiles           = arrayOfNulls<Tile?>(43)
    }
    private var root: ViewGroup?            = null

    fun update() {
        val rus = (ZxWnd.props[ZX_CPU_RL].toInt() shr 7) != 0
        tiles.forEachIndexed { index, tile ->
            tile?.apply {
                var pos = ZX_PROP_VALUES_BUTTON + index * 2
                val icon = ZxWnd.props[pos + 1].toInt()
                if(measuredWidth > 0) {
                    if (icon == 0) {
                        var text = ZxWnd.props[pos].toInt()
                        if (text < 0) text += 256
                        if(rus && text < 63) text += 192
                        setText(names[text])
                        tileIcon = -1
                    } else {
                        tileIcon = icon
                        text = ""
                    }
                }
            }
        }
    }

    private inner class KeyboardButton(context: Context) : Tile(context, style_key_button) {
        init {
            setOnTouchListener { v, event ->
                val index = ((parent as? CellLayout)?.indexOfChild(v) ?: -1) + 1
                if(ZxWnd.zxCmd(ZX_CMD_UPDATE_KEY, index, event.action, "") != 0)
                    update()
                false
            }
        }
        override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec)
            val mh = measuredWidth.coerceAtMost(measuredHeight).sp2px
            val hbutton = mh.sp * 0.4f
            setTextSize(TypedValue.COMPLEX_UNIT_PX, hbutton)
        }
    }

    fun layout(ui: CellLayout) = with(ui) {
        cellLayout(33, 4) {
            root = this
            backgroundSet(Common.style_form)
            repeat(43) { idx ->
                val pos = idx * 3
                addUiView(KeyboardButton(context).apply { tiles[idx] = this; lps(keyButtonPos[pos + 0], keyButtonPos[pos + 2], keyButtonPos[pos + 1], 1) } )
            }
        }
    }
}
