package ru.ostrovskal.zx

import ru.ostrovskal.sshstd.widgets.Tile
import ru.ostrovskal.zx.ZxCommon.*

class ZxKeyboard {
    
    private class Button(@JvmField val k: CharSequence, @JvmField val l: CharSequence, @JvmField val c: CharSequence,
                        @JvmField val e: CharSequence, @JvmField val e2: CharSequence, @JvmField val e1: CharSequence,
                        @JvmField val cl: CharSequence, @JvmField val ck: CharSequence, @JvmField val g1: CharSequence,
                        @JvmField val g2: CharSequence,
                        @JvmField var tile: Tile? = null)

    private val icons = mapOf(
                    "←" to R.integer.I_LEFT, "→" to R.integer.I_RIGHT, "↑" to R.integer.I_UP, "↓" to R.integer.I_DOWN,
                    "✔" to R.integer.I_ENTER, "✖" to R.integer.I_DELETE, "▲" to R.integer.I_CAPS_ON, "△" to R.integer.I_CAPS_OFF,
                    "✿" to R.integer.I_SYMBOL_ON, "❀" to R.integer.I_SYMBOL_OFF, "FUNT" to R.integer.I_FUNT, "edt" to R.integer.I_EDIT,
                    "G0" to R.integer.I_G0, "G1" to R.integer.I_G1, "G2" to R.integer.I_G2, "G3" to R.integer.I_G3,
                    "G4" to R.integer.I_G4, "G5" to R.integer.I_G5, "G6" to R.integer.I_G6, "G7" to R.integer.I_G7,
                    "G8" to R.integer.I_G8, "G9" to R.integer.I_G9, "G10" to R.integer.I_G10, "G11" to R.integer.I_G11,
                    "G12" to R.integer.I_G12, "G13" to R.integer.I_G13, "G14" to R.integer.I_G14
    )

