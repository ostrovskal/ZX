//
// Created by Sergey on 10.02.2020.
//

#include "zxCommon.h"
#include "zxFormats.h"
#include "zxStks.h"

static void packPage(uint8_t** buffer, uint8_t* src, uint8_t page) {
    uint32_t size;
    uint8_t* buf = *buffer;
    *buffer = packBlock(src, src + 16384, &buf[3], false, size);
    *(uint16_t*)buf = (uint16_t)size;
    buf[2] = page;
}

bool zxFormats::openZ80(uint8_t* ptr, size_t size) {
    uint8_t model = MODEL_48;
    int version;
    int pages = 3;

    auto head = (HEAD_Z80*)ptr;
    auto length = 30;
    auto buf = &TMP_BUF[INDEX_OPEN];
    auto isCompressed = (head->STATE1 & 0x20) == 0x20;
    auto PC = head->PC;
    if(PC == 0) {
        // версия 2 или 3
        auto mode = head->model;
        length += head->length + 2;
        switch(length) {
            case 55:
                version = 2;
                model = (uint8_t)(mode < 3 ? MODEL_48 : MODEL_128);
                break;
            case 86: case 87:
                version = 3;
                model = (uint8_t)(mode < 4 ? MODEL_48 : MODEL_128);
                break;
            default: LOG_INFO("Недопустимый формат файла!", 0); return false;
        }
        if(model == MODEL_48) {
            pages = (head->emulateFlags & 0x80) ? 1 : 3;
        } else if(model == MODEL_128) {
            if(mode == 9) {
                model = MODEL_PENTAGON;
                pages = 32;
            } else if(mode == 10) {
                model = MODEL_SCORPION;
                pages = 16;
            } else pages = 8;
        } else {
            LOG_INFO("Неизвестное оборудование %i", mode);
            return false;
        }
        LOG_DEBUG("version:%i length:%i model:%i pages:%i mode:%i model:%i", version, length, model, pages, mode, model);
        PC = head->PC_;
    }
    ptr += length;
    size -= length;
    // формируем страницы во временном буфере
    if(length > 30) {
        for(int i = 0 ; i < pages; i++) {
            auto sizeData = *(uint16_t*)ptr; ptr += 2;
            auto numPage = *ptr++ - 3;
            isCompressed = sizeData != 0xFFFF;
            auto sizePage = (uint32_t)(isCompressed ? sizeData : 0x4000);
            auto isValidPage = false;
            switch(model) {
                case MODEL_48:
                    isValidPage = true;
                    switch(numPage) {
                        // 4->2 5->0 8->5
                        case 1: numPage = 2; break;
                        case 2: numPage = 0; break;
                        case 5: numPage = 5; break;
                        default: isValidPage = false; break;
                    }
                    break;
                case MODEL_128:
                case MODEL_PENTAGON:
                case MODEL_SCORPION:
                    isValidPage = numPage == i;
                    break;
            }
            if(!isValidPage) {
                LOG_DEBUG("Неизвестная страница %i-%i!", numPage, i);
                return false;
            }
            auto page = &buf[numPage << 14];
            if(!unpackBlock(ptr, page, page + 16384, sizePage, isCompressed)) {
                LOG_DEBUG("Ошибка при распаковке страницы %i!", numPage);
                return false;
            }
            ptr += sizePage;
            size -= (sizePage + 3);
        }
    } else {
        if(!unpackBlock(ptr, buf, &buf[49152], size, isCompressed)) {
            LOG_DEBUG("Ошибка при распаковке!", 0);
            return false;
        }
        // перераспределяем буфер 0->5, 1->2, 2->0
        memcpy(&buf[5 << 14], &buf[0 << 14], 16384);
        memcpy(&buf[0 << 14], &buf[2 << 14], 16384);
        memcpy(&buf[2 << 14], &buf[1 << 14], 16384);
    }
    // меняем модель памяти и инициализируем регистры
    auto cpu = zx->cpu;
    zx->changeModel(model);
    *cpu->_BC   = head->BC;     *cpu->_DE   = head->DE; *cpu->_HL = head->HL;
    *cpu->_A    = head->A;      *cpu->_F    = head->F;  opts[RA_] = head->A_; opts[RF_] = head->F_;
    *cpu->_SP   = head->SP;     *cpu->_IX   = head->IX; *cpu->_IY = head->IY;
    *cpu->_I    = head->I;      *cpu->_IM   = head->IM; *cpu->_PC = PC;
    *cpu->_IFF1 = head->IFF1;   *cpu->_IFF2 = head->IFF2;
    *cpu->_R    |= ((head->STATE1 & 1) << 7) | (head->R & 0x7F);
    zx->writePort(0xfe, 0, (uint8_t)(224 | ((head->STATE1 & 14) >> 1)));
    memcpy(&opts[RC_], &head->BC_, 6);
    LOG_INFO("Z80 Start PC: %i", PC);
    if(length > 30) {
        zx->writePort(0xfd, 0x7f, head->_7FFD);
        memcpy(&opts[AY_AFINE], head->sndRegs, 16);
        zx->writePort(0xfd, 0xff, head->sndChipRegNumber);
    }
    if(length == 87) zx->writePort(0xfd, 0x1f, head->_1FFD);
    // копируем буфер
    memcpy(zx->RAMbs, buf, ZX_TOTAL_RAM);
    return true;
}

