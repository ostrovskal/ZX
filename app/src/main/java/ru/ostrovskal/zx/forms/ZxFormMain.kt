@file:Suppress("DEPRECATION")

package ru.ostrovskal.zx.forms

import android.annotation.SuppressLint
import android.app.ActionBar
import android.os.Bundle
import android.os.Message
import android.view.LayoutInflater
import android.view.View
import android.widget.LinearLayout
import android.widget.TextView
import ru.ostrovskal.sshstd.Common.MATCH
import ru.ostrovskal.sshstd.Common.RECEPIENT_FORM
import ru.ostrovskal.sshstd.Config
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.ui.cellLayout
import ru.ostrovskal.sshstd.ui.text
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.zx.*
import ru.ostrovskal.zx.ZxCommon.*

@Suppress("unused")
class ZxFormMain: Form() {

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
        if(ZxWnd.props[ZX_PROP_SHOW_DEBUGGER].toBoolean) {
            zxview?.layoutParams = CellLayout.LayoutParams(0, 0, 11, 7)
            keyLyt?.visibility = View.GONE; debLyt?.visibility = View.VISIBLE
            zxview?.updateJoy()
        } else {
            val show = if (ZxWnd.props[ZX_PROP_SHOW_KEY].toBoolean) View.VISIBLE else View.GONE
            val heightKeyboard = if (show == View.VISIBLE) ZxWnd.props[ZX_PROP_KEY_SIZE] else 0
            zxview?.layoutParams = CellLayout.LayoutParams(0, 0, 11, 15 - heightKeyboard)
            keyLyt?.apply {
                visibility = show; debLyt?.visibility = View.GONE
                layoutParams = CellLayout.LayoutParams(0, 15 - heightKeyboard, 11, heightKeyboard.toInt())
            }
        }
        root.requestLayout()
        root.invalidate()
        if(ZxWnd.props[ZX_PROP_SHOW_DEBUGGER].toBoolean)
            wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_DEBUGGER.ordinal, a1 = 0, a2 = ZX_RL, delay = 100)
        else if(ZxWnd.props[ZX_PROP_SHOW_KEY].toBoolean)
            wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_KEY_BUTTONS.ordinal, delay = 100)
    }

    override fun inflateContent(container: LayoutInflater) = ui {
        root = cellLayout(11, 15) {
            addUiView( ZxView(ctx).apply { zxview = this; id = R.id.zxView } )
            debLyt = debugger.layout(wnd, this).lps(0, 7, 11, 8)
            keyLyt = keyboard.layout(this).lps(0, 10, 11, 4)
        }
        wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal)
    }

    @SuppressLint("SetTextI18n")
    override fun handleMessage(msg: Message): Boolean {
        when(msg.action) {
            ZxWnd.ZxMessages.ACT_UPDATE_AUDIO.ordinal,
            ZxWnd.ZxMessages.ACT_UPDATE_FILTER.ordinal,
            ZxWnd.ZxMessages.ACT_RESET.ordinal,
            ZxWnd.ZxMessages.ACT_MODEL.ordinal,
            ZxWnd.ZxMessages.ACT_PRESS_MAGIC.ordinal,
            ZxWnd.ZxMessages.ACT_IO_LOAD.ordinal,
            ZxWnd.ZxMessages.ACT_IO_SAVE.ordinal            -> zxview?.callAction(msg)
            ZxWnd.ZxMessages.ACT_UPDATE_KEY_BUTTONS.ordinal -> keyboard.update()
            ZxWnd.ZxMessages.ACT_UPDATE_JOY.ordinal         -> zxview?.updateJoy()
            ZxWnd.ZxMessages.ACT_UPDATE_DEBUGGER.ordinal    -> {
                msg.obj?.apply {
                    val msk = msg.obj.toString().toBoolean()
                    ZxWnd.props[ZX_PROP_SHOW_DEBUGGER] = msk.toByte
                    if(msk) {
                        ZxWnd.props[ZX_PROP_ACTIVE_DEBUGGING] = 1
                        ZxWnd.modifyState(ZX_DEBUGGER, 0)
                    }
                    // спрятать/показать элемент клавы / вытащить/спрятать элемент отладчика
                    (wnd as? ZxWnd)?.apply {
                        menu.findItem(R.integer.MENU_KEYBOARD)?.isVisible = !msk
                        menu.findItem(R.integer.MENU_DEBUGGER1)?.isVisible = msk
                    }
                    wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal)
                }
                debugger.update(msg.arg1, msg.arg2)
            }
            ZxWnd.ZxMessages.ACT_UPDATE_MAIN_LAYOUT.ordinal -> updateLayout()
            ZxWnd.ZxMessages.ACT_IO_ERROR.ordinal           -> {
                ZxFormMessage().show(wnd, intArrayOf(R.string.app_name, msg.arg1, R.integer.I_YES, 0, 0, 280.dp, 160.dp))
            }
            ZxWnd.ZxMessages.ACT_UPDATE_NAME_PROG.ordinal   -> {
                wnd.actionBar?.apply {
                    if(customView == null) {
                        ui {
                            val cvv = text(R.string.null_text, style_action_bar) {
                                layoutParams = LinearLayout.LayoutParams(MATCH, MATCH)
                                setOnClickListener {
                                    ZxWnd.zxCmd(ZX_CMD_QUICK_SAVE, 0, 0, "")
                                }
                            }
                            customView = cvv
                        }
                        displayOptions = ActionBar.DISPLAY_SHOW_CUSTOM or ActionBar.DISPLAY_SHOW_HOME
                    }
                    (customView as? TextView)?.apply {
                        text = if (!ZxWnd.props[ZX_PROP_EXECUTE].toBoolean) {
                            "[PAUSE]"
                        } else {
                            val prgName = ZxWnd.zxProgramName("")
                            if (Config.isPortrait) {
                                prgName
                            } else {
                                val model = getString(ZxWnd.modelNames[ZxWnd.props[ZX_PROP_MODEL_TYPE].toInt()])
                                "$model - $prgName"
                            }
                        }
                    }
                }
            }
        }
        return true
    }

    fun activeGL(active: Boolean) {
        zxview?.apply { if(active) onResume() else onPause() }
    }
}
