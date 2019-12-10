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

    // DEBUGGER ACTION
    public final static int DEBUGGER_ACT_UNDO       = 0;
    public final static int DEBUGGER_ACT_BP         = 1;
    public final static int DEBUGGER_ACT_REDO       = 2;
    public final static int DEBUGGER_ACT_BP_LIST    = 3;
    public final static int DEBUGGER_ACT_ACTION     = 4;
    public final static int DEBUGGER_ACT_PREV       = 5;
    public final static int DEBUGGER_ACT_MODE       = 6;
    public final static int DEBUGGER_ACT_NEXT       = 7;
    public final static int DEBUGGER_ACT_TRACE_IN   = 8;
    public final static int DEBUGGER_ACT_TRACE_OUT  = 9;
    public final static int DEBUGGER_ACT_SET_PC     = 10;
    public final static int DEBUGGER_ACT_SET_SP     = 11;
    public final static int DEBUGGER_ACT_SET_ASM    = 12;

    // UPDATE DEBUGGER
    public final static int ZX_PC                   = 1;
    public final static int ZX_REG                  = 2;
    public final static int ZX_SP                   = 4;
    public final static int ZX_SEL                  = 8;
    public final static int ZX_ALL                  = ZX_PC | ZX_REG | ZX_SP | ZX_SEL;

    // REGISTERS
    public final static int ZX_CPU_F                = 6;
    public final static int ZX_CPU_RAM              = 35;
    public final static int ZX_CPU_ROM              = 36;
    public final static int ZX_CPU_VID              = 37;
    public final static int ZX_CPU_AY               = 42;
    public final static int ZX_CPU_RR               = 25;
    public final static int ZX_CPU_AF1              = 6;
    public final static int ZX_CPU_AF2              = 14;
    public final static int ZX_CPU_HL1              = 4;
    public final static int ZX_CPU_HL2              = 12;
    public final static int ZX_CPU_DE1              = 2;
    public final static int ZX_CPU_DE2              = 10;
    public final static int ZX_CPU_BC1              = 0;
    public final static int ZX_CPU_BC2              = 8;
    public final static int ZX_CPU_IX               = 16;
    public final static int ZX_CPU_IY               = 18;
    public final static int ZX_CPU_PC               = 22;
    public final static int ZX_CPU_SP               = 20;
    public final static int ZX_CPU_STATE            = 29;

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
    public final static int MENU_PROPS_TRACER       = 1017;
    public final static int MENU_DEBUGGER_HEX_DEC   = 1018;
    public final static int MENU_DEBUGGER_ADDRESS   = 1019;
    public final static int MENU_DEBUGGER_CODE      = 1020;
    public final static int MENU_DEBUGGER_VALUE     = 1021;
    public final static int MENU_MRU                = 1022;
    public final static int MENU_POKES              = 1023;
    public final static int MENU_DEBUGGER1          = 1024;

    public final static int MENU_DISK_A             = 1200;
    public final static int MENU_DISK_B             = 1201;
    public final static int MENU_DISK_C             = 1202;
    public final static int MENU_DISK_D             = 1203;

    public final static int MENU_MODEL_48KK         = 1300;
    public final static int MENU_MODEL_48KS         = 1301;
    public final static int MENU_MODEL_128K         = 1302;
    public final static int MENU_MODEL_PENTAGON     = 1303;
    public final static int MENU_MODEL_SCORPION     = 1304;

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

    // Разделяемые свойства

    // 0. Байтовые значения, вычисляемые во время работы программы
    public static final int ZX_PROP_JOY_TYPE        = 68; // Текущий тип джойстика
    public static final int ZX_PROP_JOY_KEYS        = 69; // Привазанные к джойстику коды кнопок клавиатуры (8) 69 - 76
    public static final int ZX_PROP_JOY_CROSS_VALUE = 77; // Нажатые кнопки джойстика-крестовины
    public static final int ZX_PROP_JOY_ACTION_VALUE= 78; // Нажатые кнопки джойстика-управления
    public static final int ZX_PROP_KEY_CURSOR_MODE = 79; // Режим курсора (E, G, L, K т.п.)