uint8_t* zxFormats::saveZ80() {
    static uint8_t models[] = { 0, 0, 0, 4, 4, 4, 9, 10 };
    static HEAD_Z80 h_z80;

    memset(&h_z80, 0, sizeof(HEAD_Z80));
    auto head = & h_z80;

    auto buf = TMP_BUF;
    auto cpu = zx->cpu;
    auto ram = zx->RAMbs;
    // основные
    head->BC    = *cpu->_BC;    head->DE    = *cpu->_DE;    head->HL   = *cpu->_HL;     head->model = models[*zx->_MODEL];
    head->A     = *cpu->_A;     head->F     = *cpu->_F;     head->A_   = opts[RA_];     head->F_    = opts[RF_];
    head->SP    = *cpu->_SP;    head->IX    = *cpu->_IX;    head->IY   = *cpu->_IY;     head->_1FFD = opts[PORT_1FFD];
    head->IM    = *cpu->_IM;    head->IFF1  = *cpu->_IFF1;  head->IFF2 = *cpu->_IFF2;   head->_7FFD = opts[PORT_7FFD];
    head->I     = *cpu->_I;     head->length= 55;           head->PC   = 0;             head->PC_   = *cpu->_PC;
    head->R     = (uint8_t)(*cpu->_R & 127);
    head->STATE1= (uint8_t)((*cpu->_R & 128) >> 7) | (uint8_t)((opts[PORT_FE] & 7) << 1);
    memcpy(&head->BC_, &opts[RC_], 6);
    // для режима 128К
    head->sndChipRegNumber = opts[AY_REG];
    memcpy(head->sndRegs, &opts[AY_AFINE], 16);
    ssh_memzero(head->sndRegs, 16);
    // формируем буфер из содержимого страниц
    ssh_memcpy(&buf, &head, sizeof(HEAD_Z80));
    // страницы, в зависимости от режима
    if(head->model < MODEL_128) {
        // 4->2 5->0 8->5
        packPage(&buf, &ram[5 << 14], 8);
        packPage(&buf, &ram[2 << 14], 4);
        packPage(&buf, &ram[0 << 14], 5);
    } else {
        auto count = zxSpeccy::machine->ramPages;
        for(int i = 0; i < count; i++) {
            packPage(&buf, &ram[i << 14], (uint8_t)(i + 3));
        }
    }
    return buf;
}

bool zxFormats::openTAP(zxDevTape* tape, uint8_t* ptr, size_t size) {
    auto buf = ptr;
    while(buf < ptr + size) {
        auto sz = *(uint16_t*)buf; buf += 2;
        if(!sz) break;
        tape->makeBlock(TZX_NORMAL_BLOCK, buf, sz, 2168, 667, 735, 855, 1710, (uint16_t)((*buf < 4) ? 8064 : 3224), 1000);
        buf += sz;
    }
    return (buf == ptr + size);
}

uint8_t* zxFormats::saveTAP(zxDevTape* tape) {
    auto ptr = TMP_BUF;
    uint8_t idx = 0;
    while(idx < tape->countBlocks) {
        auto blk = tape->blocks[idx++];
        if(blk->type == TZX_NORMAL_BLOCK) {
            auto size = blk->data_size;
            memcpy(ptr, blk->data, size);
            ptr += size;
        }
    }
    return ptr;
}

