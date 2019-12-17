package ru.ostrovskal.zx.forms

import android.view.LayoutInflater
import ru.ostrovskal.sshstd.Common
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.dp
import ru.ostrovskal.sshstd.utils.ui
import ru.ostrovskal.zx.R

@Suppress("unused")
class ZxFormTracer : Form() {
    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                root = cellLayout(15, 15, 1.dp) {
                    formHeader(R.string.headerTracer)
                    backgroundSet(Common.style_form)
                }.lps(Theme.dimen(ctx, R.dimen.widthTracer), Theme.dimen(ctx, R.dimen.heightTracer))
            }
        }
    }
}