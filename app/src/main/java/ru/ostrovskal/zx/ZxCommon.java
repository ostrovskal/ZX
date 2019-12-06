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

    // Формы эмулятора
    public final static int FORM_MAIN               = 0;
    public final static int FORM_OPTIONS            = 1;
    public final static int FORM_IO                 = 2;
    public final static int FORM_POKES              = 3;
    public final static int FORM_LOADING            = 4;

    // Разделяемые свойства

    // 0. Байтовые значения, вычисляемые во время работы программы
    public static final int ZX_PROP_JOY_TYPE        = 80; // Текущий тип джойстика
    public static final int ZX_PROP_JOY_KEYS        = 81; // Привазанные к джойстику коды кнопок клавиатуры (8) 81 - 88
    public static final int ZX_PROP_JOY_CROSS_VALUE = 89; // Нажатые кнопки джойстика-крестовины
    public static final int ZX_PROP_JOY_ACTION_VALUE= 90; // Нажатые кнопки джойстика-управления
    public static final int ZX_PROP_KEY_CURSOR_MODE = 91; // Режим курсора (E, G, L, K т.п.)
    public static final int ZX_PROP_KEY_MODE        = 92; // Режим клавиатуры (CAPS LOCK, SYMBOL SHIFT)
    public static final int ZX_PROP_VALUES_SEMI_ROW = 93; // Значения в полурядах клавиатуры (8) 93 - 100
    public static final int ZX_PROP_VALUES_KEMPSTON = 101; // Значение для кемпстон-джойстика

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
    public static final int ZX_PROP_SHOW_HEX        = 141; // Признак 16-тиричного вывода

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

    public static final int ZX_PROPS_COUNT          = 267; // Размер буфера свойств
    public static final int ZX_PROPS_INIT_COUNT     = (ZX_PROP_COLORS - ZX_PROP_FIRST_LAUNCH) +  22; // Количество свойств

    // Модели памяти
    public static final int MODEL_48KK              = 0; // Компаньон 2.02 48К
    public static final int MODEL_48K               = 1; // Синклер 48К
    public static final int MODEL_128K              = 2; // Синклер 128К
    public static final int MODEL_PENTAGON          = 3; // Пентагон 128К
    public static final int MODEL_SCORPION          = 4; // Скорпион 256К

    // Варианты форматирования чисел
    public static final int ZX_FV_CODE_LAST			= 0; // "3X", "2X"
    public static final int ZX_FV_CODE				= 2; // "3X ", "2X "
    public static final int ZX_FV_PADDR16			= 4; // "5(X)", "4(#X)"
    public static final int ZX_FV_PADDR8			= 6; // "3(X)", "2(#X)"
    public static final int ZX_FV_P_OFFS			= 8; // "3+X)", "2+#X)"
    public static final int ZX_FV_M_OFFS			= 10;// "3-X)", "2-#X)"
    public static final int ZX_FV_NUM16				= 12;// "5X", "4X"
    public static final int ZX_FV_OPS16				= 14;// "5X", "4#X"
    public static final int ZX_FV_OPS8				= 16;// "3X", "2#X"
    public static final int ZX_FV_CVAL				= 18;// "5[X]", "4[#X]"
    public static final int ZX_FV_PVAL				= 20;// "3{X}", "2{#X}"
    public static final int ZX_FV_NUMBER			= 22;// "0X", "0X"

    // Режимы курсора
    public static final byte MODE_K                 = 0;
    public static final byte MODE_L                 = 1;
    public static final byte MODE_C                 = 2;
    public static final byte MODE_E                 = 3;
    public static final byte MODE_E1                = 4;
    public static final byte MODE_E2                = 5;
    public static final byte MODE_CL                = 6;
    public static final byte MODE_CK                = 7;
    public static final byte MODE_G                 = 8;
    public static final byte MODE_G1                = 9;

    // Команды
    public static final int ZX_CMD_MODEL            = 0; // Установка модели памяти
    public static final int ZX_CMD_PROPS            = 1; // Установка свойств
    public static final int ZX_CMD_RESET            = 2; // Сброс
    public static final int ZX_CMD_UPDATE_KEY       = 3; // Обковление кнопок
    public static final int ZX_CMD_PRESETS          = 4; // Установка/получение пресетов джойстика
    public static final int ZX_CMD_POKE             = 5; // Установка POKE
    public static final int ZX_CMD_DIAG             = 6; // Диагностика
    public static final int ZX_CMD_PRESETS_SAVE     = 7; // Сохранить параметры джойстика
    public static final int ZX_CMD_PRESETS_LOAD     = 8; // Загрузить параметры джойстика
    public static final int ZX_CMD_PRESETS_LIST     = 9; // Получить список пресетов
    public static final int ZX_CMD_PRESETS_NAME     = 10;// Получить имя программы

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
            ATTR_SSH_ICON_VERT, 8,
            ATTR_SSH_ICON_HORZ, 8,
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
            ATTR_SSH_SCALE, TILE_SCALE_MIN,
            ATTR_SSH_GRAVITY, TILE_GRAVITY_LEFT | TILE_GRAVITY_CENTER_VERT,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_CHECK | THEME
    };

    public static final int[] style_tab_settins = {
            ATTR_SSH_VERT, 1,
            ATTR_SSH_HORZ, 2,
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
            ATTR_GRAVITY, Gravity.CENTER_VERTICAL,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_SIZE, 8,
            ATTR_FONT, R.string.font_normal
    };

    public static final int[] style_color_text_settings = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_SIZE, 8,
            ATTR_FONT, R.string.font_small,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_COLOR_DEFAULT, 0xffffff | COLOR
    };

    public static final int[] style_spinner_title_settings = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_COLOR_HIGHLIGHT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_PADDING_HORZ, R.dimen.paddingHorzSelectItem,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_SIZE, 14,
            ATTR_FONT, R.string.font_small,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_SPINNER | THEME,
            ATTR_SSH_VERT, 3,
            ATTR_SSH_TILE, 0
    };

    public static final int[] style_spinner_item_settings = {
            ATTR_SHADOW_TEXT, R.string.shadow_text,
            ATTR_COLOR_DEFAULT, ATTR_SSH_COLOR_LARGE | THEME,
            ATTR_COLOR_HIGHLIGHT, ATTR_SSH_COLOR_NORMAL | THEME,
            ATTR_GRAVITY, Gravity.CENTER,
            ATTR_SIZE, 14,
            ATTR_FONT, R.string.font_small,
            ATTR_SSH_BITMAP_NAME, ATTR_SSH_BM_SPINNER | THEME,
            ATTR_MIN_HEIGHT, R.dimen.heightSelectItem,
            ATTR_PADDING_HORZ, R.dimen.paddingHorzSelectItem,
            ATTR_SSH_VERT, 3,
            ATTR_SSH_PATCH9, R.string.patch9_select_item,
            ATTR_SSH_TILE, 2
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
