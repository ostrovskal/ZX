package ru.ostrovskal.zx

import android.view.ViewGroup
import android.view.ViewManager
import ru.ostrovskal.sshstd.Common
import ru.ostrovskal.sshstd.layouts.CellLayout
import ru.ostrovskal.sshstd.ui.backgroundSet
import ru.ostrovskal.sshstd.ui.cellLayout
import ru.ostrovskal.sshstd.utils.uiView
import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.zx.ZxCommon.*

class ZxKeyboard {

    private var root: ViewGroup?            = null

    private val tiles           = arrayOfNulls<Tile?>(43)

    private val names= arrayOf("N/A", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
        "PLOT", "DRAW", "REM", "RUN", "RAND", "RET", "IF", "POKE", "INPUT", "PRINT", "SIN", "COS", "TAN", "INT", "RND", "STR$", "CHR$",
        "CODE", "PEEK", "TAB", "ASN", "ACS", "ATN", "VERIFY", "MERGE", "OUT", "AND", "OR", "AT", "INKEY$", "FN",
        "NEW", "SAVE", "DIM", "FOR", "GOTO", "GOSUB", "LOAD", "LIST", "LET", "COPY", "CLEAR", "READ", "RESTORE", "DATA", "SGN", "ABS", "VAL",
        "LEN", "USR", "STOP", "NOT", "STEP", "TO", "THEN", "CIRCLE", "VAL$", "SCRN$",
        "ATTR", "LN", "BEEP", "INK", "PAPER", "FLUSH", "BRIGHT", "OVER", "INVERSE", "CONT", "CLS", "BORDER", "NEXT", "PAUSE", "POINT",
        "*", "=", "<>", "<", ">", ">=", "<=", ",", "/", "?", ".", ":", ";", "&", "%", "+", "-", "_", "'", "\"", "EXP", "LPRINT", "LLIST",
        "BIN", "", "!", "#", "$", "(", ")", "@", "[", "]", "{", "}", "DEF FN", "OPEN", "CLOSE", "FORMAT", "LINE", "ERASE", "MOVE", "CAT",
        "[C]", "[G]", "true", "invert", "SQP", "PI", "[L]", "[E]", "~", "|", "IN", "TILE"
    )

    fun update() {
        tiles.forEachIndexed { index, tile ->
            tile?.apply {
                val pos = ZX_PROP_VALUES_BUTTON + index * 2
                val icon = ZxWnd.props[pos + 1].toInt()
                if (icon == 0) {
                    var text = ZxWnd.props[pos].toInt()
                    if(text < 0) text += 256
                    setText(names[text])
                    tileIcon = -1
                } else {
                    tileIcon = icon
                    text = ""
                }
            }
        }
    }

    /** Реализация кнопки клавиатуры */
    private fun ViewManager.keyboardButton() = uiView({ Tile(it, style_key_button).apply {
        setOnTouchListener { v, event ->
            root?.apply { ZxWnd.zxCmd(ZX_CMD_UPDATE_KEY, indexOfChild(v) + 1, event.action, "") }
            false
        } } }, {} )

    fun layout(ui: CellLayout) = with(ui) {
        cellLayout(11, 4) {
            root = this
            backgroundSet(Common.style_form)
            repeat(4) { y ->
                repeat(11) rep@{ x ->
                    if(y == 3 && x > 8) return@rep
                    var xx = x
                    if(y == 3 && x > 4) xx += 2
                    tiles[y * 11 + x] = keyboardButton().lps(xx, y, if(y == 3 && x == 4) 3 else 1, 1)
                }
            }
        }
    }
}
