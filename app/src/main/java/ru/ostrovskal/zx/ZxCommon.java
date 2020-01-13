package ru.ostrovskal.zx;

import android.graphics.Color;
import android.text.InputType;
import android.view.Gravity;
import android.view.View;

import static android.view.View.TEXT_ALIGNMENT_GRAVITY;
import static android.view.inputmethod.EditorInfo.IME_FLAG_NO_EXTRACT_UI;
import static ru.ostrovskal.sshstd.Common.*;

public final class ZxCommon {

    private ZxCommon() { }

    public static final String ZX_AUTO_SAVE         = "auto_save.zx";
    public static final String ZX_TMP_SAVE          = "tmp_save.zx";
    public final static byte ZX_DEBUGGER            = 0x20;

    public final static int DA_PC                   = 1;
    public final static int DA_CODE                 = 2;
    public final static int DA_PN                   = 4;
    public final static int DA_PNN                  = 8;
    public final static int DA_LABEL                = 16;

    public final static byte ZX_BP_NONE             = 0; // не учитывается
    public final static byte ZX_BP_EXEC             = 1; // исполнение

    // DEBUGGER ACTION
    public final static int DEBUGGER_ACT_HEX_DEC    = 0;
    public final static int DEBUGGER_ACT_PREV       = 1;
    public final static int DEBUGGER_ACT_ACTION     = 2;
    public final static int DEBUGGER_ACT_NEXT       = 3;
    public final static int DEBUGGER_ACT_TRACE_IN   = 4;
    public final static int DEBUGGER_ACT_TRACE_OUT  = 5;
    public final static int DEBUGGER_ACT_TRACE_OVER = 6;
    public final static int DEBUGGER_ACT_BP         = 7;
    public final static int DEBUGGER_ACT_BP_LIST    = 8;
    public final static int DEBUGGER_ACT_SET_ASM    = 9;

    // UPDATE DEBUGGER
    public final static int ZX_PC                   = 1;
    public final static int ZX_REG                  = 2;
    public final static int ZX_DT                   = 4;
    public final static int ZX_SEL                  = 8;
    public final static int ZX_LIST                 = 16;
    public final static int ZX_STORY                = 32;
    public final static int ZX_TRACE                = 64;
    public final static int ZX_ALL                  = ZX_PC | ZX_REG | ZX_SEL | ZX_LIST | ZX_STORY | ZX_DT;
    public final static int ZX_SL                   = ZX_SEL | ZX_LIST;
    public final static int ZX_PSL                  = ZX_PC | ZX_SEL | ZX_LIST;
    public final static int ZX_DSL                  = ZX_DT | ZX_SEL | ZX_LIST;
    public final static int ZX_RSL                  = ZX_REG | ZX_SEL | ZX_LIST;
    public final static int ZX_RL                   = ZX_REG | ZX_LIST;
    public final static int ZX_PYSL                 = ZX_PC | ZX_STORY | ZX_SEL | ZX_LIST;

    // REGISTERS
    public final static int ZX_CPU_BC               = 0;
    public final static int ZX_CPU_DE               = 2;
    public final static int ZX_CPU_HL               = 4;
    public final static int ZX_CPU_AF               = 6;
    public final static int ZX_CPU_F                = 6;
    public final static int ZX_CPU_IX               = 16;
    public final static int ZX_CPU_IY               = 18;
    public final static int ZX_CPU_SP               = 20;
    public final static int ZX_CPU_PC               = 22;
    public final static int ZX_CPU_RAM              = 34;
    public final static int ZX_CPU_ROM              = 35;
    public final static int ZX_CPU_VID              = 36;
    public final static int ZX_CPU_STATE            = 65;

    public final static int MENU_CLOUD              = 1000;
    public final static int MENU_IO                 = 1001;
    public final static int MENU_SETTINGS           = 1002;
    public final static int MENU_PROPS              = 1003;
    public final static int MENU_DISKS              = 1004;
    public final static int MENU_MODEL              = 1005;
    public final static int MENU_RESET              = 1006;
    public final static int MENU_RESTORE            = 1007;
    public final static int MENU_EXIT               = 1008;