bool zxFormats::openTZX(zxDevTape* tape, uint8_t* ptr, size_t size) {
	if(ptr[0] != 'Z' && ptr[1] != 'X' && ptr[2] != 'T' && ptr[3] != 'a' && ptr[4] != 'p' && ptr[5] != 'e' && ptr[6] != '!' && ptr[7] != 0x1A)
		return false;
	auto v_minor = ptr[8];
	auto v_major = ptr[9];
    auto buf = ptr + 10; auto bufe = ptr + size;
    uint16_t tmp0, tmp1;
    while(buf < bufe) {
        switch(*buf++) {
            case TZX_NORMAL_BLOCK:
		        tmp0 = wordLE(buf + 0);
		        tmp1 = wordLE(buf + 2); buf += 4;
		        tape->makeBlock(TZX_NORMAL_BLOCK, buf, tmp1, 2168, 667, 735, 855, 1710, (uint16_t)((*buf < 4) ? 8064 : 3223), tmp0);
		        buf += tmp1;
                break;
            case TZX_TURBO_BLOCK:
		        tmp0 = (uint16_t)(dwordLE(buf + 15) & 0xFFFFFF);
		        tape->makeBlock(TZX_TURBO_BLOCK, buf + 18, tmp0, wordLE(buf), wordLE(buf + 2), wordLE(buf + 4), wordLE(buf + 6),
		        		wordLE(buf + 8), wordLE(buf + 10), wordLE(buf + 13), buf[12]);
		        buf += tmp0 + 18;
                break;
            case TZX_PURE_TONE:
	            tmp0 = wordLE(buf + 0);// длина импульса
		        tmp1 = wordLE(buf + 2);// количество импульсов
		        tape->makeBlock(TZX_PURE_TONE, nullptr, 0, tmp0, 0, 0, 0, 0, tmp1, 0, 0);
		        buf += 4;
                break;
            case TZX_SEQ_PULSES_DIFF_LEN:
		        tmp0 = *buf++;// количество импульсов
		        tape->makeBlock(TZX_PURE_TONE, buf, 0, (uint16_t)(tmp0 * 2), 0, 0, 0, 0, 0, 0, 0);
		        buf += tmp0 * 2;
                break;
            case TZX_PURE_DATA_BLOCK:
		        tmp0 = (uint16_t)(dwordLE(buf + 7) & 0xFFFFFF);
		        tape->makeBlock(TZX_PURE_DATA_BLOCK, buf + 10, tmp0, 0, 0, 0, wordLE(buf), wordLE(buf + 2), 0, wordLE(buf + 5), buf[4]);
		        buf += tmp0 + 10;
                break;
            case TZX_DIRECT_RECORD: {
	            tmp0 = wordLE(buf);
	            tmp1 = (uint16_t) (dwordLE(buf + 5) & 0xFFFFFF);
	            auto pause_t = wordLE(buf + 2);
	            auto last_t = buf[4];
	            buf += 8;
/*
		        pl = 0;
		        n = 0;
		        // подсчет числа импульсов
		        for(int i = 0; i < tmp1; i++) {
			        for(j = 128; j; j >>= 1) {
				        if((buf[i] ^ pl) & j) n++, pl ^= 0xFF;
					}
				}
		        t = 0;
		        pl = 0;
		        Reserve(n + 2);
		        // поиск импульсов
		        for(i = 1; i < size; i++, ptr++) {
			        for(j = 0x80; j; j >>= 1) {
				        t += t0;
				        if((*ptr ^ pl) & j) {
					        tape_image[tape_imagesize++] = FindPulse(t);
					        pl ^= -1;
					        t = 0;
				        }
			        }
				}
		        // поиск импульсов для последнего байта
		        for(j = 0x80; j != (byte)(0x80 >> last); j >>= 1) {
			        t += t0;
			        if((*ptr ^ pl) & j) {
				        tape_image[tape_imagesize++] = FindPulse(t);
				        pl ^= -1;
				        t = 0;
			        }
		        }
		        ptr++;
		        tape_image[tape_imagesize++] = FindPulse(t); // last pulse ???
		        if(pause) tape_image[tape_imagesize++] = FindPulse(pause * 3500);
*/
            }
                break;
            case TZX_PAUSE:
            	tape->makeBlock(TZX_PAUSE, nullptr, 0, 0, 0, 0, 0, 0, 0, wordLE(buf));
            	buf += 2;
                break;
            case TZX_GROUP_START:
	            tmp0 = *buf++;      // длина имени
		        buf += tmp0;
                break;
            case TZX_GROUP_END:
                break;
            case TZX_JUMP:
	            NamedCell("* jump");
	            tmp0 = wordLE(buf);     // смещение к блоку
		        buf += 2;
                break;
            case TZX_LOOP_START:
            	tmp0 = wordLE(buf);     // количество повторений
//		        loop_p = tape_imagesize;
		        buf += 2;
                break;
            case TZX_LOOP_END:
                break;
            case TZX_CALL:
            	tmp0 = wordLE(buf);     // количество вызовов
            	buf += 2 + 2 * tmp0;
                break;
            case TZX_RETURN:
                break;
            case TZX_SELECT_BLOCK:
	            sprintf(nm, "* choice: ");
		        n = ptr[2];
		        p = ptr + 3;
		        for(i = 0; i < n; i++)
		        {
			        if(i)
				        strcat(nm, " / ");
			        char *q = nm + strlen(nm);
			        size = *(p + 2);
			        memcpy(q, p + 3, size);
			        q[size] = 0;
			        p += size + 3;
		        }
		        NamedCell(nm);
		        ptr += 2 + Word(ptr);
                break;
            case TZX_STOP_IF_48K:
	            NamedCell("* stop if 48K");
		        ptr += 4 + Dword(ptr);
                break;
            case TZX_TEXT:
	            n = *ptr++;
		        NamedCell(ptr, n);
		        ptr += n;
		        appendable = 1;
                break;
            case TZX_MESSAGE:
	            NamedCell("- MESSAGE BLOCK ");
		        end = ptr + 2 + ptr[1];
		        pl = *end;
		        *end = 0;
		        for(p = ptr + 2; p < end; p++)
			        if(*p == 0x0D)
				        *p = 0;
		        for(p = ptr + 2; p < end; p += strlen((char*)p) + 1)
			        NamedCell(p);
		        *end = pl;
		        ptr = end;
		        NamedCell("-");
                break;
            case TZX_ARCHIVE_INFO:
                break;
            case TZX_HARDWARE_TYPE:
	            ParseHardware(ptr);
		        ptr += 1 + 3 * *ptr;
                break;
            case TZX_EMULATION_INFO:
	            NamedCell("* emulation info");
		        ptr += 8;
                break;
            case TZX_CUSTOM_INFO:
                break;
            case TZX_SNAPSHOT:
	            NamedCell("* snapshot");
		        ptr += 4 + (0xFFFFFF & Dword(ptr + 1));
                break;
            case TZX_Z:
	            ptr += 9;
                break;
            default:
	            ptr += data_size;
                break;
        }
    }
/*
                break;
            case 0x30: // text description
                break;
            case 0x31: // message block
                break;
            case 0x32: // archive info
                NamedCell("- ARCHIVE INFO ");
                p = ptr + 3;
                for(i = 0; i < ptr[2]; i++)
                {
                    const char *info;
                    switch(*p++)
                    {
                        case 0:
                            info = "Title";
                            break;
                        case 1:
                            info = "Publisher";
                            break;
                        case 2:
                            info = "Author";
                            break;
                        case 3:
                            info = "Year";
                            break;
                        case 4:
                            info = "Language";
                            break;
                        case 5:
                            info = "Type";
                            break;
                        case 6:
                            info = "Price";
                            break;
                        case 7:
                            info = "Protection";
                            break;
                        case 8:
                            info = "Origin";
                            break;
                        case 0xFF:
                            info = "Comment";
                            break;
                        default:
                            info = "info";
                            break;
                    }
                    dword size = *p + 1;
                    char tmp = p[size];
                    p[size] = 0;
                    sprintf(nm, "%s: %s", info, p + 1);
                    p[size] = tmp;
                    p += size;
                    NamedCell(nm);
                }
                NamedCell("-");
                ptr += 2 + Word(ptr);
                break;
            case 0x33: // hardware type
                break;
            case 0x34: // emulation info
                break;
            case 0x35: // custom info
                if(!memcmp(ptr, "POKEs           ", 16))
                {
                    NamedCell("- POKEs block ");
                    NamedCell(ptr + 0x15, ptr[0x14]);
                    p = ptr + 0x15 + ptr[0x14];
                    n = *p++;
                    for(i = 0; i < n; i++)
                    {
                        NamedCell(p + 1, *p);
                        p += *p + 1;
                        t = *p++;
                        strcpy(nm, "POKE ");
                        for(j = 0; j < t; j++)
                        {
                            sprintf(nm + strlen(nm), "%d,", Word(p + 1));
                            sprintf(nm + strlen(nm), *p & 0x10 ? "nn" : "%d",
                                    *(byte*)(p + 3));
                            if(!(*p & 0x08))
                                sprintf(nm + strlen(nm), "(page %d)", *p & 7);
                            strcat(nm, "; ");
                            p += 5;
                        }
                        NamedCell(nm);
                    }
                    nm[0] = '-';
                    nm[1] = 0;
                    nm[2] = 0;
                    nm[3] = 0;
                }
                else
                    sprintf(nm, "* custom info: %s", ptr), nm[15 + 16] = 0;
                NamedCell(nm);
                ptr += 0x14 + Dword(ptr + 0x10);
                break;
            case 0x40: // snapshot
                break;
            case 0x5A: // 'Z'
                break;
            default:
        }
    }
    for(i = 0; i < tape_infosize; i++)
    {
        if(tapeinfo[i].desc[0] == '*' && tapeinfo[i].desc[1] == ' ')
            strcat(tapeinfo[i].desc, " [UNSUPPORTED]");
        if(*tapeinfo[i].desc == '-')
            while(strlen(tapeinfo[i].desc) < sizeof(tapeinfo[i].desc) - 1)
                strcat(tapeinfo[i].desc, "-");
    }
    if(tape_imagesize && tape_pulse[tape_image[tape_imagesize - 1]] < 350000)
        Reserve(1), tape_image[tape_imagesize++] = FindPulse(350000); // small pause [rqd for 3ddeathchase]
    FindTapeSizes();
*/
	return (buf == ptr + size);
}

