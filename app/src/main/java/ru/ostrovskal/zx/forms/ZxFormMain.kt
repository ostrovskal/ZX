@file:Suppress("DEPRECATION")

package ru.ostrovskal.zx.forms

import android.annotation.SuppressLint
import android.app.ActionBar
import android.os.Message
import android.view.LayoutInflater
import android.view.View
import android.view.ViewManager
import android.widget.TextView
import ru.ostrovskal.sshstd.Common.RECEPIENT_FORM
import ru.ostrovskal.sshstd.Surface
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.forms.FormMessage
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.ui.backgroundSet
import ru.ostrovskal.sshstd.ui.cellLayout
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.zx.*
import ru.ostrovskal.zx.ZxCommon.ZX_CMD_UPDATE_KEY
import ru.ostrovskal.zx.ZxCommon.style_key_button

@Suppress("unused")
class ZxFormMain: Form() {

    override val surface: Surface? get()= zxview

    private inline fun ViewManager.zxView(init: ZxView.() -> Unit) = uiView({ ZxView(it) }, init)

    private val keyboard                = ZxKeyboard()

    // Клввиатура
    private var keyLyt: CellLayout?     = null

    // поверхность
    private var zxview: ZxView?         = null

    private fun updateLayout() {
        val show = if(ZxWnd.props[ZxCommon.ZX_PROP_SHOW_KEY].toBoolean) View.VISIBLE else View.GONE
        val heightKeyboard = if(show == View.VISIBLE) ZxWnd.props[ZxCommon.ZX_PROP_KEY_SIZE] else 0
        zxview?.layoutParams = CellLayout.LayoutParams(0, 0, 11, 15 - heightKeyboard)
        keyLyt?.apply {
            visibility = show
            layoutParams = CellLayout.LayoutParams(0, 15 - heightKeyboard, 11, heightKeyboard.toInt())
            if(show == View.VISIBLE) {
                var szTextButton = heightKeyboard * 2.5f
                if (szTextButton > 14f) szTextButton = 14f
                loopChildren { (it as? TextView)?.textSize = szTextButton }
            }
        }
        root.requestLayout()
        root.invalidate()
    }

    @SuppressLint("SetTextI18n")
    override fun handleMessage(msg: Message): Boolean {
        when(msg.action) {
            ZxWnd.ZxMessages.ACT_UPDATE_KEY_BUTTONS.ordinal -> keyboard.update()
            ZxWnd.ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal -> updateLayout()
            ZxWnd.ZxMessages.ACT_IO_ERROR.ordinal           -> {
                FormMessage().show(wnd, intArrayOf(R.string.app_name, msg.arg1, R.integer.I_YES, 0, 0, 0, 0))
            }
            ZxWnd.ZxMessages.ACT_UPDATE_NAME_PROG.ordinal   -> {
                wnd.actionBar?.apply {
                    if(customView == null) {
                        setCustomView(R.layout.actionbar_view)
                        displayOptions = ActionBar.DISPLAY_SHOW_CUSTOM or ActionBar.DISPLAY_SHOW_HOME
                    }
                    (customView as? TextView)?.apply {
                        val model = getString(ZxWnd.modelNames[ZxWnd.props[ZxCommon.ZX_PROP_MODEL_TYPE].toInt()])
                        text = if(config.portrait) "${msg.obj}" else "$model - ${msg.obj}"
                    }
                }
            }
        }
        return true
    }

    /** Реализация кнопки клавиатуры */
    private fun ViewManager.keyboardButton() = uiView({ Tile(it, style_key_button).apply {
        setOnTouchListener { v, event ->
            keyLyt?.apply { ZxWnd.zxCmd(ZX_CMD_UPDATE_KEY, indexOfChild(v) + 1, event.action, "") }
            false
        } } }, {} )

    override fun inflateContent(container: LayoutInflater) = ui {
        root = cellLayout(11, 15) {
            zxview = zxView { id = R.id.zxView; }
            keyLyt = cellLayout(11, 4) {
                backgroundSet { solid = 0xff203070.toInt() }
                repeat(4) {y ->
                    repeat(11) rep@{x ->
                        if(y == 3 && x > 8) return@rep
                        var xx = x
                        if(y == 3 && x > 4) xx += 2
                        keyboard.initButton(y * 11 + x + 1, keyboardButton().lps(xx, y, if(y == 3 && x == 4) 3 else 1, 1))
                    }
                }
            }.lps(0, 10, 11, 4)
        }
        wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal)
        wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_KEY_BUTTONS.ordinal)
    }
}