    public final static int MENU_PROPS_KEYBOARD     = 1009;
    public final static int MENU_PROPS_JOYSTICK     = 1010;
    public final static int MENU_PROPS_SOUND        = 1011;
    public final static int MENU_PROPS_TAPE         = 1012;
    public final static int MENU_PROPS_FILTER       = 1013;
    public final static int MENU_PROPS_TURBO        = 1014;
    public final static int MENU_PROPS_EXECUTE      = 1015;
    public final static int MENU_PROPS_DEBUGGER     = 1016;
    public final static int MENU_DEBUGGER_LABEL     = 1017;
    public final static int MENU_DEBUGGER_CODE      = 1018;
    public final static int MENU_DEBUGGER_VALUE     = 1019;
    public final static int MENU_MRU                = 1020;
    public final static int MENU_POKES              = 1021;
    public final static int MENU_DEBUGGER1          = 1022;

    public final static int MENU_DISK_A             = 1200;
    public final static int MENU_DISK_B             = 1201;
    public final static int MENU_DISK_C             = 1202;
    public final static int MENU_DISK_D             = 1203;

    public final static int MENU_MODEL_48KK         = 1300;
    public final static int MENU_MODEL_48KS         = 1301;
    public final static int MENU_MODEL_48KSN        = 1302;
    public final static int MENU_MODEL_128K         = 1303;
    public final static int MENU_MODEL_PENTAGON     = 1304;
    public final static int MENU_MODEL_SCORPION     = 1305;
    public final static int MENU_MODEL_PROFI        = 1306;

    public final static int MENU_MRU_1              = 1400;
    public final static int MENU_MRU_2              = 1401;
    public final static int MENU_MRU_3              = 1402;
    public final static int MENU_MRU_4              = 1403;
    public final static int MENU_MRU_5              = 1404;
    public final static int MENU_MRU_6              = 1405;
    public final static int MENU_MRU_7              = 1406;
    public final static int MENU_MRU_8              = 1407;
    public final static int MENU_MRU_9              = 1408;
    public final static int MENU_MRU_10             = 1409;
    
    // Формы эмулятора
    public final static int FORM_MAIN               = 0;
    public final static int FORM_OPTIONS            = 1;
    public final static int FORM_IO                 = 2;
    public final static int FORM_POKES              = 3;
    public final static int FORM_LOADING            = 4;
    public final static int FORM_BREAK_POINTS       = 5;

    // Разделяемые свойства

    // 0. Байтовые значения, вычисляемые во время работы программы
    public static final int ZX_PROP_JOY_TYPE        = 70; // Текущий тип джойстика
    public static final int ZX_PROP_JOY_KEYS        = 71; // Привазанные к джойстику коды кнопок клавиатуры (8) 69 - 76
    public static final int ZX_PROP_JOY_CROSS_VALUE = 79; // Нажатые кнопки джойстика-крестовины
    public static final int ZX_PROP_JOY_ACTION_VALUE= 80; // Нажатые кнопки джойстика-управления
    public static final int ZX_PROP_JNI_RETURN_VALUE= 92; // Значение передаваемое из JNI
    public static final int ZX_PROP_VALUES_BUTTON   = 322; // Значение для обновления кнопок клавиатуры(текст, иконка) (42 * 2) 322 - 405

    // 1. Булевы значения
    public static final int ZX_PROP_FIRST_LAUNCH    = 128; // Признак первого запуска
    private static final int ZX_PROP_TRAP_TAPE      = 129; // Признак перехвата загрузки/записи с ленты
    public static final int ZX_PROP_SHOW_JOY        = 130; // Признак отображения джойстика
    public static final int ZX_PROP_SHOW_KEY        = 131; // Признак отображения клавиатуры
    private static final int ZX_PROP_TURBO_MODE     = 132; // Признак турбо-режима процессора
    private static final int ZX_PROP_SND_LAUNCH     = 133; // Признак запуска звукового процессора
    private static final int ZX_PROP_SND_BP         = 134; // Признак запуска бипера
    private static final int ZX_PROP_SND_AY         = 135; // Признак трехканального AY
    private static final int ZX_PROP_SND_8BIT       = 136; // Признак 8 битного звука
    private static final int ZX_PROP_SND_SAVE       = 137; // Признак прямой записи
    public static final int ZX_PROP_EXECUTE         = 138; // Признак выполнения программы
    public static final int ZX_PROP_SHOW_HEX        = 139; // Признак 16-тиричного вывода
    public static final int ZX_PROP_SHOW_DEBUGGER   = 140; // Признак режима отладчика
    public static final int ZX_PROP_SHOW_LABEL      = 141; // Признак отображения меток инструции
    public static final int ZX_PROP_SHOW_CODE       = 142; // Признак режима кода инструкции
    public static final int ZX_PROP_SHOW_CODE_VALUE = 143; // Признак режима содержимого кода

