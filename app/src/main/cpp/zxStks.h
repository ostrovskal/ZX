//
// Created by Сергей on 29.11.2019.
//

#pragma once

#pragma pack(push, 1)

struct PRESET {
    char name[31];
    uint8_t joyType;
    uint8_t joyL, joyR, joyU, joyD;
    uint8_t joyX, joyY, joyA, joyB;
};

struct HEADER_TGA {
    uint8_t	    bIdLength;		//
    uint8_t	    bColorMapType;	// тип цветовой карты ()
    uint8_t	    bType;			// тип файла ()
    uint16_t	wCmIndex;		// тип индексов в палитре
    uint16_t	wCmLength;		// длина палитры
    uint8_t	    bCmEntrySize;	// число бит на элемент палитры
    uint16_t	wXorg;			//
    uint16_t	wYorg;			//
    uint16_t	wWidth;			// ширина
    uint16_t	wHeight;		// высота
    uint8_t	    bBitesPerPixel;	// бит на пиксель
    uint8_t	    bFlags;			//
};

struct HEAD1_Z80 {
    uint16_t AF, BC, HL, PC, SP;
    uint8_t I, R;
    uint8_t STATE1;				// 0 - 7 бит R, 1-3 - бордер
    uint16_t DE;
    uint16_t BC_, DE_, HL_, AF_;
    uint16_t IY, IX;
    uint8_t IFF1, IFF2;
    uint8_t STATE2;				// 0-1 IM
};

struct HEAD2_Z80 {
    HEAD1_Z80 head1;

    uint16_t length;	    	// длина HEAD2 + HEAD3 + если экстра байт для скорпиона
    uint16_t PC;				// новый ПС
    uint8_t hardMode;			// тип оборудования
    uint8_t hardState;			// значение порта 7ffd для 128К
    uint8_t interState;			// 255 - ROM1?
    uint8_t emulateFlags;		// 7 - признак мрдифицированного оборудования
    uint8_t sndChipRegNumber;	// номер последнего AY регистра
    uint8_t sndRegs[16];		// содержимое всех AY регистров
};

struct HEAD3_Z80 {
    HEAD2_Z80 head2;

    uint16_t lowTSTATE;			// не знаю зачем он нужен
    uint8_t highTSTATE;			// аналогично
    uint8_t reserved;			// резерв
    uint8_t mgtRomPaged;		// принтер - мне не надо
    uint8_t multifaceRomPaged;	// мне не надо
    uint8_t mem0000_1FFF;		// мне не надо
    uint8_t mem2000_3FFF;		// мне не надо
    uint8_t keyMaps[10];		// я сделал по своему
    uint8_t asciiVals[10];		// аналогично
    uint8_t mgtType;			// тип принтера
    uint8_t inhibitorButton;	// магическая кнопка - нахрена?
    uint8_t inhibitorFlag;		// аналогично
    uint8_t port1FFD;			// значение порта 1ffd для скорпиона (типа версия 4)
    // блоки данных:
    // 2 - длина блока(65535 - не сжатый, иначе длина сжатой страницы)
    // 1 - номер страницы(48К : 4 - 2, 5 - 0, 8 - 5, 128К и пентагон : 3-10 - 0-7, скорпион: 3-18 - 0-15)
    // типы оборудования:
    // 48К - 2(0,1) 3(0,1) 128К - все кроме(3(3)),12,13, пентагон - 9, скорпион - 10
};
#pragma pack(pop)
