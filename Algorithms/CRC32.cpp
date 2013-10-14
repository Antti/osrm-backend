/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "CRC32.h"

CRC32::CRC32() : crc(0) {
    crcFunction = detectBestCRC32C();
}

unsigned CRC32::SoftwareBasedCRC32(char *str, unsigned len, unsigned ) {
    boost::crc_optimal<32, 0x1EDC6F41, 0x0, 0x0, true, true> CRC32_Processor;
    CRC32_Processor.process_bytes( str, len);
    return CRC32_Processor.checksum();
}

unsigned CRC32::SSEBasedCRC32( char *str, unsigned len, unsigned crc) {
    unsigned q=len/sizeof(unsigned),
            r=len%sizeof(unsigned),
            *p=(unsigned*)str/*, crc*/;

    //crc=0;
    while (q--) {
        __asm__ __volatile__(
                ".byte 0xf2, 0xf, 0x38, 0xf1, 0xf1;"
                :"=S"(crc)
                 :"0"(crc), "c"(*p)
        );
        ++p;
    }

    str=(char*)p;
    while (r--) {
        __asm__ __volatile__(
                ".byte 0xf2, 0xf, 0x38, 0xf1, 0xf1;"
                :"=S"(crc)
                 :"0"(crc), "c"(*str)
        );
        ++str;
    }
    return crc;
}

CRC32::CRC32CFunctionPtr CRC32::detectBestCRC32C() {
    static const int SSE42_BIT = 20;
    unsigned ecx = cpuid(1);
    bool hasSSE42 = ecx & (1 << SSE42_BIT);
    if (hasSSE42) {
        SimpleLogger().Write() << "using hardware base sse computation";
        return &CRC32::SSEBasedCRC32; //crc32 hardware accelarated;
    } else {
        SimpleLogger().Write() << "using software base sse computation";
        return &CRC32::SoftwareBasedCRC32; //crc32cSlicingBy8;
    }
}

unsigned CRC32::cpuid(unsigned functionInput) {
    unsigned eax;
    unsigned ebx;
    unsigned ecx;
    unsigned edx;
    asm("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (functionInput));
    return ecx;
}

unsigned CRC32::operator()(char *str, unsigned len){
    crc =((*this).*(crcFunction))(str, len, crc);
    return crc;
}
