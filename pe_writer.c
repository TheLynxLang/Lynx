#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

void write_pe(const char* output, uint8_t* code, size_t code_size, int standalone) {
    FILE* f = fopen(output, "wb");
    if (!f) return;

    uint8_t dos[] = {
        0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD,
        0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68,
        0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72,
        0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F,
        0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E,
        0x20, 0x69, 0x6E, 0x20, 0x44, 0x4F, 0x53, 0x20,
        0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A,
        0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    fwrite(dos, 1, sizeof(dos), f);

    uint32_t pe_sig = 0x00004550;
    fwrite(&pe_sig, 1, 4, f);

    typedef struct {
        uint16_t machine;
        uint16_t numberOfSections;
        uint32_t timeDateStamp;
        uint32_t pointerToSymbolTable;
        uint32_t numberOfSymbols;
        uint16_t sizeOfOptionalHeader;
        uint16_t characteristics;
    } IMAGE_FILE_HEADER;

    IMAGE_FILE_HEADER fh = {0};
    fh.machine = 0x8664;
    fh.numberOfSections = 1;
    fh.sizeOfOptionalHeader = 240;
    fh.characteristics = 0x0022;
    fwrite(&fh, 1, sizeof(fh), f);

    typedef struct {
        uint16_t magic;
        uint8_t majorLinkerVersion;
        uint8_t minorLinkerVersion;
        uint32_t sizeOfCode;
        uint32_t sizeOfInitializedData;
        uint32_t sizeOfUninitializedData;
        uint32_t addressOfEntryPoint;
        uint32_t baseOfCode;
        uint64_t imageBase;
        uint32_t sectionAlignment;
        uint32_t fileAlignment;
        uint16_t majorOperatingSystemVersion;
        uint16_t minorOperatingSystemVersion;
        uint16_t majorImageVersion;
        uint16_t minorImageVersion;
        uint16_t majorSubsystemVersion;
        uint16_t minorSubsystemVersion;
        uint32_t win32VersionValue;
        uint32_t sizeOfImage;
        uint32_t sizeOfHeaders;
        uint32_t checkSum;
        uint16_t subsystem;
        uint16_t dllCharacteristics;
        uint64_t sizeOfStackReserve;
        uint64_t sizeOfStackCommit;
        uint64_t sizeOfHeapReserve;
        uint64_t sizeOfHeapCommit;
        uint32_t loaderFlags;
        uint32_t numberOfRvaAndSizes;
    } IMAGE_OPTIONAL_HEADER64;

    IMAGE_OPTIONAL_HEADER64 oh = {0};
    oh.magic = 0x020B;
    oh.majorLinkerVersion = 14;
    oh.minorLinkerVersion = 0;
    oh.sizeOfCode = code_size;
    oh.addressOfEntryPoint = 0x1000;
    oh.baseOfCode = 0x1000;
    oh.imageBase = 0x140000000;
    oh.sectionAlignment = 0x1000;
    oh.fileAlignment = 0x200;
    oh.majorSubsystemVersion = 6;
    oh.minorSubsystemVersion = 0;
    oh.sizeOfImage = 0x2000;
    oh.sizeOfHeaders = 0x400;
    oh.subsystem = 3;
    oh.dllCharacteristics = 0x8160;
    oh.sizeOfStackReserve = 0x100000;
    oh.sizeOfStackCommit = 0x1000;
    oh.sizeOfHeapReserve = 0x100000;
    oh.sizeOfHeapCommit = 0x1000;
    oh.numberOfRvaAndSizes = 16;
    fwrite(&oh, 1, sizeof(oh), f);

    uint8_t section[40] = {0};
    memcpy(section, ".text", 5);
    *(uint32_t*)(section + 8) = code_size;
    *(uint32_t*)(section + 12) = 0x1000;
    *(uint32_t*)(section + 16) = code_size;
    *(uint32_t*)(section + 20) = 0x200;
    *(uint32_t*)(section + 36) = 0x60000020;
    fwrite(section, 1, 40, f);

    fseek(f, 0x200, SEEK_SET);
    fwrite(code, 1, code_size, f);
    fclose(f);
}