    private val buttons = arrayOf(
        Button("",        "", "N/A", "",          "",         "", "", "", "", ""),
        Button("1",       "1", "1", "blue",   "DEF FN",   "!",    "edt","edt", "G0", "G8"),
        Button("2",       "2", "2", "red",    "FN",       "@",    "[C]","[C]", "G1", "G9"),
        Button("3",       "3", "3", "magenta","LINE",     "#",    "tr", "tr",  "G2", "G10"),
        Button("4",       "4", "4", "green",  "OPEN",     "$",    "inv","inv", "G3", "G11"),
        Button("5",       "5", "5", "cyan",   "CLOSE",    "%",    "←",  "←",   "G4", "G12"),
        Button("6",       "6", "6", "yellow", "MOVE",     "&",    "↓",  "↓",   "G5", "G13"),
        Button("7",       "7", "7", "white",  "ERASE",    "'",    "↑",  "↑",   "G7", "G14"),
        Button("8",       "8", "8", "",       "POINT",    "(",    "→",  "→",   "G6", " "),
        Button("9",       "9", "9", "bright", "CAT",      ")",    "[G]","[G]", "[G]","[G]"),
        Button("0",       "0", "0", "black",  "FORMAT",   "_",    "✖",  "✖",  "✖",  "✖"),
        Button("✖",       "✖","✖", "✖",     "✖",        "✖",    "✖",  "✖",  "✖",  "✖"),
        Button("PLOT",    "q", "Q", "SIN",    "ASN",      "<=",   "Q",  "PLOT","Q",  "Q"),
        Button("DRAW",    "w", "W", "COS",    "ACS",      "<>",   "W",  "DRAW","INKEY$","INKEY$"),
        Button("REM",     "e", "E", "TAN",    "ATN",      ">=",   "E",  "REM", "E",  "E"),
        Button("RUN",     "r", "R", "INT",    "VERIFY",   "<",    "R",  "RUN", "R",  "R"),
        Button("RAND",    "t", "T", "RND",    "MERGE",    ">",    "T",  "RAND","T",  "T"),
        Button("RETURN",  "y", "Y", "STR$",   "[",        "AND",  "Y",  "RETURN","FN","FN"),
        Button("IF",      "u", "U", "CHR$",   "]",        "OR",   "U",  "IF",  "U",  "U"),
        Button("INPUT",   "i", "I", "CODE",   "IN",       "AT",   "I",  "INPUT","I", "I"),
        Button("POKE",    "o", "O", "PEEK",   "OUT",      ";",    "O",  "POKE", "O", "O"),
        Button("PRINT",   "p", "P", "TAB",    "@",        "\"",   "P",  "PRINT","P", "P"),
        Button("✔",       "✔", "✔","✔",     "✔",        "✔",    "✔",  "✔",   "✔", "✔"),
        Button("▲",       "▲", "△", "▲",      "▲",        "▲",    "△",  "△",   "△","▲"),
        Button("NEW",     "a", "A", "READ",   "~",        "STOP", "A",  "NEW",  "A", "A"),
        Button("SAVE",    "s", "S", "RESTORE","|",        "NOT",  "S",  "SAVE", "S", "S"),
        Button("DIM",     "d", "D", "DATE",   "\\",       "STEP", "D",  "DIM",  "D", "D"),
        Button("FOR",     "f", "F", "SGN",    "{",        "TO",   "F",  "FOR",  "F", "F"),
        Button("GOTO",    "g", "G", "ABS",    "}",        "THEN", "G",  "GOTO", "G", "G"),
        Button("GOSUB",   "h", "H", "SQR",    "CIRCLE",   "!",    "H",  "GOSUB","H", "H"),
        Button("LOAD",    "j", "J", "VAL",    "VAL$",     "-",    "J",  "LOAD", "J", "J"),
        Button("LIST",    "k", "K", "LEN",    "SCRN$",    "+",    "K",  "LIST", "K", "K"),
        Button("LET",     "l", "L", "USR",    "ATTR",     "=",    "L",  "LET",  "L", "L"),
        Button("✿",      "✿", "✿", "✿",     "❀",        "❀",   "✿",  "✿",   "✿", "✿"),
        Button("[E]",     "[E]","[E]","[L]",  "[L]",      "[E]",  "[E]","[E]",  "[E]","[E]"),
        Button("COPY",    "z", "Z", "LN",     "BEEP",     ":",    "Z",  "COPY", "POINT","POINT"),
        Button("CLEAR",   "x", "X", "EXP",    "INK",      "FUNT", "X",  "CLEAR","PI","PI"),
        Button("CONT",    "c", "C", "LPRINT", "PAPER",    "?",    "C",  "CONT", "C", "C"),
        Button("",        "",  "",  "",       "",         "",     "",   "",     "",  ""),
        Button("CLS",     "v", "V", "LLIST",  "FLUSH",    "/",    "V",  "CLS",  "RND","RND"),
        Button("BORDER",  "b", "B", "BIN",    "BRIGHT",   "*",    "B",  "BORDER","B","B"),
        Button("NEXT",    "n", "N", "INKEYS$","OVER",     ",",    "N",  "NEXT", "N", "N"),
        Button("PAUSE",   "m", "M", "PI",     "INVERSE",  ".",    "M",  "PAUSE","M", "M"),
        Button("",        "",  "↑", "",       "",         "",     "",   "",     "",  ""),
        Button("",        "",  "↓", "",       "",         "",     "",   "",     "",  ""),
        Button("",        "",  "←", "",       "",         "",     "",   "",     "",  ""),
        Button("",        "",  "→", "",       "",         "",     "",   "",     "",  ""),
        Button("",       "",  "K←", "",       "",         "",     "",   "",     "",  ""),// 47
        Button("",       "",  "K→", "",       "",         "",     "",   "",     "",  ""),
        Button("",       "",  "K↑", "",       "",         "",     "",   "",     "",  ""),
        Button("",       "",  "K↓", "",       "",         "",     "",   "",     "",  ""),
        Button("",       "",  "K*", "",       "",         "",     "",   "",     "",  "")
    )

    fun update() {
        val cursor = ZxWnd.props[ZX_PROP_KEY_CURSOR_MODE]
        repeat(42) {
            var kt: CharSequence
            val key = buttons[it + 1].apply {
                kt = when (cursor) {
                    MODE_E -> e; MODE_C -> c; MODE_L -> l; MODE_K -> k
                    MODE_E1 -> e1; MODE_E2 -> e2; MODE_CL -> cl; MODE_CK -> ck
                    MODE_G -> g1; MODE_G1 -> g2; else -> error("Неизвестный режим курсора!")
                }
            }
            key.tile?.apply {
                val icon = icons[kt]
                if(icon == null) {
                    text = kt
                    tileIcon = -1
                } else {
                    iconResource = icon
                    text = ""
                }
            }
        }
    }

    fun initButton(idx: Int, tile: Tile) {
        buttons[idx].tile = tile
    }
}