uint8_t* zxFormats::saveTZX(zxDevTape* tape) {
    return nullptr;
}

bool zxFormats::openCSW(zxDevTape* tape, uint8_t* ptr, size_t) {
/*
    const byte* buf = (const byte*)data;
    const dword Z80FQ = 3500000;
    NamedCell("CSW tape image");
    if(buf[0x1B] != 1)
        return false; // unknown compression type
    dword rate = Z80FQ / Word(buf + 0x19); // usually 3.5mhz / 44khz
    if(!rate)
        return false;
    Reserve(data_size - 0x18);
    if(!(buf[0x1C] & 1)) tape_image[tape_imagesize++] = FindPulse(1);
    for(const byte* ptr = (const byte*)data + 0x20; ptr < (const byte*)data + data_size;) {
        dword len = *ptr++ * rate;
        if(!len) {
            len = Dword(ptr) / rate;
            ptr += 4;
        }
        tape_image[tape_imagesize++] = FindPulse(len);
    }
    tape_image[tape_imagesize++] = FindPulse(Z80FQ / 10);
    FindTapeSizes();
    return true;
*/
    return false;
}

uint8_t* zxFormats::saveCSW(zxDevTape* tape) {
    return nullptr;
}

bool zxFormats::openTRD(zxFDD *fdd, uint8_t *ptr, size_t size) {
    fdd->make_trd();
    if(size > 655360) size = 655360;
    for(size_t i = 0; i < size; i += 256) {
        fdd->write_sec(i >> 13, (i >> 12) & 1, ((i >> 8) & 0x0f) + 1, (const uint8_t *)ptr + i);
    }
    return true;
}

