package ru.ostrovskal.zx.forms

import android.view.LayoutInflater
import ru.ostrovskal.sshstd.Common
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.addUiView
import ru.ostrovskal.sshstd.utils.dp
import ru.ostrovskal.sshstd.utils.padding
import ru.ostrovskal.sshstd.utils.ui
import ru.ostrovskal.zx.R
import ru.ostrovskal.zx.ZxCommon
import ru.ostrovskal.zx.ZxListView

@Suppress("unused")
class ZxFormTracer : Form() {
    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                root = cellLayout(15, 15, 1.dp) {
                    formHeader(R.string.headerTracer)
                    backgroundSet(Common.style_form)
                    button {
                        isEnabled = false
                        iconResource = R.integer.I_DELETE
                        setOnClickListener {
                        }
                    }.lps(2, 11, 5, 2)
                    button {
                        iconResource = R.integer.I_CLOSE
                        setOnClickListener { footer(Common.BTN_OK, 0) }
                    }.lps(8, 11, 5, 2)
                    addUiView(ZxListView(context, true).apply {
                        padding = 2.dp
                        backgroundSet(ZxCommon.style_backgrnd_io)
                        addUiView(ZxListView.ItemView(R.id.button1, context).lps(Common.MATCH, Common.WRAP))
                    }.lps(0, 0, 15, 11))
                }.lps(Theme.dimen(ctx, R.dimen.widthTracer), Theme.dimen(ctx, R.dimen.heightTracer))
            }
        }
    }
}