    // 2. Байтовые значения
    public static final int ZX_PROP_ACTIVE_DISK     = 150; // Номер активного диска
    private static final int ZX_PROP_BORDER_SIZE    = 151; // Размер границы
    public static final int ZX_PROP_MODEL_TYPE      = 152; // Модель памяти
    private static final int ZX_PROP_SND_TYPE_AY    = 153; // Тип каналов в звуковом процессоре AY
    private static final int ZX_PROP_SND_FREQUENCY  = 154; // Частота звука
    private static final int ZX_PROP_SND_VOLUME_BP  = 155; // Громкость бипера
    private static final int ZX_PROP_SND_VOLUME_AY  = 156; // Громкость AY
    private static final int ZX_PROP_BLINK_SPEED    = 157; // Скорость мерцания курсора
    private static final int ZX_PROP_CPU_SPEED      = 158; // Скорость процессора
    public static final int ZX_PROP_KEY_SIZE        = 159; // Размер экранной клавиатуры
    public static final int ZX_PROP_JOY_SIZE        = 160; // Размер экранного джойстика

    // 3. Целые значения
    public static final int ZX_PROP_COLORS          = 170; // значения цветов (16 * 4) 170 - 233

    // 4. Значение структур
    private static final int ZX_PROP_BPS            = 192; // значения точек останова (8 * 8) 258 - 321

    public static final int ZX_PROPS_COUNT          = 410; // Размер буфера свойств
    public static final int ZX_PROPS_INIT_COUNT     = (ZX_PROP_BPS - ZX_PROP_FIRST_LAUNCH) + 8; // Количество свойств

    // Варианты форматирования чисел
    public static final int ZX_FV_OPS16				= 12;// "5X", "4#X"
    public static final int ZX_FV_OPS8				= 14;// "3X", "2#X"
    public static final int ZX_FV_SIMPLE			= 24;// "0X", "0X"

    // Команды
    public static final int ZX_CMD_MODEL            = 0;  // Установка модели памяти
    public static final int ZX_CMD_PROPS            = 1;  // Установка свойств
    public static final int ZX_CMD_RESET            = 2;  // Сброс
    public static final int ZX_CMD_UPDATE_KEY       = 3;  // Обновление кнопок
    public static final int ZX_CMD_INIT_GL          = 4;  // Инициализация GL
    public static final int ZX_CMD_POKE             = 5;  // Установка POKE
    public static final int ZX_CMD_ASSEMBLER        = 6;  // Ассемблирование
    public static final int ZX_CMD_TAPE_COUNT       = 7;  // Получение количества блоков ленты
    public static final int ZX_CMD_QUICK_BP         = 8;  // Быстрая установка точки останова
    public static final int ZX_CMD_TRACE_X          = 9;  // Трассировка
    public static final int ZX_CMD_STEP_DEBUG       = 10; // Выполнение в отладчике
    public static final int ZX_CMD_MOVE             = 11; // Выполнение сдвига ПС
    public static final int ZX_CMD_JUMP             = 12; // Получение адреса в памяти/адреса перехода в инструкции

    public static final int ZX_DEBUGGER_MODE_PC     = 0; // Список ДА
    public static final int ZX_DEBUGGER_MODE_SP     = 1; // Список СП
    public static final int ZX_DEBUGGER_MODE_DT     = 2; // Список данных

    private static final int ATTR_SSH_COLOR_DEBUGGER_SELECTOR     = 1000; // Аттрибут для цвета выделения в отладчике