//    public static final int ZX_PROP_KEY_MODE        = 80; // Режим клавиатуры (CAPS LOCK, SYMBOL SHIFT)
//    public static final int ZX_PROP_VALUES_SEMI_ROW = 81; // Значения в полурядах клавиатуры (8) 81 - 88
//    public static final int ZX_PROP_VALUES_KEMPSTON = 89; // Значение для кемпстон-джойстика
    public static final int ZX_PROP_VALUES_BUTTON     = 322; // Значение для обновления кнопок клавиатуры(текст, иконка) (42 * 2) 322 - 405

    // 1. Булевы значения
    public static final int ZX_PROP_FIRST_LAUNCH    = 128; // Признак первого запуска
    public static final int ZX_PROP_TRAP_TAPE       = 129; // Признак перехвата загрузки/записи с ленты
    public static final int ZX_PROP_SHOW_JOY        = 130; // Признак отображения джойстика
    public static final int ZX_PROP_SHOW_KEY        = 131; // Признак отображения клавиатуры
    public static final int ZX_PROP_SHOW_FPS        = 132; // Признак отображения FPS
    public static final int ZX_PROP_TURBO_MODE      = 133; // Признак турбо-режима процессора
    public static final int ZX_PROP_SND_LAUNCH      = 134; // Признак запуска звукового процессора
    public static final int ZX_PROP_SND_BP          = 135; // Признак запуска бипера
    public static final int ZX_PROP_SND_AY          = 136; // Признак трехканального AY
    public static final int ZX_PROP_SND_8BIT        = 137; // Признак 8 битного звука
    public static final int ZX_PROP_SND_SAVE        = 138; // Признак прямой записи
    public static final int ZX_PROP_SKIP_FRAMES     = 139; // Признак пропуска кадров при отображений
    public static final int ZX_PROP_EXECUTE         = 140; // Признак выполнения программы
    public static final int ZX_PROP_SHOW_DEBUGGER   = 141; // Признак режима отладчика
    public static final int ZX_PROP_TRACER          = 142; // Признак записи трассировки
    public static final int ZX_PROP_SHOW_HEX        = 143; // Признак 16-тиричного вывода
    public static final int ZX_PROP_SHOW_ADDRESS    = 144; // Признак отображения адреса инструции
    public static final int ZX_PROP_SHOW_CODE       = 145; // Признак режима кода инструкции
    public static final int ZX_PROP_SHOW_CODE_VALUE = 146; // Признак режима содержимого кода
//    public static final int ZX_PROP_EXIT_ERROR    = 147; // Признак завершения с ошибкой(не загружать состояние)

    // 2. Байтовые значения
    public static final int ZX_PROP_ACTIVE_DISK     = 150; // Номер активного диска
    public static final int ZX_PROP_BORDER_SIZE     = 151; // Размер границы
    public static final int ZX_PROP_MODEL_TYPE      = 152; // Модель памяти
    public static final int ZX_PROP_SND_TYPE_AY     = 153; // Тип каналов в звуковом процессоре AY
    public static final int ZX_PROP_SND_FREQUENCY   = 154; // Частота звука
    public static final int ZX_PROP_SND_VOLUME_BP   = 155; // Громкость бипера
    public static final int ZX_PROP_SND_VOLUME_AY   = 156; // Громкость AY
    public static final int ZX_PROP_BLINK_SPEED     = 157; // Скорость мерцания курсора
    public static final int ZX_PROP_CPU_SPEED       = 158; // Скорость процессора
    public static final int ZX_PROP_KEY_SIZE        = 159; // Размер экранной клавиатуры
    public static final int ZX_PROP_JOY_SIZE        = 160; // Размер экранного джойстика

    // 3. Целые значения
    public static final int ZX_PROP_COLORS          = 170; // значения цветов (16 * 4) 170 - 233

    // 4. Значение структур
    public static final int ZX_PROP_BPS             = 192; // значения точек останова (8 * 8) 258 - 321

    public static final int ZX_PROPS_COUNT          = 410; // Размер буфера свойств
    public static final int ZX_PROPS_INIT_COUNT     = (ZX_PROP_BPS - ZX_PROP_FIRST_LAUNCH) + 8; // Количество свойств

    // Модели памяти
