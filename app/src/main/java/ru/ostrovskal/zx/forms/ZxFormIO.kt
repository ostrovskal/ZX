@file:Suppress("DEPRECATION")

package ru.ostrovskal.zx.forms

import android.os.Message
import android.text.InputType
import android.view.LayoutInflater
import ru.ostrovskal.sshstd.Common.*
import ru.ostrovskal.sshstd.adapters.ArrayListAdapter
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Edit
import ru.ostrovskal.sshstd.widgets.Text
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.sshstd.widgets.lists.Ribbon
import ru.ostrovskal.zx.R
import ru.ostrovskal.zx.ZxCommon.*
import ru.ostrovskal.zx.ZxWnd
import java.io.File

@Suppress("unused")
class ZxFormIO: Form() {

    // Имя выбранного
    private var name        = ""

    // список расщирений
    private val validExt    by lazy { arguments.getString("filter")?.split(',') ?: listOf("") }

    // список файлов
    private val files       by lazy { File(folderFiles).list()?.run {
                                filter { it.validFileExtensions(validExt) }.sorted().toMutableList() } ?: mutableListOf("") }

    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                root = cellLayout(15, 15, 1.dp) {
                    formHeader(R.string.headerIO)
                    backgroundSet(style_form)
                    editEx(R.id.editName, R.string.hintName, style_edit_zx) {
                        maxLength = 27
                        inputType = InputType.TYPE_CLASS_TEXT
                        changeTextLintener = {
                            val txt = it.toString()
                            val enabled = files.contains(txt)
                            root.byIdx<Tile>(2).isEnabled = enabled
                            root.byIdx<Tile>(3).isEnabled = txt.validFileExtensions(validExt) && !enabled
                            root.byIdx<Tile>(4).isEnabled = enabled
                        }
                    }.lps(0, 0, 10, 3)
                    button {
                        id = R.id.buttonNull1
                        isEnabled = false
                        iconResource = R.integer.I_OPEN
                        setOnClickListener {
                            (wnd as? ZxWnd)?.openMRU(R.id.menu_mru10, name, false)
                            footer(BTN_OK, 0)
                        }
                    }.lps(10, 1, 5, 3)
                    button {
                        id = R.id.buttonNull2
                        isEnabled = false
                        iconResource = R.integer.I_SAVE
                        setOnClickListener {
                            footer(BTN_OK, 0)
                            wnd.hand?.send(RECEPIENT_SURFACE_BG, ZxWnd.ZxMessages.ACT_IO_SAVE.ordinal, o = root.byId<Edit>(R.id.editName)?.text)
                        }
                    }.lps(10, 4, 5, 3)
                    button {
                        id = R.id.buttonNull3
                        isEnabled = false
                        iconResource = R.integer.I_DELETE
                        setOnClickListener {
                            val name = root.byIdx<Edit>(1).text.toString()
                            val path = folderFiles + File.separatorChar + name
                            files.remove(name)
                            File(path).delete()
                            root.byIdx<Edit>(1).setText("")
                            root.byIdx<Ribbon>(6).adapter = ArrayListAdapter(context, ItemIO(), ItemIO(), files)
                        }
                    }.lps(10, 7, 5, 3)
                    button {
                        id = R.id.buttonNull4
                        iconResource = R.integer.I_CLOSE
                        setOnClickListener { footer(BTN_OK, 0) }
                    }.lps(10, 10, 5, 3)
                    ribbon(R.id.ribbonMain) {
                        itemClickListener = {_, v, _, _ ->
                            (v as? Text)?.apply {
                                name = text.toString()
                                root.byIdx<Edit>(1).setText(name)
                            }
                        }
                        padding = 7.dp
                        backgroundSet(style_backgrnd_io)
                        adapter = ArrayListAdapter(context, ItemIO(), ItemIO(), files)
                    }.lps(0, 3, 10, 10)
                }.lps(Theme.dimen(ctx, R.dimen.ioWidth), Theme.dimen(ctx, R.dimen.ioHeight))
            }
        }
    }

    /** Класс, реализующий элемент списка файлов */
    private class ItemIO : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) { text(R.string.null_text, style_item_io) }
    }

    override fun handleMessage(msg: Message): Boolean {
        wnd.findForm<ZxFormMain>("main")?.handleMessage(msg)
        return super.handleMessage(msg)
    }
}