    // Тема по умолчанию
    public static final int[] themeDef = {
            ATTR_SSH_THEME_NAME, R.string.themeDef,
            ATTR_SSH_BM_ICONS, R.drawable.zx_icons,
            ATTR_SSH_SEEK_ANIM, SEEK_ANIM_ROTATE,
            ATTR_SSH_BM_BUTTONS, R.drawable.button,
            ATTR_SSH_BM_SEEK, R.drawable.seek,
            ATTR_SSH_BM_TOOLS, R.drawable.tool,
            ATTR_SSH_BM_HEADER, R.drawable.header,
            ATTR_SSH_BM_EDIT, R.drawable.edit,
            ATTR_SSH_BM_CHECK, R.drawable.check,
            ATTR_SSH_BM_RADIO, R.drawable.radio,
            ATTR_SSH_BM_SWITCH, R.drawable.switched,
            ATTR_SSH_BM_SPINNER, R.drawable.spinner,
            ATTR_SSH_BM_BACKGROUND, R.drawable.background,
            ATTR_SSH_COLOR_DIVIDER, 0x7a7a7a | COLOR,
            ATTR_SSH_COLOR_LAYOUT, 0x2d2929 | COLOR,
            ATTR_SSH_COLOR_NORMAL, 0x9599f7 | COLOR,
            ATTR_SSH_COLOR_LARGE, 0xbc5a1d | COLOR,
            ATTR_SSH_COLOR_DEBUGGER_SELECTOR, 0x60bc5a1d,
            ATTR_SSH_COLOR_SMALL, 0x2ea362 | COLOR,
            ATTR_SSH_COLOR_HINT, 0xf77499 | COLOR,
            ATTR_SSH_COLOR_SELECTOR, Color.MAGENTA | COLOR,
            ATTR_SSH_COLOR_HEADER, 0xcfba41 | COLOR,
            ATTR_SSH_COLOR_HTML_HEADER, 0xf22782 | COLOR,
            ATTR_SSH_COLOR_LINK, 0xb8fa01 | COLOR,
            ATTR_SSH_COLOR_MESSAGE, 0xd2fa64 | COLOR,
            ATTR_SSH_COLOR_WINDOW, 0x030303 | COLOR,
            ATTR_SSH_COLOR_WIRED, 0xffffff | COLOR,
            ATTR_SSH_ICON_VERT, 10,
            ATTR_SSH_ICON_HORZ, 10,
    };

    /** Стиль кнопки клавиатуры */
    public static final int[] style_key_button = {
            ATTR_FONT, R.string.font_small,
            ATTR_COLOR_DEFAULT, 0xf77499 | COLOR,
            ATTR_CLICKABLE, 1,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_SSH_HORZ, 2,
            ATTR_SSH_TILE, 1,
            ATTR_MAX_LINES, 1,
            ATTR_SSH_GRAVITY, TILE_GRAVITY_CENTER | TILE_GRAVITY_BACKGROUND,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_BUTTONS | THEME,
            ATTR_SSH_STATES, TILE_STATE_PRESS
    };

    public static final String jCross          =    "     2222222222     \n" +
                                                    "  0202222222222121  \n" +
                                                    " 002020222222121211 \n" +
                                                    "00000202022121211111\n" +
                                                    "00000002022121111111\n" +
                                                    "00000003031313111111\n" +
                                                    "00000303033131311111\n" +
                                                    "00030303333331313111\n" +
                                                    " 003033333333331311 \n" +
                                                    "     3333333333     ";

    public static final String jAction          =   "       000000       \n" +
                                                    "      00000000      \n" +
                                                    "      00000000      \n" +
                                                    " 111111000000222222 \n" +
                                                    "11111111    22222222\n" +
                                                    "11111111    22222222\n" +
                                                    " 111111333333222222 \n" +
                                                    "      33333333      \n" +
                                                    "      33333333      \n" +
                                                    "       333333       ";