uint8_t* zxFormats::saveTRD(zxFDD *fdd) {
    return nullptr;
}

bool zxFormats::openSCL(zxFDD *fdd, uint8_t *ptr, size_t size) {
    if(size < 9) return false;
    auto buf = (const uint8_t*)ptr;
    if(memcmp(ptr, "SINCLAIR", 8) || int(size) < (9 + 270 * buf[8])) return false;

    fdd->make_trd();
    size = 0;
    for(int i = 0; i < buf[8]; ++i) size += buf[14 * i + 22];
    if(size > 2544) {
        auto s = fdd->get_sec(0, 0, 9);
        // free sec
        s->content[229] = (uint8_t)(size & 0xFF);
        s->content[230] = (uint8_t)(size >> 8);
        fdd->update_crc(s);
    }
    auto d = buf + 9 + 14 * buf[8];
    for(int i = 0; i < buf[8]; ++i) {
        if(!fdd->add_file(buf + 9 + 14 * i, d)) return false;
        d += buf[14 * i + 22] * 256;
    }
    return true;
}

uint8_t* zxFormats::saveSCL(zxFDD *fdd) {
    return nullptr;
}

bool zxFormats::openFDI(zxFDD *fdd, uint8_t *ptr, size_t size) {
    auto buf = (const uint8_t*)ptr;
    auto disk = new zxDisk(buf[4], buf[6]);

    auto offsSecs = *(uint16_t*)(buf + 0x0A);
    auto offsTrks = *(uint16_t*)(buf + 0x0C);
    auto trks = buf + 0x0E + offsTrks;
    auto dat = buf + offsSecs;

    for(int i = 0; i < disk->trks; ++i) {
        for(int j = 0; j < disk->heads; ++j) {
            fdd->seek(i, j);
            auto t = fdd->get_trk(); auto l = t->len; auto d = t->content;
            memset(d, 0, (size_t)l);

            int pos = 0;
            // gap4a sync iam
            fdd->write_blk(pos, 0x4e, 80); fdd->write_blk(pos, 0, 12); fdd->write_blk(pos, 0xc2, 3);
            fdd->write(pos++, 0xfc);

            auto t0 = dat + Dword(trks);
            auto ns = trks[6];
            t->total_sec = ns;
            trks += 7;
            for(int n = 0; n < ns; ++n) {
                // gap1 sync am
                fdd->write_blk(pos, 0x4e, 40); fdd->write_blk(pos, 0, 12); fdd->write_blk(pos, 0xa1, 3); fdd->write(pos++, 0xfe);
                auto sec = fdd->get_sec(n);
                sec->caption = d + pos;
                fdd->write(pos++, trks[0]); fdd->write(pos++, trks[1]); fdd->write(pos++, trks[2]); fdd->write(pos++, trks[3]);
                auto crc = fdd->CRC(d + pos - 5, 5);
                fdd->write(pos++, (uint8_t)(crc >> 8)); fdd->write(pos++, (uint8_t)crc);
                if(trks[4] & 0x40) sec->content = nullptr;
                else {
                    auto data1 = (t0 + wordLE(trks + 5));
                    if(data1 + 128 > buf + size) return false;
                    // gap2 sync am
                    fdd->write_blk(pos, 0x4e, 22); fdd->write_blk(pos, 0, 12); fdd->write_blk(pos, 0xa1, 3); fdd->write(pos++, 0xfb);
                    sec->content = d + pos;
                    auto len = sec->len();
                    memcpy(sec->content, data1, (size_t)len);
                    crc = fdd->CRC(d + pos - 1, len + 1);
                    if(!(trks[4] & (1 << (trks[3] & 3)))) crc ^= 0xffff;
                    pos += len;
                    fdd->write(pos++, (uint8_t)(crc >> 8)); fdd->write(pos++, (uint8_t)crc);
                }
                fdd->trk += 7;
            }
            if(pos > fdd->get_trk()->len) { assert(0); }
            fdd->write_blk(pos, 0x4e, fdd->get_trk()->len - pos - 1); //gap3
        }
    }
    delete fdd->disk;
    fdd->disk = disk;
    return true;
}

