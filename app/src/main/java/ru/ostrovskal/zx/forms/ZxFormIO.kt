@file:Suppress("DEPRECATION")

package ru.ostrovskal.zx.forms

import android.content.Context
import android.os.Message
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import ru.ostrovskal.sshstd.Common.*
import ru.ostrovskal.sshstd.DropBox
import ru.ostrovskal.sshstd.STORAGE
import ru.ostrovskal.sshstd.adapters.ArrayListAdapter
import ru.ostrovskal.sshstd.forms.FORM_PROGRESS_DOWNLOAD
import ru.ostrovskal.sshstd.forms.FORM_PROGRESS_WAIT
import ru.ostrovskal.sshstd.forms.Form
import ru.ostrovskal.sshstd.forms.FormProgress
import ru.ostrovskal.sshstd.objects.Theme
import ru.ostrovskal.sshstd.ui.*
import ru.ostrovskal.sshstd.utils.*
import ru.ostrovskal.sshstd.widgets.Check
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

    companion object {
        // Допустимые расщирения
        private val validNetExt     = listOf(".tap", ".z80", ".trd", ".fdi", ".scl", ".tga", ".wav")
    }

    // фоновый тред
    private var thread: Thread?             = null

    // Количество выбранных файлов
    @STORAGE
    private var checkedCount                = 0

    // Список уже загруженных
    private val filesLocal      by lazy { folderFiles.collectedFiles(validExt) }

    // Список в облаке
    private var filesCloud= mutableListOf<DropBox.FileInfo>()

    // Выбранные
    private var checked                     = BooleanArray(0) { false }

    // DropBox
    private val dbx                by lazy { DropBox("zx", getString(R.string.dropbox_token)) }

    private var isNet                       = false

    // признак инициализации сети
    private var isNetInit                  = false

    // корневая папка
    private var rootFolder          = folderFiles

    // относительный путь к выбранному файлу
    private var filePath                    = ""

    // список расщирений
    private val validExt        = listOf(".z80", ".tap", ".wav")

    // список файлов
    private var files           = listOf("")

    private fun updateFiles(updateRibbon: Boolean) {
        if(!isNet) files = rootFolder.listFoldersAndFiles(rootFolder != folderFiles, validExt)
        if(updateRibbon) root.byIdx<Ribbon>(7).adapter = if(isNet)
            LoadingAdapter(wnd, filesCloud) else ArrayListAdapter(wnd, ItemIO(), ItemIO(), files)
    }

    private fun downloadCheckedFiles() {
        // загрузить отмеченные файлы из облака
        wnd.launch {
            FormProgress().show(wnd, R.string.cloudDownload, FORM_PROGRESS_DOWNLOAD).doInBackground(ZxWnd.ZxMessages.ACT_DROPBOX_DOWNLOAD_FINISH.ordinal) { fp ->
                delay(500L)
                fp.maximum = checkedCount
                var idx = 1
                checked.forEachIndexed { index, b ->
                    if (b) {
                        val file = filesCloud[index]
                        withContext(Dispatchers.IO) {
                            var sep = ""
                            val name = file.name
                            when(validNetExt.indexOf(name.substring(name.lastIndexOf('.')))) {
                                0,6     -> sep = "TAPE/"
                                1,5     -> sep = "SNAPSHOT/"
                                2,3,4   -> sep = "IMAGES/"
                            }
                            dbx.download(file, folderFiles + sep + name)
                        }
                        fp.primary = idx++
                    }
                }
                delay(500)
                BTN_OK
            }
        }
    }

    override fun handleMessage(msg: Message): Boolean {
        if(msg.action == ZxWnd.ZxMessages.ACT_DROPBOX_LIST_FILES_FINISH.ordinal) {
            if(msg.arg1 == BTN_NO) {
                ZxFormMessage().show(wnd, intArrayOf(R.string.app_name, R.string.cloudConnectError, R.integer.I_YES, 0, 0, 0, 0))
            } else {
                updateFiles(true)
            }
        }
        return super.handleMessage(msg)
    }

    private fun update() {
        val edit = root.byIdx<Edit>(1)
        edit.isEnabled = !isNet
        val name = edit.text.toString()
        val enabled = files.contains(name)
        filePath = rootFolder.removePrefix(folderFiles) + name
        root.byIdx<Tile>(2).isEnabled = name.isNotBlank() && !isNet && name.indexOf('.') == -1
        root.byIdx<Tile>(3).isEnabled = if(isNet) checkedCount != 0 else enabled && name[0] != '['
        root.byIdx<Tile>(4).isEnabled = name.validFileExtensions(validExt) && !enabled && !isNet
        root.byIdx<Tile>(5).isEnabled = enabled && !isNet
    }

    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                root = cellLayout(16, 15, 1.dp) {
                    formHeader(R.string.headerIO)
                    backgroundSet(style_form)
                    editEx(R.id.edit1, R.string.hintName, style_edit_zx, style_zx_editEx) {
                        maxLength = 40
                        action.zoom = 0.75f
                        changeTextLintener = { update() }
                    }.lps(0, 0, 10, 3)
                    button(style_debugger_action) {
                        isEnabled = false
                        iconResource = R.integer.I_OPEN
                        setOnClickListener {
                            // создать папку
                            makeDirectories(filePath, FOLDER_FILES)
                            updateFiles(true)
                        }
                    }.lps(10, 0, 3, 4)
                    button(style_debugger_action) {
                        isEnabled = false
                        iconResource = R.integer.I_BOX
                        setOnClickListener {
                            if(isNet) {
                                downloadCheckedFiles()
                            } else {
                                (wnd as? ZxWnd)?.openMRU(MENU_MRU_10, filePath, false)
                                footer(BTN_OK, 0)
                            }
                        }
                    }.lps(10, 4, 3, 4)
                    button(style_debugger_action) {
                        isEnabled = false
                        iconResource = R.integer.I_SAVE
                        setOnClickListener {
                            footer(BTN_OK, 0)
                            wnd.hand?.send(RECEPIENT_FORM, ZxWnd.ZxMessages.ACT_IO_SAVE.ordinal, o = filePath)
                        }
                    }.lps(13, 4, 3, 4)
                    button(style_debugger_action) {
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
                    }.lps(10, 8, 3, 4)
                    button(style_debugger_action) {
                        iconResource = R.integer.I_CLOSE
                        setOnClickListener { footer(BTN_OK, 0) }
                    }.lps(13, 8, 3, 4)
                    ribbon(R.id.ribbonMain) {
                        requestFocus()
                        itemClickListener = {_, v, _, _ ->
                            (v as? Text)?.apply {
                                var file = text.toString()
                                var isDir = true
                                when {
                                    file == "..." -> {
                                        file = rootFolder.substringBeforeLast(File.separatorChar)
                                        rootFolder = file.substringBeforeLast(File.separatorChar) + File.separatorChar
                                        file = "[" + file.substringAfterLast(File.separatorChar) + "]"
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
                    }.lps(0, 3, 10, 11)
                    check(R.id.button1, R.string.checkNet) {
                        setOnClickListener {
                            // изменить иконку кнопки - загрузка
                            isNet = root.byIdx<Check>(8).isChecked
                            update()
                            root.byIdx<Tile>(3).iconResource = if(isNet) R.integer.I_DOWNLOAD else R.integer.I_BOX
                            if(isNet) {
                                // поменять источник
                                if (!isNetInit) {
                                    removeForm("progress")
                                    wnd.launch {
                                        FormProgress().show(wnd, R.string.cloudConnect, FORM_PROGRESS_WAIT)
                                            .doInBackground(ZxWnd.ZxMessages.ACT_DROPBOX_LIST_FILES_FINISH.ordinal) {
                                                val result = withContext(Dispatchers.IO) { dbx.folders("/ZX") }
                                                result?.filter { it.name.validFileExtensions(validNetExt) && !filesLocal.contains(it.name) }?.run {
                                                    filesCloud = sortedBy { it.name }.toMutableList()
                                                    checked = BooleanArray(filesCloud.size) { false }
                                                    BTN_OK
                                                } ?: BTN_NO
                                            }
                                        isNetInit = true
                                    }
                                }
                            }
                            updateFiles(true)
                        }
                    }.lps(13, 1, 3, 2)
                }.lps(Theme.dimen(ctx, R.dimen.ioWidth), Theme.dimen(ctx, R.dimen.ioHeight))
            }
        }
    }

    private inner class LoadingAdapter(context: Context, mObjects: List<DropBox.FileInfo>):
        ArrayListAdapter<DropBox.FileInfo>(context, ItemCloud(), ItemCloud(), mObjects) {

        override fun createView(position: Int, convertView: View?, resource: UiComponent, parent: ViewGroup, color: Boolean): View? {
            return ((convertView ?: resource.createView(UiCtx(context))) as? Check)?.apply {
                text = getItem(position)?.name
                tile = checked[position].toInt
                data = tile.toFloat()
                setOnClickListener {
                    if (isChecked) checkedCount++ else checkedCount--
                    checked[position] = isChecked
                    root.byIdx<Tile>(3).isEnabled = checkedCount != 0
                }
            }
        }
    }

    /** Класс, реализующий элемент списка файлов */
    private class ItemIO : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) { text(R.string.null_text, style_item_io) }
    }

    /** Класс, реализующий элемент списка файлов для загрузки */
    private class ItemCloud : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) { check(0, R.string.null_text, style_item_cloud) }
    }
}