/*
    public static final int MODEL_48KK              = 0; // Компаньон 2.02 48К
    public static final int MODEL_48K               = 1; // Синклер 48К
    public static final int MODEL_128K              = 2; // Синклер 128К
    public static final int MODEL_PENTAGON          = 3; // Пентагон 128К
    public static final int MODEL_SCORPION          = 4; // Скорпион 256К
*/

    // Варианты форматирования чисел
    public static final int ZX_FV_CODE_LAST			= 0; // "3X", "2X"
    public static final int ZX_FV_CODE				= 2; // "3X ", "2X "
    public static final int ZX_FV_PADDR16			= 4; // "5(X)", "4(#X)"
    public static final int ZX_FV_PADDR8			= 6; // "3(X)", "2(#X)"
    public static final int ZX_FV_OFFS			    = 8; // "3+-X)", "2+-#X)"
    public static final int ZX_FV_NUM16				= 10;// "5X", "4X"
    public static final int ZX_FV_OPS16				= 12;// "5X", "4#X"
    public static final int ZX_FV_OPS8				= 14;// "3X", "2#X"
    public static final int ZX_FV_CVAL				= 16;// "5[X]", "4[#X]"
    public static final int ZX_FV_PVAL8				= 18;// "3{X}", "2{#X}"
    public static final int ZX_FV_PVAL16			= 20;// "3{X}", "2{#X}"
    public static final int ZX_FV_NUMBER			= 22;// "0X", "0X"
    public static final int ZX_FV_SIMPLE			= 24;// "0X", "0X"

    // Режимы курсора
    public static final byte MODE_K                 = 0;
    public static final byte MODE_L                 = 1;
    public static final byte MODE_C                 = 2;
    public static final byte MODE_E                 = 3;
    public static final byte MODE_SK                = 4;
    public static final byte MODE_SE                = 5;
    public static final byte MODE_CL                = 6;
    public static final byte MODE_CK                = 7;
    public static final byte MODE_G                 = 8;
    public static final byte MODE_G1                = 9;
    public static final byte MODE_CE                = 10;

    // Команды
    public static final int ZX_CMD_MODEL            = 0;  // Установка модели памяти
    public static final int ZX_CMD_PROPS            = 1;  // Установка свойств
    public static final int ZX_CMD_RESET            = 2;  // Сброс
    public static final int ZX_CMD_UPDATE_KEY       = 3;  // Обковление кнопок
    public static final int ZX_CMD_PRESETS          = 4;  // Установка/получение пресетов джойстика
    public static final int ZX_CMD_POKE             = 5;  // Установка POKE
//    public static final int ZX_CMD_DIAG             = 6;// Диагностика
    public static final int ZX_CMD_PRESETS_SAVE     = 7;  // Сохранить параметры джойстика
    public static final int ZX_CMD_PRESETS_LOAD     = 8;  // Загрузить параметры джойстика
    public static final int ZX_CMD_PRESETS_LIST     = 9;  // Получить список пресетов
    public static final int ZX_CMD_PRESETS_NAME     = 10; // Получить имя программы
