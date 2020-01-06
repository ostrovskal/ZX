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
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.sshstd.widgets.lists.Ribbon
import ru.ostrovskal.zx.R
import ru.ostrovskal.zx.ZxCommon.style_backgrnd_io
import ru.ostrovskal.zx.ZxCommon.style_item_cloud
import ru.ostrovskal.zx.ZxWnd

@Suppress("unused")
class ZxFormCloud : Form() {
    companion object {
        // Допустимые расщирения
        private val validExt     = listOf(".tap", ".z80", ".trd", ".tga", ".wav")
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

    override fun handleMessage(msg: Message): Boolean {
        if(msg.action == ZxWnd.ZxMessages.ACT_DROPBOX_LIST_FILES_FINISH.ordinal) {
            if(msg.arg1 == BTN_NO) {
                ZxFormMessage().show(wnd, intArrayOf(R.string.app_name, R.string.connectError, R.integer.I_YES, 0, 0, 0, 0))
            } else {
                root.byIdx<Ribbon>(3).adapter = LoadingAdapter(wnd, filesCloud)
            }
        } else if(msg.action == ZxWnd.ZxMessages.ACT_DROPBOX_DOWNLOAD_FINISH.ordinal)
            footer(BTN_OK, 0)
        return super.handleMessage(msg)
    }

    override fun initContent(content: ViewGroup) {
        removeForm("progress")
        wnd.launch {
            FormProgress().show(wnd, R.string.connectCloud, FORM_PROGRESS_WAIT).doInBackground(ZxWnd.ZxMessages.ACT_DROPBOX_LIST_FILES_FINISH.ordinal) {
                val result = withContext(Dispatchers.IO) {dbx.folders("/ZX") }
                result?.filter { it.name.validFileExtensions(validExt) && !filesLocal.contains(it.name) }?.run {
                    filesCloud = sortedBy { it.name }.toMutableList()
                    checked = BooleanArray(filesCloud.size) { false }
                    BTN_OK
                } ?: BTN_NO
            }
        }
    }

    private fun downloadCheckedFiles() {
        // загрузить отмеченные файлы из облака
        wnd.launch {
            FormProgress().show(wnd, R.string.downloadCloud, FORM_PROGRESS_DOWNLOAD).doInBackground(ZxWnd.ZxMessages.ACT_DROPBOX_DOWNLOAD_FINISH.ordinal) { fp ->
                delay(500L)
                fp.maximum = checkedCount
                var idx = 1
                checked.forEachIndexed { index, b ->
                    if (b) {
                        val file = filesCloud[index]
                        withContext(Dispatchers.IO) {
                            var sep = ""
                            val name = file.name
                            when(validExt.indexOf(name.substring(name.indexOf('.')))) {
                                0   -> sep = "TAP/"
                                1   -> sep = "Z80/"
                                2   -> sep = "TRD/"
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

    override fun inflateContent(container: LayoutInflater): UiCtx {
        return ui {
            linearLayout {
                root = cellLayout(15, 15, 1.dp) {
                    formHeader(R.string.headerCloud)
                    backgroundSet(style_form)
                    button {
                        isEnabled = false
                        iconResource = R.integer.I_DOWNLOAD
                        setOnClickListener { downloadCheckedFiles() }
                    }.lps(10, 4, 5, 3)
                    button {
                        iconResource = R.integer.I_CLOSE
                        setOnClickListener { thread?.interrupt(); footer(BTN_OK, 0) }
                    }.lps(10, 7, 5, 3)
                    ribbon(R.id.ribbonMain) {
                        padding = 7.dp
                        backgroundSet(style_backgrnd_io)
                    }.lps(0, 0, 10, 13)
                }.lps(Theme.dimen(ctx, R.dimen.cloudWidth), Theme.dimen(ctx, R.dimen.cloudHeight))
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
                    root.byIdx<Tile>(1).isEnabled = checkedCount != 0
                }
            }
        }
    }
    /** Класс, реализующий элемент списка файлов для загрузки */
    private class ItemCloud : UiComponent() {
        override fun createView(ui: UiCtx) = with(ui) { check(0, R.string.null_text, style_item_cloud) }
    }
}
