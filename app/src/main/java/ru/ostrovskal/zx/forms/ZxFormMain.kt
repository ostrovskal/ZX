@file:Suppress("DEPRECATION")

package ru.ostrovskal.zx.forms

import android.annotation.SuppressLint
import android.app.ActionBar
import android.os.Bundle
import android.os.Message
import android.view.LayoutInflater
import android.view.View
import android.widget.TextView
import ru.ostrovskal.sshstd.Common.RECEPIENT_FORM
import ru.ostrovskal.sshstd.Config
import ru.ostrovskal.sshstd.Surface
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.ui.cellLayout
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.zx.*
import ru.ostrovskal.zx.ZxCommon.ZX_RL

@Suppress("unused")
class ZxFormMain: Form() {

    override val surface: Surface? get() = zxview

    // Клавиатура
    private val keyboard                = ZxKeyboard()

    // Отладчик
    @JvmField val debugger              = ZxDebugger()

    // Разметка клавиатуры
    private var keyLyt: CellLayout?     = null

    // Разметка отладчика
    private var debLyt: CellLayout?     = null

    // поверхность
    private var zxview: ZxView?         = null

    override fun saveState(state: Bundle) {
        super.saveState(state)
        debugger.save(state)
    }

    override fun restoreState(state: Bundle) {
        super.restoreState(state)
        debugger.restore(state)
    }

    private fun updateLayout() {
        if(ZxWnd.props[ZxCommon.ZX_PROP_SHOW_DEBUGGER].toBoolean) {
            zxview?.layoutParams = CellLayout.LayoutParams(0, 0, 11, 7)
            keyLyt?.visibility = View.GONE; debLyt?.visibility = View.VISIBLE
            debLyt?.apply { visibility = View.VISIBLE }
        } else {
            val show = if (ZxWnd.props[ZxCommon.ZX_PROP_SHOW_KEY].toBoolean) View.VISIBLE else View.GONE
            val heightKeyboard = if (show == View.VISIBLE) ZxWnd.props[ZxCommon.ZX_PROP_KEY_SIZE] else 0
            zxview?.layoutParams = CellLayout.LayoutParams(0, 0, 11, 15 - heightKeyboard)
            keyLyt?.apply {
                visibility = show; debLyt?.visibility = View.GONE
                layoutParams = CellLayout.LayoutParams(0, 15 - heightKeyboard, 11, heightKeyboard.toInt())
            }
        }
        root.requestLayout()
        root.invalidate()
    }

    override fun inflateContent(container: LayoutInflater) = ui {
        root = cellLayout(11, 15) {
            addUiView( ZxView(ctx).apply { zxview = this; id = R.id.zxView } )
            debLyt = debugger.layout(wnd, this).lps(0, 7, 11, 8)
            keyLyt = keyboard.layout(this).lps(0, 10, 11, 4)
        }
        wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal)
        wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_KEY_BUTTONS.ordinal)
        wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_DEBUGGER.ordinal, a1 = 0, a2 = ZX_RL, delay = 100)
    }

    @SuppressLint("SetTextI18n")
    override fun handleMessage(msg: Message): Boolean {
        when(msg.action) {
            ZxWnd.ZxMessages.ACT_UPDATE_KEY_BUTTONS.ordinal -> keyboard.update()
            ZxWnd.ZxMessages.ACT_UPDATE_DEBUGGER.ordinal    -> debugger.update(msg.arg1, msg.arg2)
            ZxWnd.ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal -> updateLayout()
            ZxWnd.ZxMessages.ACT_IO_ERROR.ordinal           -> {
                ZxFormMessage().show(wnd, intArrayOf(R.string.app_name, msg.arg1, R.integer.I_YES, 0, 0, 0, 0))
            }
            ZxWnd.ZxMessages.ACT_UPDATE_NAME_PROG.ordinal   -> {
                wnd.actionBar?.apply {
                    if(customView == null) {
                        setCustomView(R.layout.actionbar_view)
                        displayOptions = ActionBar.DISPLAY_SHOW_CUSTOM or ActionBar.DISPLAY_SHOW_HOME
                    }
                    (customView as? TextView)?.apply {
                        val model = getString(ZxWnd.modelNames[ZxWnd.props[ZxCommon.ZX_PROP_MODEL_TYPE].toInt()])
                        text = if(Config.isPortrait) msg.obj.toString() else "$model - ${msg.obj}"
                    }
                }
            }
        }
        return true
    }
}
