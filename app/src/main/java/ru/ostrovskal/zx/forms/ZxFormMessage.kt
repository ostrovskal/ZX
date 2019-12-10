package ru.ostrovskal.zx.forms

import android.view.LayoutInflater
import ru.ostrovskal.sshstd.forms.FormMessage
import ru.ostrovskal.sshstd.ui.UiCtx
import ru.ostrovskal.sshstd.ui.backgroundSet
import ru.ostrovskal.zx.ZxCommon.style_form_message

class ZxFormMessage : FormMessage() {
    override fun inflateContent(container: LayoutInflater): UiCtx {
        return super.inflateContent(container).apply {
            root.backgroundSet(style_form_message)
        }
    }
/*

    override fun handleMessage(msg: Message): Boolean {
        wnd.findForm<ZxFormMain>("main")?.handleMessage(msg)
        return super.handleMessage(msg)
    }
*/
}