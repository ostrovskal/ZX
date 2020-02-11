package ru.ostrovskal.zx.forms

import android.view.LayoutInflater
import ru.ostrovskal.sshstd.Common
import ru.ostrovskal.sshstd.adapters.ArrayListAdapter
import ru.ostrovskal.sshstd.forms.Form
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
class ZxFormDisk : Form() {

    private var numDisk                    = -1

    private var isEmpty                    = false

    // корневая папка
    private var rootFolder          = Common.folderFiles

    // относительный путь к выбранному файлу
    private var filePath                    = ""

    // список расщирений
    private val validExt        = listOf(".trd", ".fdi", ".scl")

    // имена дисков
    private val diskInsert      = listOf("A: ", "B: ", "C: ", "D: ")

    // список файлов
    private var files           = listOf("")

    private fun updateFiles(updateRibbon: Boolean) {
        files = rootFolder.listFoldersAndFiles(rootFolder != Common.folderFiles, validExt)
        if(updateRibbon) root.byIdx<Ribbon>(11).adapter = ArrayListAdapter(wnd, ItemIO(), ItemIO(), files)
    }

    private fun update() {
        root.byIdx<Tile>(8).isEnabled = !isEmpty
        val name = root.byIdx<Edit>(4).text.toString()
        val enabled = files.contains(name)
        filePath = rootFolder.removePrefix(Common.folderFiles) + name
        root.byIdx<Tile>(5).isEnabled = enabled
        root.byIdx<Tile>(6).isEnabled = name.validFileExtensions(validExt) && !enabled && !isEmpty
        root.byIdx<Tile>(7).isEnabled = enabled
    }

    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                root = cellLayout(16, 18, 1.dp) {
                    formHeader(R.string.headDisk)
                    backgroundSet(Common.style_form)
                    spinner(R.id.spinner1) {
                        adapter = ArrayListAdapter(context, ZxFormSettings.Popup(), ZxFormSettings.Item(), diskInsert)
                        itemClickListener = { _, _, p, _ ->
                            // в текст поставить выбранный диск
                            var path = "disk$p".s
                            isEmpty = path.isBlank()
                            numDisk = p
                            var check = false
                            if(isEmpty) {
                                path = context.getString(R.string.diskEmpty)
                            } else {
                                check = ZxWnd.zxCmd(ZX_CMD_DISK_OPS, numDisk, ZX_DISK_OPS_GET_READONLY, "") != 0
                            }
                            // установить read only
                            root.byIdx<Check>(3).isChecked = check
                            root.byIdx<Text>(2).text = path
                            update()
                        }
                    }.lps(0, 0, 3, 2)
                    text(R.string.app_name).lps(3, 0, 10, 2)
                    check(R.id.button1, R.string.checkReadOnly) {
                        setOnClickListener {
                            val check = root.byIdx<Check>(3).isChecked
                            ZxWnd.zxCmd(ZX_CMD_DISK_OPS, numDisk or (check.toInt shl 7), ZX_DISK_OPS_SET_READONLY, "")
                        }
                    }.lps(10, 14, 5, 2)
                    editEx(R.id.edit1, R.string.hintName, style_edit_zx) {
                        maxLength = 50
                        changeTextLintener = { update() }
                    }.lps(0, 2, 10, 3)
                    button(style_debugger_action) {
                        // вставить
                        isEnabled = false
                        iconResource = R.integer.I_INSERT
                        setOnClickListener {
                            if(ZxWnd.zxCmd(ZX_CMD_DISK_OPS, numDisk, ZX_DISK_OPS_OPEN, filePath) != 0) {
                                val check = root.byIdx<Check>(3).isChecked
                                ZxWnd.zxCmd(ZX_CMD_DISK_OPS, numDisk, ZX_DISK_OPS_SET_READONLY or (check.toInt shl 7), "")
                                "disk$numDisk".s = filePath
                                root.byIdx<Text>(2).text = filePath
                                isEmpty = false
                                update()
                            }
                        }
                    }.lps(10, 2, 3, 4)
                    button(style_debugger_action) {
                        // записать
                        isEnabled = false
                        iconResource = R.integer.I_SAVE
                        setOnClickListener {
                            ZxWnd.zxCmd(ZX_CMD_DISK_OPS, numDisk, ZX_DISK_OPS_SAVE, filePath)
                        }
                    }.lps(13, 2, 3, 4)
                    button(style_debugger_action) {
                        // удалить
                        isEnabled = false
                        iconResource = R.integer.I_DELETE
                        setOnClickListener {
                            // проверить на удаление папки
                            var fp = filePath
                            if(filePath.last() == ']') {
                                fp = filePath.replace("[", "").replace("]", "")
                            }
                            File(Common.folderFiles + fp).delete()
                            root.byIdx<Edit>(4).setText("")
                            // проверить - если этот файл был подключен - сделать eject
                            repeat(4) { num ->
                                val dsk = "disk$num"
                                if(dsk.s == fp) {
                                    ZxWnd.zxCmd(ZX_CMD_DISK_OPS, num, ZX_DISK_OPS_EJECT, fp)
                                    dsk.s = ""
                                    if(num == numDisk) {
                                        // убрать текст
                                        isEmpty = true
                                        root.byIdx<Text>(2).text = context.getString(R.string.diskEmpty)
                                    }
                                }
                            }
                            updateFiles(true)
                            update()
                        }
                    }.lps(10, 6, 3, 4)
                    button(style_debugger_action) {
                        // извлечь
                        isEnabled = false
                        iconResource = R.integer.I_EJECT
                        setOnClickListener {
                            ZxWnd.zxCmd(ZX_CMD_DISK_OPS, numDisk, ZX_DISK_OPS_EJECT, "")
                            "disk$numDisk".s = ""
                            root.byIdx<Text>(2).text = context.getString(R.string.diskEmpty)
                            isEmpty = true
                            update()
                        }
                    }.lps(13, 6, 3, 4)
                    button(style_debugger_action) {
                        // TRDOS
                        iconResource = R.integer.I_TRDOS
                        setOnClickListener {
                            ZxWnd.zxCmd(ZX_CMD_DISK_OPS, 0, ZX_DISK_OPS_TRDOS, "")
                            footer(Common.BTN_OK, 0)
                        }
                    }.lps(10, 10, 3, 4)
                    button(style_debugger_action) {
                        // закрыть
                        iconResource = R.integer.I_CLOSE
                        setOnClickListener { footer(Common.BTN_OK, 0) }
                    }.lps(13, 10, 3, 4)
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
                                root.byIdx<Edit>(4).setText(file)
                            }
                        }
                        padding = 7.dp
                        backgroundSet(style_backgrnd_io)
                        updateFiles(false)
                        adapter = ArrayListAdapter(wnd, ItemIO(), ItemIO(), files)
                    }.lps(0, 5, 10, 12)
                }.lps(Theme.dimen(ctx, R.dimen.diskWidth), Theme.dimen(ctx, R.dimen.diskHeight))
            }
        }
    }

    /** Класс, реализующий элемент списка файлов */
    private class ItemIO : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) { text(R.string.null_text, style_item_io) }
    }
}