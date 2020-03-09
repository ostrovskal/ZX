//
// Created by Сергей on 29.11.2019.
//

#pragma once

#pragma pack(push, 1)
struct HEAD_SNA {
	enum { S_48 = 49179, S_128_5 = 131103, S_128_6 = 147487 };
	uint8_t I;
	uint16_t HL_, DE_, BC_, AF_;
	uint16_t HL, DE, BC, IY, IX;
	uint8_t IFF1;                       // 00 - reset, FF - set
	uint8_t R;
	uint16_t AF, SP;
	uint8_t IM, _FE;
	uint8_t PAGE5[16384];	            // 4000-7FFF
	uint8_t PAGE2[16384];	            // 8000-BFFF
	uint8_t PAGEX[16384];	            // C000-FFFF
	// 128k extension
	uint16_t PC;
	uint8_t _7FFD;
	uint8_t TRDOS;
	uint8_t* PAGES;                     // Остальные страницы для 128K
};

struct HEAD_Z80 {
	uint8_t A, F;
	uint16_t BC, HL, PC, SP;
	uint8_t I, R;
	uint8_t STATE1;				// 0 -> 7 бит R, 1-3 -> бордер
	uint16_t DE;
	uint16_t BC_, DE_, HL_;
	uint8_t A_, F_;
	uint16_t IY, IX;
	uint8_t IFF1, IFF2, IM;
	/* 2.01 extension */
	uint16_t length;	    	// длина HEAD2 + HEAD3 + если экстра байт для скорпиона
	uint16_t PC_;				// новый ПС
	uint8_t model;  			// тип оборудования
	uint8_t _7FFD;	    		// значение порта 7ffd для 128К
	uint8_t ROM1; 			    // 255 - ROM1?
	uint8_t emulateFlags;		// 7 - признак мрдифицированного оборудования
	uint8_t sndChipRegNumber;	// номер последнего AY регистра
	uint8_t sndRegs[16];		// содержимое всех AY регистров
	/* 3.0 extension */
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
	uint8_t _1FFD;			    // значение порта 1ffd для скорпиона (типа версия 4)
	// блоки данных:
	// 2 - длина блока(65535 - не сжатый, иначе длина сжатой страницы)
	// 1 - номер страницы(48К : 4 - 2, 5 - 0, 8 - 5, 128К и пентагон : 3-10 - 0-7, скорпион: 3-18 - 0-15)
	// типы оборудования:
	// 48К - 2(0,1) 3(0,1) 128К - все кроме(3(3)),12,13, пентагон - 9, скорпион - 10
};

struct WAV {
    uint32_t fileID;		// RIFF
    uint32_t fileSize;		// len + 8 + 12 + 16
    uint32_t waveID;		// WAVE
    uint32_t chunkID;		// fmt_
    uint32_t wFormatTag;	// 16
    uint16_t nChannels;		// 1/2
    uint16_t nChannels1;	// 1/2
    uint32_t nSamplesPerSec;// 11025/22050/44100
    uint32_t nAvgBytesPerSec;// 11025/22050/44100
    uint16_t nBlockAlign;	// 1
    uint16_t wBitsPerSample;// 8/16
    uint32_t nData;			// data
    uint32_t nDataSize;		// len
};

#pragma pack(pop)
