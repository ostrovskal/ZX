@file:Suppress("DEPRECATION")

package ru.ostrovskal.zx.forms

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

    private var rootPath    = folderFiles
    // список расщирений
    private val validExt by lazy { arguments.getString("filter")?.split(',') ?: listOf("") }

    // список файлов
    private var files               = listOf("")

    private fun updateFiles() { files = rootPath.listFoldersAndFiles(rootPath != folderFiles, validExt) }

    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                root = cellLayout(15, 15, 1.dp) {
                    formHeader(R.string.headerIO)
                    backgroundSet(style_form)
                    editEx(R.id.edit1, R.string.hintName, style_edit_zx) {
                        maxLength = 27
                        inputType = InputType.TYPE_CLASS_TEXT
                        changeTextLintener = {
                            name = it.toString()
                            val enabled = files.contains(name)
                            root.byIdx<Tile>(2).isEnabled = enabled
                            root.byIdx<Tile>(3).isEnabled = name.validFileExtensions(validExt) && !enabled
                            root.byIdx<Tile>(4).isEnabled = enabled
                        }
                    }.lps(0, 0, 10, 3)
                    button {
                        isEnabled = false
                        iconResource = R.integer.I_OPEN
                        setOnClickListener {
                            isEnabled.info()
                            val path = rootPath.substring(0, rootPath.length - 1).substringAfterLast(File.separatorChar) + File.separatorChar + name
                            (wnd as? ZxWnd)?.openMRU(MENU_MRU_10, path, false)
                            footer(BTN_OK, 0)
                        }
                        isEnabled.info()
                    }.lps(10, 1, 5, 3)
                    button {
                        isEnabled = false
                        iconResource = R.integer.I_SAVE
                        setOnClickListener {
                            val path = rootPath.substring(0, rootPath.length - 1).substringAfterLast(File.separatorChar) + File.separatorChar + name
                            footer(BTN_OK, 0)
                            wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_IO_SAVE.ordinal, o = path)
                        }
                    }.lps(10, 4, 5, 3)
                    button {
                        isEnabled = false
                        iconResource = R.integer.I_DELETE
                        setOnClickListener {
                            val path = rootPath + name
                            path.info()
                            File(path).delete()
                            root.byIdx<Edit>(1).setText("")
                            updateFiles()
                            root.byIdx<Ribbon>(6).adapter = ArrayListAdapter(context, ItemIO(), ItemIO(), files)
                        }
                    }.lps(10, 7, 5, 3)
                    button {
                        iconResource = R.integer.I_CLOSE
                        setOnClickListener { footer(BTN_OK, 0) }
                    }.lps(10, 10, 5, 3)
                    ribbon(R.id.ribbonMain) {
                        requestFocus()
                        itemClickListener = {_, v, _, _ ->
                            (v as? Text)?.apply {
                                var file = text.toString()
                                var isDir = true
                                when {
                                    file == "..." -> {
                                        rootPath = rootPath.substringBeforeLast(File.separatorChar)
                                        rootPath = rootPath.substringBeforeLast(File.separatorChar) + File.separatorChar
                                        file = ""
                                    }
                                    file[0] == '[' -> {
                                        // goto folder
                                        rootPath += file.substring(1, file.length - 1) + File.separatorChar
                                        file = ""
                                    }
                                    else -> isDir = false
                                }
                                if(isDir) {
                                    updateFiles()
                                    root.byIdx<Ribbon>(6).adapter = ArrayListAdapter(context, ItemIO(), ItemIO(), files)
                                }
                                root.byIdx<Edit>(1).setText(file)
                            }
                        }
                        padding = 7.dp
                        backgroundSet(style_backgrnd_io)
                        updateFiles()
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
}