uint8_t* zxFormats::saveFDI(zxFDD* fdd) {
/*
    unsigned c,s, total_s = 0;
    for (c = 0; c < cyls; c++)
        for (s = 0; s < sides; s++) {
            t.seek(this, c,s, LOAD_SECTORS);
            total_s += t.s;
        }
    unsigned tlen = strlen(dsc)+1;
    unsigned hsize = 14+(total_s+cyls*sides)*7;
    *(unsigned*)snbuf = WORD4('F','D','I',0);
    *(unsigned short*)(snbuf+4) = cyls;
    *(unsigned short*)(snbuf+6) = sides;
    *(unsigned short*)(snbuf+8) = hsize;
    *(unsigned short*)(snbuf+0x0A) = hsize + tlen;
    *(unsigned short*)(snbuf+0x0C) = 0;
    fwrite(snbuf, 1, 14, ff);
    unsigned trkoffs = 0;
    for (c = 0; c < cyls; c++)
        for (s = 0; s < sides; s++) {
            t.seek(this,c,s,LOAD_SECTORS);
            unsigned secoffs = 0;
            *(unsigned*)snbuf = trkoffs;
            *(unsigned*)(snbuf+4) = 0;
            snbuf[6] = t.s;
            fwrite(snbuf, 1, 7, ff);
            for (unsigned se = 0; se < t.s; se++) {
                *(unsigned*)snbuf = *(unsigned*)&t.hdr[se];
                snbuf[4] = t.hdr[se].c2 ? (1<<t.hdr[se].l) : 0;
                if (t.hdr[se].data && t.hdr[se].data[-1] == 0xF8) snbuf[4] |= 0x80;
                if (!t.hdr[se].data) snbuf[4] |= 0x40;
                *(unsigned*)(snbuf+5) = secoffs;
                fwrite(snbuf, 1, 7, ff);
                secoffs += t.hdr[se].datlen;
            }
            trkoffs += secoffs;
        }
    fseek(ff, hsize, SEEK_SET);
    fwrite(dsc, 1, tlen, ff);
    for (c = 0; c < cyls; c++)
        for (s = 0; s < sides; s++) {
            t.seek(this,c,s,LOAD_SECTORS);
            for (unsigned se = 0; se < t.s; se++)
                if (t.hdr[se].data)
                    if (fwrite(t.hdr[se].data, 1, t.hdr[se].datlen, ff) != t.hdr[se].datlen) return 0;
        }
    return 1;
*/
    return nullptr;
}