    public static final int[] style_zxCross = {
            ATTR_VISIBILITY, View.GONE,
            ATTR_SSH_CONTROLLER_WIDTH, 120,
            ATTR_SSH_CONTROLLER_HEIGHT, 120,
            ATTR_SSH_ALPHA, 174,
            ATTR_SSH_BITMAP_NAME, R.drawable.controller_zx_cross,
            ATTR_SSH_GRAVITY, TILE_GRAVITY_BACKGROUND,
            ATTR_SSH_HORZ, 5,
            ATTR_SSH_VERT, 1,
            ATTR_SSH_TILE, 0
    };

    public static final int[] style_zxAction= {
            ATTR_VISIBILITY, View.GONE,
            ATTR_SSH_CONTROLLER_WIDTH, 120,
            ATTR_SSH_CONTROLLER_HEIGHT, 120,
            ATTR_SSH_ALPHA, 174,
            ATTR_SSH_BITMAP_NAME, R.drawable.controller_zx_action,
            ATTR_SSH_GRAVITY, TILE_GRAVITY_BACKGROUND,
            ATTR_SSH_HORZ, 5,
            ATTR_SSH_VERT, 1,
            ATTR_SSH_TILE, 0
    };

    public static final int[] style_edit_zx = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_GRAVITY, Gravity.CENTER_VERTICAL,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_COLOR_HIGHLIGHT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_SIZE, R.dimen.normal,
            ATTR_FONT, R.string.font_normal,
            ATTR_FOCUSABLE_TOUCH_MODE, 1,
            ATTR_COLOR_HINT, ATTR_SSH_COLOR_HINT | THEME,
            ATTR_FOCUSABLE, 1,
            ATTR_CLICKABLE, 1,
            ATTR_SSH_TILE, 0,
            ATTR_PADDING_HORZ, R.dimen.paddingHorzEdit,
            ATTR_PADDING_VERT, R.dimen.paddingVertEdit,
            ATTR_INPUT_TYPE, InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_CLASS_TEXT,
            ATTR_IME_OPTIONS, IME_FLAG_NO_EXTRACT_UI,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_EDIT | THEME,
            ATTR_SSH_PATCH9, R.string.patch9_edit
    };

    public static final int[] style_backgrnd_io = {
            ATTR_SSH_SHAPE, TILE_SHAPE_ROUND,
            ATTR_SSH_COLOR_SELECTOR, Color.DKGRAY | COLOR,
            ATTR_SSH_WIDTH_SELECTOR, 2,
            ATTR_SSH_SOLID, 0x80000000 | COLOR
    };

    public static final int[] style_item_io = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_COLOR_HIGHLIGHT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_GRAVITY, Gravity.START,
            ATTR_MAX_LINES, 1,
            ATTR_SIZE, R.dimen.normal,
            ATTR_FONT, R.string.font_normal
    };

    public static final int[] style_item_cloud = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_COLOR_HIGHLIGHT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_GRAVITY, Gravity.START | Gravity.CENTER_VERTICAL,
            ATTR_SIZE, R.dimen.normal,
            ATTR_FONT, R.string.font_normal,
            ATTR_TEXT_ALIGN, TEXT_ALIGNMENT_GRAVITY,
            ATTR_MIN_HEIGHT, R.dimen.heightCheck,
            ATTR_SSH_HORZ, 2,
            ATTR_SSH_TILE, 0,
            ATTR_MAX_LINES, 1,
            ATTR_SSH_SCALE, TILE_SCALE_MIN,
            ATTR_SSH_GRAVITY, TILE_GRAVITY_LEFT | TILE_GRAVITY_CENTER_VERT,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_CHECK | THEME
    };

    public static final int[] style_tab_settings = {
            ATTR_SSH_SCALE_ICON, 49152,
            ATTR_SSH_VERT, 9,
            ATTR_SSH_HORZ, 8,
            ATTR_CLICKABLE, 1,
            ATTR_FOCUSABLE, 1,
            ATTR_SSH_SCALE, TILE_SCALE_NONE,
            ATTR_SSH_WIDTH_SELECTOR, 2,
            ATTR_SSH_COLOR_SELECTOR, ATTR_SSH_COLOR_HEADER | THEME,
            ATTR_SSH_GRAVITY, TILE_GRAVITY_BACKGROUND,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_HEADER | THEME,
            ATTR_FONT, R.string.font_large,
            ATTR_SIZE, R.dimen.normal,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_BUTTONS | THEME
    };

    public static final int[] style_text_settings = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_SIZE, R.dimen.textSettings,
            ATTR_GRAVITY, Gravity.CENTER_VERTICAL,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_FONT, R.string.font_normal
    };

    public static final int[] style_text_tape = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_SIZE, R.dimen.normal,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_FONT, R.string.font_large
    };

    public static final int[] style_color_text_settings = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_FONT, R.string.font_small,
            ATTR_SIZE, R.dimen.settingsTextColor,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_COLOR_DEFAULT, 0xffffff | COLOR
    };

    public static final int[] style_spinner_title_settings = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_COLOR_HIGHLIGHT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_PADDING_HORZ, R.dimen.paddingHorzSelectItem,
            ATTR_MAX_LINES, 1,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_FONT, R.string.font_small,
            ATTR_SIZE, R.dimen.large,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_SPINNER | THEME,
            ATTR_SSH_VERT, 3,
            ATTR_SSH_TILE, 0
    };

    public static final int[] style_spinner_item_settings = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_COLOR_HIGHLIGHT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_MAX_LINES, 1,
            ATTR_FONT, R.string.font_small,
            ATTR_SIZE, R.dimen.normal,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_SPINNER | THEME,
            ATTR_MIN_HEIGHT, R.dimen.heightSpinnerItem,
            ATTR_PADDING_HORZ, R.dimen.paddingHorzSelectItem,
            ATTR_SSH_VERT, 3,
            ATTR_SSH_PATCH9, R.string.patch9_select_item,
            ATTR_SSH_TILE, 2
    };

    public static final int[] style_zx_toolbar = {
            ATTR_SSH_VERT, 10,
            ATTR_SSH_HORZ, 10,
            ATTR_SSH_TILE, 0,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_ICONS | THEME
    };

    public static final int[] style_debugger_flags = {
            ATTR_SIZE, R.dimen.debuggerTextDef,
            ATTR_FONT, R.string.font_small,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_MAX_LINES, 1,
    };

    public static final int[] style_debugger_selector = {
            ATTR_SSH_SHAPE, TILE_SHAPE_RECT,
            ATTR_SSH_COLOR_SELECTOR, ATTR_SSH_COLOR_DEBUGGER_SELECTOR | THEME,
            ATTR_SSH_WIDTH_SELECTOR, 2
    };
    public static final int[] style_debugger_text = {
            ATTR_SIZE, R.dimen.debuggerTextDef,
            ATTR_FONT, R.string.font_small,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_MAX_LINES, 1
    };

    public static final int[] style_debugger_ribbon = {
            ATTR_SELECTOR, ATTR_SSH_COLOR_DEBUGGER_SELECTOR | THEME,
            ATTR_LONG_CLICKABLE, 1,
            ATTR_PADDING, 2
    };

    public static final int[] style_debugger_item = {
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_GRAVITY, Gravity.START,
            ATTR_MAX_LINES, 1,
            ATTR_SIZE, R.dimen.debuggerTextList
    };

    public static final int[] style_debugger_action = {
            ATTR_CLICKABLE, 1,
            ATTR_FOCUSABLE, 1,
            ATTR_MIN_HEIGHT, R.dimen.heightButton,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_SSH_HORZ, 3,
            ATTR_SSH_TILE, 0,
            ATTR_SSH_GRAVITY, TILE_GRAVITY_CENTER | TILE_GRAVITY_BACKGROUND,
            ATTR_SSH_BITMAP_NAME, R.drawable.tool,
            ATTR_SSH_STATES, TILE_STATE_PRESS
    };

    public static final int[] style_debugger_edit = {
            ATTR_SHADOW_TEXT, R.string.shadow_null,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_COLOR_HIGHLIGHT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_SIZE, R.dimen.debuggerTextDef,
            ATTR_FOCUSABLE_TOUCH_MODE, 1,
            ATTR_COLOR_HINT, ATTR_SSH_COLOR_HINT | THEME,
            //ATTR_FOCUSABLE, 1,
            ATTR_CLICKABLE, 1,
            ATTR_SSH_TILE, 0,
            ATTR_PADDING_HORZ, R.dimen.paddingHorzDebEdit,
            ATTR_PADDING_VERT, 0,//R.dimen.paddingVertEdit,
            ATTR_IME_OPTIONS, IME_FLAG_NO_EXTRACT_UI,
            ATTR_INPUT_TYPE, InputType.TYPE_NULL,
            ATTR_SSH_SHAPE, TILE_SHAPE_RECT,
            ATTR_SSH_WIDTH_SELECTOR, 1,
            ATTR_SSH_COLOR_SELECTOR, Color.DKGRAY | COLOR,
            ATTR_SSH_SOLID, 0x00007f | COLOR
    };

    /** Стиль для формы сообщений */
    public static final int[] style_form_message         = {
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_BACKGROUND | THEME,
            ATTR_SSH_RADII, R.string.radiiFormMessage,
            ATTR_SSH_SHAPE, TILE_SHAPE_ROUND,
            ATTR_SSH_TILE, 0
    };

    public static final String[] joyButtons     = {"K←", "K→", "K↑", "K↓", "K*", "6", "7", "9", "8", "0", "1", "2", "4", "3", "5", "←", "→", "↑",
                                                    "↓", "0", "N/A", "N/A", "N/A", "N/A", "N/A"};

    public static final int[] settingsCommon    = { ZX_PROP_CPU_SPEED, ZX_PROP_BLINK_SPEED, ZX_PROP_JOY_SIZE, ZX_PROP_BORDER_SIZE, ZX_PROP_KEY_SIZE};
    public static final int[] settingsCheckSnd  = { ZX_PROP_SND_BP, ZX_PROP_SND_AY, ZX_PROP_SND_8BIT, ZX_PROP_SND_SAVE };
    public static final int[] settingsSnd       = { ZX_PROP_SND_TYPE_AY, ZX_PROP_SND_FREQUENCY, ZX_PROP_SND_VOLUME_BP, ZX_PROP_SND_VOLUME_AY };
    public static final int[] settingsAllSnd    = { 0, ZX_PROP_SND_TYPE_AY, 0, ZX_PROP_SND_FREQUENCY, 0, ZX_PROP_SND_VOLUME_BP, 0, ZX_PROP_SND_VOLUME_AY,
                                                    ZX_PROP_SND_BP, ZX_PROP_SND_AY, ZX_PROP_SND_8BIT, ZX_PROP_SND_SAVE};

    public static final int[] debuggerLand = {
            30, 14,
            20, 0, 2, 3,   22, 0, 2, 3,   24, 0, 2, 3,  26, 0, 2, 3,  28, 0, 2, 3,
            21, 3, 2, 3,   23, 3, 2, 3,   25, 3, 2, 3,  27, 3, 2, 3,  28, 12, 2, 3,
            0, 0, 20, 15,
            21, 6, 1, 1,   22, 6, 1, 1,   23, 6, 1, 1,  24, 6, 1, 1,  25, 6, 1, 1,  26, 6, 1, 1, 27, 6, 1, 1, 28, 6, 1, 1,

            20, 7, 2, 1,   22, 7, 2, 1,   24, 7, 1, 1,  25, 7, 2, 1,  27, 7, 1, 1,  28, 7, 2, 1,

            20,  8, 2, 1,  22,  8, 3, 1,  25,  8, 2, 1, 27,  8, 3, 1,
            20, 9, 2, 1,   22,  9, 3, 1,  25,  9, 2, 1, 27,  9, 3, 1,
            20, 10, 2, 1,  22, 10, 3, 1,  25, 10, 2, 1, 27, 10, 3, 1,
            20, 11, 2, 1,  22, 11, 3, 1,  25, 11, 2, 1, 27, 11, 3, 1,
            20, 12, 8, 3
    };

    public static final int[] debuggerPort = {
            32, 18,

            1, 0, 4, 2,    6, 0, 4, 2,   10, 0, 4, 2,   14, 0, 4, 2,  19, 0, 4, 2, 23, 0, 4, 2, 27, 0, 4, 2,
            3, 2, 4, 2,    9, 2, 4, 2,
            29, 6, 3, 2,
            0, 8, 34, 10,

            16, 2, 2, 1,   18, 2, 2, 1,   20, 2, 2, 1,  22, 2, 2, 1,  24, 2, 2, 1,  26, 2, 2, 1,  28, 2, 2, 1,  30, 2, 2, 1,

            17, 3, 2, 1,   19, 3, 3, 1,   22, 3, 2, 1,  24, 3, 3, 1,  27, 3, 2, 1,  29, 3, 3, 1,

            0, 4, 2, 1,    2, 4, 6, 1,    8, 4, 2, 1,   10, 4, 6, 1,
            16, 4, 2, 1,   18, 4, 6, 1,   24, 4, 2, 1,  26, 4, 6, 1,
            0, 5, 2, 1,    2, 5, 6, 1,    8, 5, 2, 1,   10, 5, 6, 1,
            16, 5, 2, 1,   18, 5, 6, 1,   24, 5, 2, 1,  26, 5, 6, 1,
            0, 6, 29, 2
    };

    public static final int[] keyButtonPos  = {
            0, 3, 0, 3, 3, 0, 6, 3, 0, 9, 3, 0, 12, 3, 0, 15, 3, 0, 18, 3, 0, 21, 3, 0, 24, 3, 0, 27, 3, 0, 30, 3, 0,
            0, 3, 1, 3, 3, 1, 6, 3, 1, 9, 3, 1, 12, 3, 1, 15, 3, 1, 18, 3, 1, 21, 3, 1, 24, 3, 1, 27, 3, 1, 30, 3, 1,
            0, 3, 2, 3, 3, 2, 6, 3, 2, 9, 3, 2, 12, 3, 2, 15, 3, 2, 18, 3, 2, 21, 3, 2, 24, 3, 2, 27, 3, 2, 30, 3, 2,
            0, 3, 3, 3, 3, 3, 6, 3, 3, 9, 3, 3, 12, 9, 3, 21, 3, 3, 24, 3, 3, 27, 3, 3, 30, 3, 3
    };

    public static final int[] menuProps     = {
            ZX_PROP_SHOW_KEY, ZX_PROP_SHOW_JOY, ZX_PROP_SND_LAUNCH, ZX_PROP_TRAP_TAPE, 0,
            ZX_PROP_TURBO_MODE, ZX_PROP_EXECUTE, ZX_PROP_SHOW_DEBUGGER,
            ZX_PROP_SHOW_LABEL, ZX_PROP_SHOW_CODE, ZX_PROP_SHOW_CODE_VALUE
    };

    public static final CharSequence[] names = {
            "N/A", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
            "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
            "PLOT", "DRAW", "REM", "RUN", "RAND", "RET", "IF", "POKE", "INPUT", "PRINT", "SIN", "COS", "TAN", "INT", "RND", "STR$", "CHR$",
            "CODE", "PEEK", "TAB", "ASN", "ACS", "ATN", "VERIF", "MERGE", "OUT", "AND", "OR", "AT", "INKY$", "FN",
            "NEW", "SAVE", "DIM", "FOR", "GOTO", "GOSUB", "LOAD", "LIST", "LET", "COPY", "CLEAR", "READ", "REST", "DATA", "SGN", "ABS", "VAL",
            "LEN", "USR", "STOP", "NOT", "STEP", "TO", "THEN", "CIRCL", "VAL$", "SCRN$",
            "ATTR", "LN", "BEEP", "INK", "PAPER", "FLUSH", "BRIGH", "OVER", "INVER", "CONT", "CLS", "BORD", "NEXT", "PAUSE", "POINT",
            "*", "=", "<>", "<", ">", ">=", "<=", ",", "/", "?", ".", ":", ";", "&", "%", "+", "-", "_", "'", "\"", "EXP", "LPRINT", "LLIST",
            "BIN", "", "!", "#", "$", "(", ")", "@", "[", "]", "{", "}", "DEF FN", "OPEN", "CLOSE", "FORMAT", "LINE", "ERASE", "MOVE", "CAT",
            "[C]", "[G]", "true", "inv", "SQR", "PI", "[L]", "[E]", "~", "|", "IN", "TILE", "BREAK"
    };
}