//    public static final int ZX_CMD_PRESETS_SET    = 11; // Установить имя программы
public static final int ZX_CMD_TRACER               = 12; // Запуск трасировщика
    public static final int ZX_CMD_QUICK_BP         = 13; // Быстрая установка точки останова
    public static final int ZX_CMD_TRACE_IN         = 14; // Трассировка с заходом
    public static final int ZX_CMD_TRACE_OUT        = 15; // Трассировка без захода

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
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_SIZE, 13,
            ATTR_FONT, R.string.font_small,
            ATTR_COLOR_DEFAULT, 0xf77499 | COLOR,
            ATTR_CLICKABLE, 1,
            ATTR_PADDING, 0,
            ATTR_MIN_HEIGHT, R.dimen.heightButton,
            ATTR_SSH_PRESSED_OFFS, 0,
            ATTR_SSH_WIDTH_SELECTOR, 0,
            ATTR_SSH_COLOR_SELECTOR, 0,
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
            ATTR_INPUT_TYPE, InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_CLASS_NUMBER,
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
            ATTR_FONT, R.string.font_small,
            ATTR_SIZE, R.dimen.normal,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_SPINNER | THEME,
            ATTR_MIN_HEIGHT, R.dimen.heightSelectItem,
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
            ATTR_SHADOW_TEXT, R.string.shadow_null,
            ATTR_SIZE, R.dimen.debuggerTextFlags,
            ATTR_FONT, R.string.font_small,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_MAX_LINES, 1,
            ATTR_COLOR_DEFAULT, 0xf77499 | COLOR
    };

    public static final int[] style_debugger_text = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_SIZE, R.dimen.debuggerTextDef,
            ATTR_FONT, R.string.font_small,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_MAX_LINES, 1,
            ATTR_COLOR_DEFAULT, 0xffffff | COLOR
    };

    public static final int[] style_debugger_ribbon = {
            ATTR_SELECTOR, ATTR_SSH_COLOR_SELECTOR | THEME,
            ATTR_LONG_CLICKABLE, 1,
            ATTR_PADDING, 2
    };

    public static final int[] style_debugger_item = {
            ATTR_SHADOW_TEXT, R.string.shadow_null,
            ATTR_PADDING, 3,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_COLOR_HIGHLIGHT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_GRAVITY, Gravity.START,
            ATTR_SIZE, R.dimen.debuggerTextDef
    };

    public static final int[] style_debugger_action = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_COLOR_DEFAULT, 0xf77499 | COLOR,
            ATTR_CLICKABLE, 1,
            ATTR_FOCUSABLE, 1,
            ATTR_PADDING, 0,
            ATTR_MIN_HEIGHT, R.dimen.heightButton,
            ATTR_SSH_PRESSED_OFFS, 0,
            ATTR_SSH_WIDTH_SELECTOR, 0,
            ATTR_SSH_COLOR_SELECTOR, 0,
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
            ATTR_INPUT_TYPE, android.text.InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS,
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

    public static final int[] settingsCommon    = { ZX_PROP_CPU_SPEED, ZX_PROP_BLINK_SPEED, ZX_PROP_JOY_SIZE, ZX_PROP_BORDER_SIZE, ZX_PROP_KEY_SIZE,
                                                    ZX_PROP_SHOW_FPS, ZX_PROP_SKIP_FRAMES };
    public static final int[] settingsCheckSnd  = { ZX_PROP_SND_BP, ZX_PROP_SND_AY, ZX_PROP_SND_8BIT, ZX_PROP_SND_SAVE };
    public static final int[] settingsSnd       = { ZX_PROP_SND_TYPE_AY, ZX_PROP_SND_FREQUENCY, ZX_PROP_SND_VOLUME_BP, ZX_PROP_SND_VOLUME_AY };
    public static final int[] settingsAllSnd    = { 0, ZX_PROP_SND_TYPE_AY, 0, ZX_PROP_SND_FREQUENCY, 0, ZX_PROP_SND_VOLUME_BP, 0, ZX_PROP_SND_VOLUME_AY,
                                                    ZX_PROP_SND_BP, ZX_PROP_SND_AY, ZX_PROP_SND_8BIT, ZX_PROP_SND_SAVE};
}
