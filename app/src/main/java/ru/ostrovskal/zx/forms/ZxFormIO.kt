@file:Suppress("DEPRECATION")

package ru.ostrovskal.zx.forms

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

    // корневая папка
    private var rootFolder          = folderFiles

    // относительный путь к выбранному файлу
    private var filePath                    = ""

    // список расщирений
    private val validExt by lazy { arguments.getString("filter")?.split(',') ?: listOf("") }

    // список файлов
    private var files           = listOf("")

    private fun updateFiles(updateRibbon: Boolean) {
        files = rootFolder.listFoldersAndFiles(rootFolder != folderFiles, validExt)
        if(updateRibbon) root.byIdx<Ribbon>(7).adapter = ArrayListAdapter(wnd, ItemIO(), ItemIO(), files)
    }

    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                root = cellLayout(15, 15, 1.dp) {
                    formHeader(R.string.headerIO)
                    backgroundSet(style_form)
                    editEx(R.id.edit1, R.string.hintName, style_edit_zx) {
                        maxLength = 40
                        changeTextLintener = {
                            val name = it.toString()
                            val enabled = files.contains(name)
                            filePath = rootFolder.removePrefix(folderFiles) + name
                            root.byIdx<Tile>(2).isEnabled = name.isNotBlank()
                            root.byIdx<Tile>(3).isEnabled = enabled
                            root.byIdx<Tile>(4).isEnabled = name.validFileExtensions(validExt) && !enabled
                            root.byIdx<Tile>(5).isEnabled = enabled
                        }
                    }.lps(0, 0, 10, 3)
                    button {
                        isEnabled = false
                        iconResource = R.integer.I_OPEN
                        setOnClickListener {
                            // создать папку
                            makeDirectories(filePath, FOLDER_FILES)
                            updateFiles(true)
                        }
                    }.lps(10, 1, 5, 2)
                    button {
                        isEnabled = false
                        iconResource = R.integer.I_BOX
                        setOnClickListener {
                            (wnd as? ZxWnd)?.openMRU(MENU_MRU_10, filePath, false)
                            footer(BTN_OK, 0)
                        }
                    }.lps(10, 4, 5, 2)
                    button {
                        isEnabled = false
                        iconResource = R.integer.I_SAVE
                        setOnClickListener {
                            footer(BTN_OK, 0)
                            wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_IO_SAVE.ordinal, o = filePath)
                        }
                    }.lps(10, 6, 5, 2)
                    button {
                        isEnabled = false
                        iconResource = R.integer.I_DELETE
                        setOnClickListener {
                            // проверить на удаление папки
                            var fp = filePath
                            if(filePath.last() == ']') {
                                fp = filePath.replace("[", "").replace("]", "")
                            }
                            File(folderFiles + fp).delete()
                            root.byIdx<Edit>(1).setText("")
                            updateFiles(true)
                        }
                    }.lps(10, 8, 5, 2)
                    button {
                        iconResource = R.integer.I_CLOSE
                        setOnClickListener { footer(BTN_OK, 0) }
                    }.lps(10, 10, 5, 2)
                    ribbon(R.id.ribbonMain) {
                        requestFocus()
                        itemClickListener = {_, v, _, _ ->
                            (v as? Text)?.apply {
                                var file = text.toString()
                                var isDir = true
                                when {
                                    file == "..." -> {
                                        rootFolder = rootFolder.substringBeforeLast(File.separatorChar).substringBeforeLast(File.separatorChar) + File.separatorChar
                                        file = ""
                                    }
                                    file[0] == '[' -> {
                                        // goto folder
                                        rootFolder += file.substring(1, file.length - 1) + File.separatorChar
                                        file = ""
                                    }
                                    else -> isDir = false
                                }
                                if(isDir) updateFiles(true)
                                root.byIdx<Edit>(1).setText(file)
                            }
                        }
                        padding = 7.dp
                        backgroundSet(style_backgrnd_io)
                        updateFiles(false)
                        adapter = ArrayListAdapter(wnd, ItemIO(), ItemIO(), files)
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