bool zxFormats::openUDI(zxFDD *fdd, uint8_t *ptr, size_t size) {
    int c, s;
    delete fdd->disk;
    fdd->disk = new zxDisk(ptr[9] + 1, ptr[10] + 1);
    auto disk = fdd->disk;
    auto buf = ptr + 16;
    for (c = 0; c < disk->trks; c++) {
        for (s = 0; s < disk->heads; s++) {
            if (*buf) return false;
            // only MFM
            auto sz = *(uint16_t *) (buf + 1);
            if(sz < 6250) return false;
            fdd->seek(c, s);
            auto t = fdd->get_trk();
            memcpy(t->content, buf, t->len);
            buf += sz + 3;
            if (buf > ptr + size) return false;
        }
    }
    return true;
}

uint8_t* zxFormats::saveUDI(zxFDD* fdd) {
/*
    auto ptr = TMP_BUF;
    auto dsk = fdd->disk;
    memset(ptr, 0, 16);
    memcpy(ptr, "UDI!", 4);
    ptr[9]  = dsk->trks - 1;
    ptr[10] = dsk->heads - 1;
    *(unsigned*)(snbuf+12) = 0;

    unsigned char *dst = snbuf+0x10;
    for (int c = 0; c < dsk->trks; c++)
        for (int s = 0; s < dsk->heads; s++) {
            *dst++ = 0;
            unsigned len = trklen[c][s];
            *(unsigned short*)dst = len; dst += 2;
            memcpy(dst, trkd[c][s], len); dst += len;
            len = (len+7)/8;
            memcpy(dst, trki[c][s], len); dst += len;
        }
    if (*dsc) strcpy((char*)dst, dsc), dst += strlen(dsc)+1, snbuf[11] = 1;
    *(unsigned*)(snbuf+4) = dst-snbuf;
    int crc = -1; crc32(crc, snbuf, dst-snbuf);
    *(unsigned*)dst = crc; dst += 4;
    if (fwrite(snbuf, 1, dst-snbuf, ff) != (unsigned)(dst-snbuf)) return 0;
    return 1;
*/
    return nullptr;
}

bool zxFormats::openTD0(zxFDD *fdd, uint8_t *ptr, size_t size) {
    return false;
}

uint8_t* zxFormats::saveTD0(zxFDD *fdd) {
    return nullptr;
}
