#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "debug.h"
#include "box86stack.h"
#include "x86emu.h"
#include "x86run.h"
#include "x86emu_private.h"
#include "x86run_private.h"
#include "x86primop.h"
#include "x86trace.h"
#include "box86context.h"

#define F8      *(uint8_t*)(ip++)
#define F8S     *(int8_t*)(ip++)
#define F16     *(uint16_t*)(ip+=2, ip-2)
#define F16S    *(int16_t*)(ip+=2, ip-2)
#define F32     *(uint32_t*)(ip+=4, ip-4)
#define F32S    *(int32_t*)(ip+=4, ip-4)
#define PK(a)   *(uint8_t*)(ip+a)

#include "modrm.h"

void Run660F(x86emu_t *emu)
{
    uintptr_t ip = R_EIP+2; //skip 66 0F
    uint8_t opcode;
    uint8_t nextop;
    reg32_t *oped;
    uint8_t tmp8u;
    int8_t tmp8s;
    uint16_t tmp16u;
    int16_t tmp16s;
    uint32_t tmp32u;
    int32_t tmp32s;
    sse_regs_t *opex, eax1, *opx2;
    mmx_regs_t *opem;

    static const void* opcodes660f[256] = {
    &&_default, &&_default, &&_default, &&_default, &&_default ,&&_default, &&_default, &&_default, //0x00-0x07
    &&_default, &&_default, &&_default, &&_default, &&_default ,&&_default, &&_default, &&_default, //0x08-0x0F
    &&_6f_0x10, &&_6f_0x11, &&_6f_0x12, &&_6f_0x13, &&_6f_0x14, &&_6f_0x15, &&_6f_0x16, &&_6f_0x17, 
    &&_default, &&_default, &&_default, &&_default, &&_default, &&_default, &&_default, &&_6f_0x1F, 
    &&_default, &&_default, &&_default, &&_default, &&_default ,&&_default, &&_default, &&_default, //0x20-0x27
    &&_6f_0x28, &&_6f_0x29, &&_6f_0x2A, &&_default, &&_6f_0x2C, &&_6f_0x2D, &&_6f_0x2E, &&_6f_0x2F, 
    &&_default, &&_default, &&_default, &&_default, &&_default ,&&_default, &&_default, &&_default, //0x30-0x37
    &&_6f_0x38, &&_default, &&_6f_0x3A, &&_default, &&_default ,&&_default, &&_default, &&_default, //0x38-0x3F
    &&_6f_0x40_0, &&_6f_0x40_1, &&_6f_0x40_2, &&_6f_0x40_3, &&_6f_0x40_4, &&_6f_0x40_5, &&_6f_0x40_6, &&_6f_0x40_7, 
    &&_6f_0x40_8, &&_6f_0x40_9, &&_6f_0x40_A, &&_6f_0x40_B, &&_6f_0x40_C, &&_6f_0x40_D, &&_6f_0x40_E, &&_6f_0x40_F, 
    &&_6f_0x50, &&_6f_0x51, &&_default, &&_default, &&_6f_0x54, &&_6f_0x55, &&_6f_0x56, &&_6f_0x57, 
    &&_6f_0x58, &&_6f_0x59, &&_6f_0x5A, &&_6f_0x5B, &&_6f_0x5C, &&_6f_0x5D, &&_6f_0x5E, &&_6f_0x5F, 
    &&_6f_0x60, &&_6f_0x61, &&_6f_0x62, &&_6f_0x63, &&_6f_0x64, &&_6f_0x65, &&_6f_0x66, &&_6f_0x67, 
    &&_6f_0x68, &&_6f_0x69, &&_6f_0x6A, &&_6f_0x6B, &&_6f_0x6C, &&_6f_0x6D, &&_6f_0x6E, &&_6f_0x6F,     
    &&_6f_0x70, &&_6f_0x71, &&_6f_0x72, &&_6f_0x73, &&_6f_0x74, &&_6f_0x75, &&_6f_0x76, &&_default, 
    &&_default, &&_default, &&_default, &&_default, &&_6f_0x7C, &&_default, &&_6f_0x7E, &&_6f_0x7F, //0x78-0x7F
    &&_default, &&_default, &&_default, &&_default, &&_default ,&&_default, &&_default, &&_default, //0x80-0x87
    &&_default, &&_default, &&_default, &&_default, &&_default ,&&_default, &&_default, &&_default, //0x88-0x8F
    &&_default, &&_default, &&_default, &&_default, &&_default ,&&_default, &&_default, &&_default, //0x90-0x97
    &&_default, &&_default, &&_default, &&_default, &&_default ,&&_default, &&_default, &&_default, //0x98-0x9F
    &&_default, &&_default, &&_default, &&_6f_0xA3, &&_6f_0xA4, &&_6f_0xA5, &&_default, &&_default, 
    &&_default, &&_default, &&_default, &&_6f_0xAB, &&_6f_0xAC, &&_6f_0xAD, &&_default, &&_6f_0xAF, 
    &&_default, &&_6f_0xB1, &&_default, &&_6f_0xB3, &&_default, &&_default, &&_6f_0xB6, &&_6f_0xB7, 
    &&_default, &&_default, &&_6f_0xBA, &&_6f_0xBB, &&_6f_0xBC, &&_6f_0xBD, &&_6f_0xBE, &&_default, //0xB8-0xBF
    &&_default, &&_6f_0xC1, &&_6f_0xC2, &&_default, &&_6f_0xC4, &&_6f_0xC5, &&_6f_0xC6, &&_default, 
    &&_6f_0xC8, &&_6f_0xC9, &&_6f_0xCA, &&_6f_0xCB, &&_6f_0xCC ,&&_6f_0xCD, &&_6f_0xCE, &&_6f_0xCF, //0xC8-0xCF
    &&_default, &&_6f_0xD1, &&_6f_0xD2, &&_6f_0xD3, &&_6f_0xD4, &&_6f_0xD5, &&_6f_0xD6, &&_6f_0xD7, 
    &&_6f_0xD8, &&_6f_0xD9, &&_6f_0xDA, &&_6f_0xDB, &&_6f_0xDC, &&_6f_0xDD, &&_6f_0xDE, &&_6f_0xDF, 
    &&_6f_0xE0, &&_6f_0xE1, &&_6f_0xE2, &&_6f_0xE3, &&_6f_0xE4, &&_6f_0xE5, &&_6f_0xE6, &&_6f_0xE7, 
    &&_6f_0xE8, &&_6f_0xE9, &&_6f_0xEA, &&_6f_0xEB, &&_6f_0xEC, &&_6f_0xED, &&_6f_0xEE, &&_6f_0xEF, 
    &&_default, &&_6f_0xF1, &&_6f_0xF2, &&_6f_0xF3, &&_6f_0xF4, &&_6f_0xF5, &&_6f_0xF6, &&_6f_0xF7, 
    &&_6f_0xF8, &&_6f_0xF9, &&_6f_0xFA, &&_6f_0xFB, &&_6f_0xFC, &&_6f_0xFD, &&_6f_0xFE, &&_default
    };

    #define NEXT    goto _fini

    opcode = F8;
    goto *opcodes660f[opcode];

    _default:
        emu->old_ip = R_EIP;
        R_EIP = ip;
        UnimpOpcode(emu);
        goto _fini;

    #define GOCOND(BASE, PREFIX, CONDITIONAL) \
    _6f_##BASE##_0:                          \
        PREFIX                              \
        if(ACCESS_FLAG(F_OF))               \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_1:                          \
        PREFIX                              \
        if(!ACCESS_FLAG(F_OF))              \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_2:                          \
        PREFIX                              \
        if(ACCESS_FLAG(F_CF))               \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_3:                          \
        PREFIX                              \
        if(!ACCESS_FLAG(F_CF))              \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_4:                          \
        PREFIX                              \
        if(ACCESS_FLAG(F_ZF))               \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_5:                          \
        PREFIX                              \
        if(!ACCESS_FLAG(F_ZF))              \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_6:                          \
        PREFIX                              \
        if((ACCESS_FLAG(F_ZF) || ACCESS_FLAG(F_CF)))  \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_7:                          \
        PREFIX                              \
        if(!(ACCESS_FLAG(F_ZF) || ACCESS_FLAG(F_CF))) \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_8:                          \
        PREFIX                              \
        if(ACCESS_FLAG(F_SF))               \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_9:                          \
        PREFIX                              \
        if(!ACCESS_FLAG(F_SF))              \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_A:                          \
        PREFIX                              \
        if(ACCESS_FLAG(F_PF))               \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_B:                          \
        PREFIX                              \
        if(!ACCESS_FLAG(F_PF))              \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_C:                          \
        PREFIX                              \
        if(ACCESS_FLAG(F_SF) != ACCESS_FLAG(F_OF))  \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_D:                          \
        PREFIX                              \
        if(ACCESS_FLAG(F_SF) == ACCESS_FLAG(F_OF)) \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_E:                          \
        PREFIX                              \
        if(ACCESS_FLAG(F_ZF) || (ACCESS_FLAG(F_SF) != ACCESS_FLAG(F_OF))) \
            CONDITIONAL                     \
        NEXT;                              \
    _6f_##BASE##_F:                          \
        PREFIX                              \
        if(!ACCESS_FLAG(F_ZF) && (ACCESS_FLAG(F_SF) == ACCESS_FLAG(F_OF))) \
            CONDITIONAL                     \
        NEXT;

    GOCOND(0x40
        , nextop = F8;
        CHECK_FLAGS(emu);
        GET_EW;
        , GW.word[0] = EW->word[0];
    )                               /* 0x40 -> 0x4F CMOVxx Gw,Ew */ // conditional move, no sign
    #undef GOCOND
        
    _6f_0x10:                      /* MOVUPD Gx, Ex */
        nextop = F8;
        GET_EX;
        memcpy(&GX, EX, 16); // unaligned...
        NEXT;
    _6f_0x11:                      /* MOVUPD Ex, Gx */
        nextop = F8;
        GET_EX;
        memcpy(EX, &GX, 16); // unaligned...
        NEXT;
    _6f_0x12:                      /* MOVLPD Gx, Eq */
        nextop = F8;
        GET_ED;
        GX.q[0] = *(uint64_t*)ED;
        NEXT;
    _6f_0x13:                      /* MOVLPD Eq, Gx */
        nextop = F8;
        GET_ED;
        if((uintptr_t)ED & 7)
            memcpy(ED, &GX.q[0], 8);
        else
            *(uint64_t*)ED = GX.q[0];
        NEXT;
    _6f_0x14:                      /* UNPCKLPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.q[1] = EX->q[0];
        NEXT;
    _6f_0x15:                      /* UNPCKHPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] = GX.q[1];
        GX.q[1] = EX->q[1];
        NEXT;
    _6f_0x16:                      /* MOVHPD Gx, Ed */
        nextop = F8;
        GET_ED;
        GX.q[1] = *(uint64_t*)ED;
        NEXT;
    _6f_0x17:                      /* MOVHPD Ed, Gx */
        nextop = F8;
        GET_ED;
        *(uint64_t*)ED = GX.q[1];
        NEXT;

    _6f_0x1F:                      /* NOP (multi-byte) */
        nextop = F8;
        GET_ED;
        NEXT;
    
    _6f_0x28:                      /* MOVAPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] = EX->q[0];
        GX.q[1] = EX->q[1];
        NEXT;
    _6f_0x29:                      /* MOVAPD Ex, Gx */
        nextop = F8;
        GET_EX;
        EX->q[0] = GX.q[0];
        EX->q[1] = GX.q[1];
        NEXT;
    _6f_0x2A:                      /* CVTPI2PD Gx, Em */
        nextop = F8;
        GET_EM;
        GX.d[0] = EM->sd[0];
        GX.d[1] = EM->sd[1];
        NEXT;


    _6f_0x2C:                      /* CVTTPD2PI Gm, Ex */
        nextop = F8;
        GET_EX;
        GM.sd[0] = EX->d[0];
        GM.sd[1] = EX->d[1];
        NEXT;
    _6f_0x2D:                      /* CVTPD2PI Gm, Ex */
        nextop = F8;
        GET_EX;
        switch(emu->round) {
            case ROUND_Nearest:
                GM.sd[0] = floor(EX->d[0]+0.5);
                GM.sd[1] = floor(EX->d[1]+0.5);
                break;
            case ROUND_Down:
                GM.sd[0] = floor(EX->d[0]);
                GM.sd[1] = floor(EX->d[1]);
                break;
            case ROUND_Up:
                GM.sd[0] = ceil(EX->d[0]);
                GM.sd[1] = ceil(EX->d[1]);
                break;
            case ROUND_Chop:
                GM.sd[0] = EX->d[0];
                GM.sd[1] = EX->d[1];
                break;
        }
        NEXT;
    _6f_0x2E:                      /* UCOMISD Gx, Ex */
        // no special check...
    _6f_0x2F:                      /* COMISD Gx, Ex */
        RESET_FLAGS(emu);
        nextop = F8;
        GET_EX;
        if(isnan(GX.d[0]) || isnan(EX->d[0])) {
            SET_FLAG(F_ZF); SET_FLAG(F_PF); SET_FLAG(F_CF);
        } else if(isgreater(GX.d[0], EX->d[0])) {
            CLEAR_FLAG(F_ZF); CLEAR_FLAG(F_PF); CLEAR_FLAG(F_CF);
        } else if(isless(GX.d[0], EX->d[0])) {
            CLEAR_FLAG(F_ZF); CLEAR_FLAG(F_PF); SET_FLAG(F_CF);
        } else {
            SET_FLAG(F_ZF); CLEAR_FLAG(F_PF); CLEAR_FLAG(F_CF);
        }
        CLEAR_FLAG(F_OF); CLEAR_FLAG(F_AF); CLEAR_FLAG(F_SF);
        NEXT;

    _6f_0x38:  // these are some SSE3 opcodes
        opcode = F8;
        switch(opcode) {
            case 0x00:  /* PSHUFB */
                nextop = F8;
                GET_EX;
                eax1 = GX;
                for (int i=0; i<16; ++i) {
                    if(EX->ub[i]&128)
                        GX.ub[i] = 0;
                    else
                        GX.ub[i] = eax1.ub[EX->ub[i]&15];
                }
                break;
            case 0x01:  /* PHADDW Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<4; ++i)
                    GX.sw[i] = GX.sw[i*2+0]+GX.sw[i*2+1];
                if(&GX == EX) {
                    GX.q[1] = GX.q[0];
                } else {
                    for (int i=0; i<4; ++i)
                        GX.sw[4+i] = EX->sw[i*2+0] + EX->sw[i*2+1];
                }
                break;
            case 0x02:  /* PHADDD Gx, Ex */
                nextop = F8;
                GET_EX;
                GX.sd[0] += GX.sd[1];
                GX.sd[1] = GX.sd[2] + GX.sd[3];
                if(&GX == EX) {
                    GX.q[1] = GX.q[0];
                } else {
                    GX.sd[2] = EX->sd[0] + EX->sd[1];
                    GX.sd[3] = EX->sd[2] + EX->sd[3];
                }
                break;
            case 0x03:  /* PHADDSW Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<4; ++i) {
                    tmp32s = GX.sw[i*2+0]+GX.sw[i*2+1];
                    GX.sw[i] = (tmp32s<-32768)?-32768:((tmp32s>32767)?32767:tmp32s);
                }
                if(&GX == EX) {
                    GX.q[1] = GX.q[0];
                } else {
                    for (int i=0; i<4; ++i) {
                        tmp32s = EX->sw[i*2+0] + EX->sw[i*2+1];
                        GX.sw[4+i] = (tmp32s<-32768)?-32768:((tmp32s>32767)?32767:tmp32s);
                    }
                }
                break;
            case 0x04:  /* PMADDUBSW Gx,Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<8; ++i) {
                    tmp32s = (int32_t)(GX.ub[i*2+0])*EX->sb[i*2+0] + (int32_t)(GX.ub[i*2+1])*EX->sb[i*2+1];
                    GX.sw[i] = (tmp32s>32767)?32767:((tmp32s<-32768)?-32768:tmp32s);
                }
                break;
            case 0x05:  /* PHSUBW Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<4; ++i)
                    GX.sw[i] = GX.sw[i*2+0] - GX.sw[i*2+1];
                if(&GX == EX) {
                    GX.q[1] = GX.q[0];
                } else {
                    for (int i=0; i<4; ++i)
                        GX.sw[4+i] = EX->sw[i*2+0] - EX->sw[i*2+1];
                }
                break;
                
            case 0x08:  /* PSIGNB Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<16; ++i)
                    GX.sb[i] *= (EX->sb[i]<0)?-1:((EX->sb[i]>0)?1:0);
            break;
            case 0x09:  /* PSIGNW Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<8; ++i)
                    GX.sw[i] *= (EX->sw[i]<0)?-1:((EX->sw[i]>0)?1:0);
            break;
            case 0x0A:  /* PSIGND Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<4; ++i)
                    GX.sd[i] *= (EX->sd[i]<0)?-1:((EX->sd[i]>0)?1:0);
            break;
            case 0x0B:  /* PMULHRSW Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<8; ++i) {
                    tmp32s = ((((int32_t)(GX.sw[i])*(int32_t)(EX->sw[i]))>>14) + 1)>>1;
                    GX.uw[i] = tmp32s&0xffff;
                }
            break;

            case 0x1C:  /* PABSB Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<16; ++i) {
                    GX.sb[i] = abs(EX->sb[i]);
                }
                break;
            case 0x1D:  /* PABSW Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<8; ++i) {
                    GX.sw[i] = abs(EX->sw[i]);
                }
                break;
            case 0x1E:  /* PABSD Gx, Ex */
                nextop = F8;
                GET_EX;
                for (int i=0; i<4; ++i) {
                    GX.sd[i] = abs(EX->sd[i]);
                }
                break;

            default:
                goto _default;
        }
        NEXT;

    _6f_0x3A:  // these are some SSE3 opcodes
        opcode = F8;
        switch(opcode) {
            case 0x0F:          // PALIGNR GX, EX, u8
                nextop = F8;
                GET_EX;
                tmp8u = F8;
                if(tmp8u>31)
                    {GX.q[0] = GX.q[1] = 0;}
                else
                #if 0
                    if(tmp8u>15) {
                    tmp8u=(tmp8u-16)*8;
                    if (tmp8u < 64) {
                        GX.q[0] = (GX.q[0] >> tmp8u) | (GX.q[1] << (64 - tmp8u));
                        GX.q[1] = (GX.q[1] >> tmp8u);
                    } else {
                        GX.q[0] = GX.q[1] >> (tmp8u - 64);
                        GX.q[1] = 0;
                    }                    
                } else {
                    tmp8u*=8;
                    if (tmp8u < 64) {
                        GX.q[0] = (EX->q[0] >> tmp8u) | (EX->q[1] << (64 - tmp8u));
                        GX.q[1] = (EX->q[1] >> tmp8u) | (GX.q[0] << (64-tmp8u));
                    } else {
                        tmp8u -= 64;
                        GX.q[0] = (EX->q[1] >> tmp8u) | (GX.q[0] << (64 - tmp8u));
                        GX.q[1] = (GX.q[0] >> tmp8u) | (GX.q[1] >> (64 - tmp8u));
                    }                    
                }
                #else
                {
                    for (int i=0; i<16; ++i, ++tmp8u)
                        eax1.ub[i] = (tmp8u>15)?((tmp8u>31)?0:GX.ub[tmp8u-16]):EX->ub[tmp8u];
                    GX.q[0] = eax1.q[0];
                    GX.q[1] = eax1.q[1];
                }
                #endif
                break;
            default:
                goto _default;
        }
        NEXT;

    _6f_0x50:                      /* MOVMSKPD Gd, Ex */
        nextop = F8;
        GET_EX;
        GD.dword[0] = 0;
        for(int i=0; i<2; ++i)
            GD.dword[0] |= ((EX->q[i]>>63)&1)<<i;
        NEXT;
    _6f_0x51:                      /* SQRTPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.d[0] = sqrt(EX->d[0]);
        GX.d[1] = sqrt(EX->d[1]);
        NEXT;

    _6f_0x54:                      /* ANDPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] &= EX->q[0];
        GX.q[1] &= EX->q[1];
        NEXT;
    _6f_0x55:                      /* ANDNPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] = (~GX.q[0]) & EX->q[0];
        GX.q[1] = (~GX.q[1]) & EX->q[1];
        NEXT;
    _6f_0x56:                      /* ORPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] |= EX->q[0];
        GX.q[1] |= EX->q[1];
        NEXT;
    _6f_0x57:                      /* XORPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] ^= EX->q[0];
        GX.q[1] ^= EX->q[1];
        NEXT;
    _6f_0x58:                      /* ADDPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.d[0] += EX->d[0];
        GX.d[1] += EX->d[1];
        NEXT;
    _6f_0x59:                      /* MULPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.d[0] *= EX->d[0];
        GX.d[1] *= EX->d[1];
        NEXT;
    _6f_0x5A:                      /* CVTPD2PS Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.f[0] = EX->d[0];
        GX.f[1] = EX->d[1];
        GX.q[1] = 0;
        NEXT;
    _6f_0x5B:                      /* CVTPS2DQ Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.sd[0] = EX->f[0];
        GX.sd[1] = EX->f[1];
        GX.sd[2] = EX->f[2];
        GX.sd[3] = EX->f[3];
        NEXT;
    _6f_0x5C:                      /* SUBPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.d[0] -= EX->d[0];
        GX.d[1] -= EX->d[1];
        NEXT;
    _6f_0x5D:                      /* MINPD Gx, Ex */
        nextop = F8;
        GET_EX;
        if (isnan(GX.d[0]) || isnan(EX->d[0]) || isless(EX->d[0], GX.d[0]))
            GX.d[0] = EX->d[0];
        if (isnan(GX.d[1]) || isnan(EX->d[1]) || isless(EX->d[1], GX.d[1]))
            GX.d[1] = EX->d[1];
        NEXT;
    _6f_0x5E:                      /* DIVPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.d[0] /= EX->d[0];
        GX.d[1] /= EX->d[1];
        NEXT;
    _6f_0x5F:                      /* MAXPD Gx, Ex */
        nextop = F8;
        GET_EX;
        if (isnan(GX.d[0]) || isnan(EX->d[0]) || isgreater(EX->d[0], GX.d[0]))
            GX.d[0] = EX->d[0];
        if (isnan(GX.d[1]) || isnan(EX->d[1]) || isgreater(EX->d[1], GX.d[1]))
            GX.d[1] = EX->d[1];
        NEXT;

    _6f_0x60:  /* PUNPCKLBW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=7; i>0; --i)  // 0 is untouched
            GX.ub[2 * i] = GX.ub[i];
        if(&GX==EX)
            for(int i=0; i<8; ++i)
                GX.ub[2 * i + 1] = GX.ub[2 * i];
        else 
            for(int i=0; i<8; ++i)
                GX.ub[2 * i + 1] = EX->ub[i];
        NEXT;
    _6f_0x61:  /* PUNPCKLWD Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=3; i>0; --i)
            GX.uw[2 * i] = GX.uw[i];
        if(&GX==EX)
            for(int i=0; i<4; ++i)
                GX.uw[2 * i + 1] = GX.uw[2 * i];
        else
            for(int i=0; i<4; ++i)
                GX.uw[2 * i + 1] = EX->uw[i];
        NEXT;
    _6f_0x62:  /* PUNPCKLDQ Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.ud[3] = EX->ud[1];
        GX.ud[2] = GX.ud[1];
        GX.ud[1] = EX->ud[0];
        NEXT;
    _6f_0x63:  /* PACKSSWB Gx,Ex */
        nextop = F8;
        GET_EX;
        if(&GX==EX) {
            for(int i=0; i<8; ++i)
                GX.sb[i] = (EX->sw[i]<-128)?-128:((EX->sw[i]>127)?127:EX->sw[i]);
            GX.q[1] = GX.q[0];
        } else {
            for(int i=0; i<8; ++i)
                GX.sb[i] = (GX.sw[i]<-128)?-128:((GX.sw[i]>127)?127:GX.sw[i]);
            for(int i=0; i<8; ++i)
                GX.sb[8+i] = (EX->sw[i]<-128)?-128:((EX->sw[i]>127)?127:EX->sw[i]);
        }
        NEXT;
    _6f_0x64:  /* PCMPGTB Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<16; ++i)
            GX.ub[i] = (GX.sb[i]>EX->sb[i])?0xFF:0x00;
        NEXT;
    _6f_0x65:  /* PCMPGTW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i)
            GX.uw[i] = (GX.sw[i]>EX->sw[i])?0xFFFF:0x0000;
        NEXT;
    _6f_0x66:  /* PCMPGTD Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<4; ++i)
            GX.ud[i] = (GX.sd[i]>EX->sd[i])?0xFFFFFFFF:0x00000000;
        NEXT;
    _6f_0x67:  /* PACKUSWB Gx,Ex */
        nextop = F8;
        GET_EX;
        if(&GX==EX) {
            for(int i=0; i<8; ++i)
                GX.ub[i] = (EX->sw[i]<0)?0:((EX->sw[i]>0xff)?0xff:EX->sw[i]);
            GX.q[1] = GX.q[0];
        } else {
            for(int i=0; i<8; ++i)
                GX.ub[i] = (GX.sw[i]<0)?0:((GX.sw[i]>0xff)?0xff:GX.sw[i]);
            for(int i=0; i<8; ++i)
                GX.ub[8+i] = (EX->sw[i]<0)?0:((EX->sw[i]>0xff)?0xff:EX->sw[i]);
        }
        NEXT;
    _6f_0x68:  /* PUNPCKHBW Gx,Ex */
        nextop = F8;
        GET_EX;
        if(EX==&GX) {eax1 = GX; EX = &eax1;}   // copy is needed
        for(int i=0; i<8; ++i)
            GX.ub[2 * i] = GX.ub[i + 8];
        for(int i=0; i<8; ++i)
            GX.ub[2 * i + 1] = EX->ub[i + 8];
        NEXT;
    _6f_0x69:  /* PUNPCKHWD Gx,Ex */
        nextop = F8;
        GET_EX;
        if(EX==&GX) {eax1 = GX; EX = &eax1;}   // copy is needed
        for(int i=0; i<4; ++i)
            GX.uw[2 * i] = GX.uw[i + 4];
        for(int i=0; i<4; ++i)
            GX.uw[2 * i + 1] = EX->uw[i + 4];
        NEXT;
    _6f_0x6A:  /* PUNPCKHDQ Gx,Ex */
        nextop = F8;
        GET_EX;
        // no copy needed if &GX==EX
        GX.ud[0] = GX.ud[2];
        GX.ud[1] = EX->ud[2];
        GX.ud[2] = GX.ud[3];
        GX.ud[3] = EX->ud[3];
        NEXT;
    _6f_0x6B:  /* PACKSSDW Gx,Ex */
        nextop = F8;
        GET_EX;
        if(&GX==EX) {
            for(int i=0; i<4; ++i)
                GX.sw[i] = (EX->sd[i]<-32768)?-32768:((EX->sd[i]>32767)?32767:EX->sd[i]);
            GX.q[1] = GX.q[0];
        } else {
            for(int i=0; i<4; ++i)
                GX.sw[i] = (GX.sd[i]<-32768)?-32768:((GX.sd[i]>32767)?32767:GX.sd[i]);
            for(int i=0; i<4; ++i)
                GX.sw[4+i] = (EX->sd[i]<-32768)?-32768:((EX->sd[i]>32767)?32767:EX->sd[i]);
        }
        NEXT;
    _6f_0x6C:  /* PUNPCKLQDQ Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.q[1] = EX->q[0];
        NEXT;
    _6f_0x6D:  /* PUNPCKHQDQ Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] = GX.q[1];
        GX.q[1] = EX->q[1];
        NEXT;
    _6f_0x6E:  /* MOVD Gx, Ed */
        nextop = F8;
        GET_ED;
        GX.q[0] = ED->dword[0]; // zero extend, so ud[1] <- 0
        GX.q[1] = 0;
        NEXT;
    _6f_0x6F:  /* MOVDQA Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] = EX->q[0];
        GX.q[1] = EX->q[1];
        NEXT;
    _6f_0x70:  /* PSHUFD Gx,Ex,Ib */
        nextop = F8;
        GET_EX;
        tmp8u = F8;
        if(EX==&GX) {eax1 = GX; EX = &eax1;}   // copy is needed
        for (int i=0; i<4; ++i)
            GX.ud[i] = EX->ud[(tmp8u>>(i*2))&3];
        NEXT;
    _6f_0x71:  /* GRP */
        nextop = F8;
        GET_EX;
        switch((nextop>>3)&7) {
            case 2:                 /* PSRLW Ex, Ib */
                tmp8u = F8;
                if(tmp8u>15)
                    {EX->q[0] = EX->q[1] = 0;}
                else
                    for (int i=0; i<8; ++i) EX->uw[i] >>= tmp8u;
                break;
            case 4:                 /* PSRAW Ex, Ib */
                tmp8u = F8;
                for (int i=0; i<8; ++i) EX->sw[i] >>= tmp8u;
                break;
            case 6:                 /* PSLLW Ex, Ib */
                tmp8u = F8;
                if(tmp8u>15)
                    {EX->q[0] = EX->q[1] = 0;}
                else
                    for (int i=0; i<8; ++i) EX->uw[i] <<= tmp8u;
                break;
            default:
                goto _default;
        }
        NEXT;
    _6f_0x72:  /* GRP */
        nextop = F8;
        GET_EX;
        switch((nextop>>3)&7) {
            case 2:                 /* PSRLD Ex, Ib */
                tmp8u = F8;
                if(tmp8u>31)
                    {EX->q[0] = EX->q[1] = 0;}
                else
                    for (int i=0; i<4; ++i) EX->ud[i] >>= tmp8u;
                break;
            case 4:                 /* PSRAD Ex, Ib */
                tmp8u = F8;
                for (int i=0; i<4; ++i) EX->sd[i] >>= tmp8u;
                break;
            case 6:                 /* PSLLD Ex, Ib */
                tmp8u = F8;
                if(tmp8u>31)
                    {EX->q[0] = EX->q[1] = 0;}
                else
                    for (int i=0; i<4; ++i) EX->ud[i] <<= tmp8u;
                break;
            default:
                goto _default;
        }
        NEXT;
    _6f_0x73:  /* GRP */
        nextop = F8;
        GET_EX;
        switch((nextop>>3)&7) {
            case 2:                 /* PSRLQ Ex, Ib */
                tmp8u = F8;
                if(tmp8u>63)
                    {EX->q[0] = EX->q[1] = 0;}
                else
                    {EX->q[0] >>= tmp8u; EX->q[1] >>= tmp8u;}
                break;
            case 3:                 /* PSRLDQ Ex, Ib */
                tmp8u = F8;
                if(tmp8u>15)
                    {EX->q[0] = EX->q[1] = 0;}
                else {
                    tmp8u*=8;
                    if (tmp8u < 64) {
                        EX->q[0] = (EX->q[0] >> tmp8u) | (EX->q[1] << (64 - tmp8u));
                        EX->q[1] = (EX->q[1] >> tmp8u);
                    } else {
                        EX->q[0] = EX->q[1] >> (tmp8u - 64);
                        EX->q[1] = 0;
                    }                    
                }
                break;
            case 6:                 /* PSLLQ Ex, Ib */
                tmp8u = F8;
                if(tmp8u>63)
                    {EX->q[0] = EX->q[1] = 0;}
                else
                    {EX->q[0] <<= tmp8u; EX->q[1] <<= tmp8u;}
                break;
            case 7:                 /* PSLLDQ Ex, Ib */
                tmp8u = F8;
                if(tmp8u>15)
                    {EX->q[0] = EX->q[1] = 0;}
                else {
                    tmp8u*=8;
                    if (tmp8u < 64) {
                        EX->q[1] = (EX->q[1] << tmp8u) | (EX->q[0] >> (64 - tmp8u));
                        EX->q[0] = (EX->q[0] << tmp8u);
                    } else {
                        EX->q[1] = EX->q[0] << (tmp8u - 64);
                        EX->q[0] = 0;
                    }
                }
                break;
            default:
                goto _default;
        }
        NEXT;
    _6f_0x74:  /* PCMPEQB Gx,Ex */
        nextop = F8;
        GET_EX;
        for (int i=0; i<16; ++i)
            GX.ub[i] = (GX.ub[i]==EX->ub[i])?0xff:0;
        NEXT;
    _6f_0x75:  /* PCMPEQW Gx,Ex */
        nextop = F8;
        GET_EX;
        for (int i=0; i<8; ++i)
            GX.uw[i] = (GX.uw[i]==EX->uw[i])?0xffff:0;
        NEXT;
    _6f_0x76:  /* PCMPEQD Gx,Ex */
        nextop = F8;
        GET_EX;
        for (int i=0; i<4; ++i)
            GX.ud[i] = (GX.ud[i]==EX->ud[i])?0xffffffff:0;
        NEXT;

    _6f_0x7C:  /* HADDPD Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.d[0] += GX.d[1];
        if(EX==&GX) {
            GX.d[1] = GX.d[0];
        } else {
            GX.d[1] = EX->d[0] + EX->d[1];
        }
        NEXT;

    _6f_0x7E:  /* MOVD Ed, Gx */
        nextop = F8;
        GET_ED;
        ED->dword[0] = GX.ud[0];
        NEXT;
    _6f_0x7F:  /* MOVDQA Ex,Gx */
        nextop = F8;
        GET_EX;
        EX->q[0] = GX.q[0];
        EX->q[1] = GX.q[1];
        NEXT;

    _6f_0xA3:                      /* BT Ew,Gw */
        CHECK_FLAGS(emu);
        nextop = F8;
        GET_EW;
        if(EW->word[0] & (1<<(GW.word[0]&15)))
            SET_FLAG(F_CF);
        else
            CLEAR_FLAG(F_CF);
        NEXT;
    _6f_0xA4:                      /* SHLD Ew,Gw,Ib */
    _6f_0xA5:                      /* SHLD Ew,Gw,CL */
        nextop = F8;
        GET_EW;
        if(opcode==0xA4)
            tmp8u = F8;
        else
            tmp8u = R_CL;
        EW->word[0] = shld16(emu, EW->word[0], GW.word[0], tmp8u);
        NEXT;

    _6f_0xAB:                      /* BTS Ew,Gw */
        CHECK_FLAGS(emu);
        nextop = F8;
        GET_EW;
        if(EW->word[0] & (1<<(GW.word[0]&15)))
            SET_FLAG(F_CF);
        else {
            EW->word[0] |= (1<<(GW.word[0]&15));
            CLEAR_FLAG(F_CF);
        }
        NEXT;
    _6f_0xAC:                      /* SHRD Ew,Gw,Ib */
    _6f_0xAD:                      /* SHRD Ew,Gw,CL */
        nextop = F8;
        GET_EW;
        if(opcode==0xAC)
            tmp8u = F8;
        else
            tmp8u = R_CL;
        EW->word[0] = shrd16(emu, EW->word[0], GW.word[0], tmp8u);
        NEXT;

    _6f_0xAF:                      /* IMUL Gw,Ew */
        nextop = F8;
        GET_EW;
        GW.word[0] = imul16(emu, GW.word[0], EW->word[0]);
        NEXT;

    _6f_0xB1:                      /* CMPXCHG Ew,Gw */
        nextop = F8;
        GET_EW;
        cmp16(emu, R_AX, EW->word[0]);
        if(ACCESS_FLAG(F_ZF)) {
            EW->word[0] = GW.word[0];
        } else {
            R_AX = EW->word[0];
        }
        NEXT;

    _6f_0xB3:                      /* BTR Ew,Gw */
        CHECK_FLAGS(emu);
        nextop = F8;
        GET_EW;
        if(EW->word[0] & (1<<(GW.word[0]&15))) {
            SET_FLAG(F_CF);
            EW->word[0] ^= (1<<(GW.word[0]&15));
        } else
            CLEAR_FLAG(F_CF);
        NEXT;

    _6f_0xB6:                      /* MOVZX Gw,Eb */
        nextop = F8;
        GET_EB;
        GW.word[0] = EB->byte[0];
        NEXT;
    _6f_0xB7:                      /* MOVZX Gw,Ew */
        nextop = F8;
        GET_EW;
        GW.word[0] = EW->word[0];   // yeah, the ZX is useless here
        NEXT;

    _6f_0xBA:                      
        nextop = F8;
        switch((nextop>>3)&7) {
            case 4:                 /* BT Ew,Ib */
                CHECK_FLAGS(emu);
                GET_EW;
                tmp8u = F8;
                if((nextop&0xC0)!=0xC0)
                {
                    EW=(reg32_t*)(((uint16_t*)(EW))+(tmp8u>>4));
                }
                tmp8u&=15;
                if(EW->word[0] & (1<<tmp8u))
                    SET_FLAG(F_CF);
                else
                    CLEAR_FLAG(F_CF);
                break;
            case 5:             /* BTS Ew, Ib */
                CHECK_FLAGS(emu);
                GET_EW;
                tmp8u = F8;
                if((nextop&0xC0)!=0xC0)
                {
                    EW=(reg32_t*)(((uint16_t*)(EW))+(tmp8u>>4));
                }
                tmp8u&=15;
                if(EW->word[0] & (1<<tmp8u)) {
                    SET_FLAG(F_CF);
                } else
                    EW->word[0] ^= (1<<tmp8u);
                    CLEAR_FLAG(F_CF);
                break;
            case 6:             /* BTR Ew, Ib */
                CHECK_FLAGS(emu);
                GET_EW;
                tmp8u = F8;
                if((nextop&0xC0)!=0xC0)
                {
                    EW=(reg32_t*)(((uint16_t*)(ED))+(tmp8u>>4));
                }
                tmp8u&=15;
                if(EW->word[0] & (1<<tmp8u)) {
                    SET_FLAG(F_CF);
                    EW->word[0] ^= (1<<tmp8u);
                } else
                    CLEAR_FLAG(F_CF);
                break;
            case 7:             /* BTC Ew, Ib */
                CHECK_FLAGS(emu);
                GET_EW;
                tmp8u = F8;
                if((nextop&0xC0)!=0xC0)
                {
                    EW=(reg32_t*)(((uint16_t*)(EW))+(tmp8u>>4));
                }
                tmp8u&=15;
                if(EW->word[0] & (1<<tmp8u))
                    SET_FLAG(F_CF);
                else
                    CLEAR_FLAG(F_CF);
                EW->word[0] ^= (1<<tmp8u);
                break;

            default:
                goto _default;
        }
        NEXT;
    _6f_0xBB:                      /* BTC Ew,Gw */
        CHECK_FLAGS(emu);
        nextop = F8;
        GET_EW;
        if(EW->word[0] & (1<<(GW.word[0]&15)))
            SET_FLAG(F_CF);
        else
            CLEAR_FLAG(F_CF);
        EW->word[0] ^= (1<<(GW.word[0]&15));
        NEXT;
    _6f_0xBC:                      /* BSF Ew,Gw */
        CHECK_FLAGS(emu);
        nextop = F8;
        GET_EW;
        tmp16u = EW->word[0];
        if(tmp16u) {
            CLEAR_FLAG(F_ZF);
            tmp8u = 0;
            while(!(tmp16u&(1<<tmp8u))) ++tmp8u;
            GW.word[0] = tmp8u;
        } else {
            SET_FLAG(F_ZF);
        }
        NEXT;
    _6f_0xBD:                      /* BSR Ew,Gw */
        CHECK_FLAGS(emu);
        nextop = F8;
        GET_EW;
        tmp16u = EW->word[0];
        if(tmp16u) {
            CLEAR_FLAG(F_ZF);
            tmp8u = 15;
            while(!(tmp16u&(1<<tmp8u))) --tmp8u;
            GW.word[0] = tmp8u;
        } else {
            SET_FLAG(F_ZF);
        }
        NEXT;
    _6f_0xBE:                      /* MOVSX Gw,Eb */
        nextop = F8;
        GET_EB;
        GW.sword[0] = EB->sbyte[0];
        NEXT;

    _6f_0xC1:                      /* XADD Gw,Ew */ // Xchange and Add
        nextop = F8;
        GET_EW;
        tmp16u = add16(emu, EW->word[0], GW.word[0]);
        GW.word[0] = EW->word[0];
        EW->word[0] = tmp16u;
        NEXT;
    _6f_0xC2:                      /* CMPPD Gx, Ex, Ib */
        nextop = F8;
        GET_EX;
        tmp8u = F8;
        for(int i=0; i<2; ++i) {
            tmp8s = 0;
            switch(tmp8u&7) {
                case 0: tmp8s=(GX.d[i] == EX->d[i]); break;
                case 1: tmp8s=isless(GX.d[i], EX->d[i]); break;
                case 2: tmp8s=islessequal(GX.d[i], EX->d[i]); break;
                case 3: tmp8s=isnan(GX.d[i]) || isnan(EX->d[i]); break;
                case 4: tmp8s=isnan(GX.d[i]) || isnan(EX->d[i]) || (GX.d[i] != EX->d[i]); break;
                case 5: tmp8s=isnan(GX.d[i]) || isnan(EX->d[i]) || isgreaterequal(GX.d[i], EX->d[i]); break;
                case 6: tmp8s=isnan(GX.d[i]) || isnan(EX->d[i]) || isgreater(GX.d[i], EX->d[i]); break;
                case 7: tmp8s=!isnan(GX.d[i]) && !isnan(EX->d[i]); break;
            }
            GX.q[i]=(tmp8s)?0xffffffffffffffffLL:0LL;
        }
        NEXT;

    _6f_0xC4:  /* PINSRW Gx,Ew,Ib */
        nextop = F8;
        GET_ED;
        tmp8u = F8;
        GX.uw[tmp8u&7] = ED->word[0];   // only low 16bits
        NEXT;
    _6f_0xC5:  /* PEXTRW Gw,Ex,Ib */
        nextop = F8;
        GET_EX;
        tmp8u = F8;
        GD.dword[0] = EX->uw[tmp8u&7];  // 16bits extract, 0 extended
        NEXT;
    _6f_0xC6:  /* SHUFPD Gx, Ex, Ib */
        nextop = F8;
        GET_EX;
        tmp8u = F8;
        eax1.q[0] = GX.q[tmp8u&1];
        eax1.q[1] = EX->q[(tmp8u>>1)&1];
        GX.q[0] = eax1.q[0];
        GX.q[1] = eax1.q[1];
        NEXT;

    _6f_0xC8:
    _6f_0xC9:
    _6f_0xCA:
    _6f_0xCB:
    _6f_0xCC:
    _6f_0xCD:
    _6f_0xCE:
    _6f_0xCF:                  /* BSWAP reg16 */
        tmp8s = opcode&7;
        emu->regs[tmp8s].word[0] = __builtin_bswap16(emu->regs[tmp8s].word[0]);
        NEXT;

    _6f_0xD1:  /* PSRLW Gx, Ex */
        nextop = F8;
        GET_EX;
        if(EX->q[0]>15)
            {GX.q[0] = GX.q[1] = 0;}
        else 
            {tmp8u=EX->ub[0]; for (int i=0; i<8; ++i) GX.uw[i] >>= tmp8u;}
        NEXT;
    _6f_0xD2:  /* PSRLD Gx, Ex */
        nextop = F8;
        GET_EX;
        if(EX->q[0]>31)
            {GX.q[0] = GX.q[1] = 0;}
        else 
            {tmp8u=EX->ub[0]; for (int i=0; i<4; ++i) GX.ud[i] >>= tmp8u;}
        NEXT;
    _6f_0xD3:  /* PSRLQ Gx, Ex */
        nextop = F8;
        GET_EX;
        if(EX->q[0]>63)
            {GX.q[0] = GX.q[1] = 0;}
        else 
            {tmp8u=EX->ub[0]; for (int i=0; i<2; ++i) GX.q[i] >>= tmp8u;}
        NEXT;
    _6f_0xD4:  /* PADDQ Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.sq[0] += EX->sq[0];
        GX.sq[1] += EX->sq[1];
        NEXT;
    _6f_0xD5:  /* PMULLW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i) {
            tmp32s = (int32_t)GX.sw[i] * EX->sw[i];
            GX.sw[i] = tmp32s&0xffff;
        }
        NEXT;
    _6f_0xD6:  /* MOVQ Ex,Gx */
        nextop = F8;
        GET_EX;
        EX->q[0] = GX.q[0];
        if((nextop&0xC0)==0xC0)
            EX->q[1] = 0;
        NEXT;
    _6f_0xD7:  /* PMOVMSKB Gd,Ex */
        nextop = F8;
        if((nextop&0xC0)==0xC0) {
            GET_EX;
            GD.dword[0] = 0;
            for (int i=0; i<16; ++i)
                if(EX->ub[i]&0x80)
                    GD.dword[0] |= (1<<i);
            NEXT;
        } else
            goto _default;
    _6f_0xD8:  /* PSUBUSB Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<16; ++i) {
            tmp16s = (int16_t)GX.ub[i] - EX->ub[i];
            GX.ub[i] = (tmp16s<0)?0:tmp16s;
        }
        NEXT;
    _6f_0xD9:  /* PSUBUSW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i) {
            tmp32s = (int32_t)GX.uw[i] - EX->uw[i];
            GX.uw[i] = (tmp32s<0)?0:tmp32s;
        }
        NEXT;
    _6f_0xDA:  /* PMINUB Gx, Ex */
        nextop = F8;
        GET_EX;
        for (int i=0; i<16; ++i)
            if(EX->ub[i]<GX.ub[i]) GX.ub[i] = EX->ub[i];
        NEXT;
    _6f_0xDB:  /* PAND Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] &= EX->q[0];
        GX.q[1] &= EX->q[1];
        NEXT;
    _6f_0xDC:  /* PADDUSB Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<16; ++i) {
            tmp16s = (int16_t)GX.ub[i] + EX->ub[i];
            GX.ub[i] = (tmp16s>255)?255:tmp16s;
        }
        NEXT;
    _6f_0xDD:  /* PADDUSW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i) {
            tmp32s = (int32_t)GX.uw[i] + EX->uw[i];
            GX.uw[i] = (tmp32s>65535)?65535:tmp32s;
        }
        NEXT;
    _6f_0xDE:  /* PMAXUB Gx, Ex */
        nextop = F8;
        GET_EX;
        for (int i=0; i<16; ++i)
            if(EX->ub[i]>GX.ub[i]) GX.ub[i] = EX->ub[i];
        NEXT;
    _6f_0xDF:  /* PANDN Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.q[0] = (~(GX.q[0])) & EX->q[0];
        GX.q[1] = (~(GX.q[1])) & EX->q[1];
        NEXT;
    _6f_0xE0:  /* PAVGB Gx, Ex */
        nextop = F8;
        GET_EX;
        for (int i=0; i<16; ++i)
            GX.ub[i] = ((uint16_t)GX.ub[i] + EX->ub[i] + 1)>>1;
        NEXT;
    _6f_0xE1:  /* PSRAW Gx, Ex */
        nextop = F8;
        GET_EX;
        tmp8u=(EX->q[0]>15)?16:EX->ub[0];
        for (int i=0; i<8; ++i) 
            GX.sw[i] >>= tmp8u;
        NEXT;
    _6f_0xE2:  /* PSRAD Gx, Ex */
        nextop = F8;
        GET_EX;
        tmp8u=(EX->q[0]>31)?32:EX->ub[0];
        for (int i=0; i<4; ++i)
            GX.sd[i] >>= tmp8u;
        NEXT;
    _6f_0xE3:  /* PAVGW Gx, Ex */
        nextop = F8;
        GET_EX;
        for (int i=0; i<8; ++i)
            GX.uw[i] = ((uint32_t)GX.uw[i] + EX->uw[i] + 1)>>1;
        NEXT;
    _6f_0xE4:  /* PMULHUW Gx, Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i) {
            tmp32u = (uint32_t)GX.uw[i] * EX->uw[i];
            GX.uw[i] = (tmp32u>>16)&0xffff;
        }
        NEXT;
    _6f_0xE5:  /* PMULHW Gx, Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i) {
            tmp32s = (int32_t)GX.sw[i] * EX->sw[i];
            GX.uw[i] = (tmp32s>>16)&0xffff;
        }
        NEXT;
    _6f_0xE6:  /* CVTTPD2DQ Gx, Ex */
        nextop = F8;
        GET_EX;
        GX.sd[0] = EX->d[0];
        GX.sd[1] = EX->d[1];
        GX.q[1] = 0;
        NEXT;
    _6f_0xE7:   /* MOVNTDQ Ex, Gx */
        nextop = F8;
        GET_EX;
        EX->q[0] = GX.q[0];
        EX->q[1] = GX.q[1];
        NEXT;
    _6f_0xE8:  /* PSUBSB Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<16; ++i) {
            tmp16s = (int16_t)GX.sb[i] - EX->sb[i];
            GX.sb[i] = (tmp16s<-128)?-128:((tmp16s>127)?127:tmp16s);
        }
        NEXT;
    _6f_0xE9:  /* PSUBSW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i) {
            tmp32s = (int32_t)GX.sw[i] - EX->sw[i];
            GX.sw[i] = (tmp32s>32767)?32767:((tmp32s<-32768)?-32768:tmp32s);
        }
        NEXT;
    _6f_0xEA:  /* PMINSW Gx,Ex */
        nextop = F8;
        GET_EX;
        for (int i=0; i<8; ++i)
            GX.sw[i] = (GX.sw[i]<EX->sw[i])?GX.sw[i]:EX->sw[i];
        NEXT;
    _6f_0xEB:  /* POR Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.ud[0] |= EX->ud[0];
        GX.ud[1] |= EX->ud[1];
        GX.ud[2] |= EX->ud[2];
        GX.ud[3] |= EX->ud[3];
        NEXT;
    _6f_0xEC:  /* PADDSB Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<16; ++i) {
            tmp16s = (int16_t)GX.sb[i] + EX->sb[i];
            GX.sb[i] = (tmp16s>127)?127:((tmp16s<-128)?-128:tmp16s);
        }
        NEXT;
    _6f_0xED:  /* PADDSW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i) {
            tmp32s = (int32_t)GX.sw[i] + EX->sw[i];
            GX.sw[i] = (tmp32s>32767)?32767:((tmp32s<-32768)?-32768:tmp32s);
        }
        NEXT;
    _6f_0xEE:  /* PMAXSW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i)
            GX.sw[i] = (GX.sw[i]>EX->sw[i])?GX.sw[i]:EX->sw[i];
        NEXT;
    _6f_0xEF:  /* PXOR Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.ud[0] ^= EX->ud[0];
        GX.ud[1] ^= EX->ud[1];
        GX.ud[2] ^= EX->ud[2];
        GX.ud[3] ^= EX->ud[3];
        NEXT;

    _6f_0xF1:  /* PSLLW Gx, Ex */
        nextop = F8;
        GET_EX;
        if(EX->q[0]>15)
            {GX.q[0] = GX.q[1] = 0;}
        else 
            {tmp8u=EX->ub[0]; for (int i=0; i<8; ++i) GX.uw[i] <<= tmp8u;}
        NEXT;
    _6f_0xF2:  /* PSLLD Gx, Ex */
        nextop = F8;
        GET_EX;
        if(EX->q[0]>31)
            {GX.q[0] = GX.q[1] = 0;}
        else 
            {tmp8u=EX->ub[0]; for (int i=0; i<4; ++i) GX.ud[i] <<= tmp8u;}
        NEXT;
    _6f_0xF3:  /* PSLLQ Gx, Ex */
        nextop = F8;
        GET_EX;
        if(EX->q[0]>63)
            {GX.q[0] = GX.q[1] = 0;}
        else 
            {tmp8u=EX->q[0]; for (int i=0; i<2; ++i) GX.q[i] <<= tmp8u;}
        NEXT;
    _6f_0xF4:  /* PMULUDQ Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.q[1] = (uint64_t)EX->ud[2]*GX.ud[2];
        GX.q[0] = (uint64_t)EX->ud[0]*GX.ud[0];
        NEXT;
    _6f_0xF5:  /* PMADDWD Gx,Ex */
        nextop = F8;
        GET_EX;
        for (int i=0; i<4; ++i)
            GX.sd[i] = (int32_t)(GX.sw[i*2+0])*EX->sw[i*2+0] + (int32_t)(GX.sw[i*2+1])*EX->sw[i*2+1];
        NEXT;
    _6f_0xF6:  /* PSADBW Gx, Ex */
        nextop = F8;
        GET_EX;
        tmp32u = 0;
        for (int i=0; i<8; ++i)
            tmp32u += (GX.ub[i]>EX->ub[i])?(GX.ub[i] - EX->ub[i]):(EX->ub[i] - GX.ub[i]);
        GX.q[0] = tmp32u;
        tmp32u = 0;
        for (int i=8; i<16; ++i)
            tmp32u += (GX.ub[i]>EX->ub[i])?(GX.ub[i] - EX->ub[i]):(EX->ub[i] - GX.ub[i]);
        GX.q[1] = tmp32u;
        NEXT;
    _6f_0xF7:  /* MASKMOVDQU Gx, Ex */
        nextop = F8;
        GET_EX;
        opx2 = (sse_regs_t *)(R_EDI);
        for (int i=0; i<16; ++i) {
            if(EX->ub[i]&0x80)
                opx2->ub[i] = GX.ub[i];
        }
        NEXT;
    _6f_0xF8:  /* PSUBB Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<16; ++i)
            GX.sb[i] -= EX->sb[i];
        NEXT;
    _6f_0xF9:  /* PSUBW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i)
            GX.sw[i] -= EX->sw[i];
        NEXT;
    _6f_0xFA:  /* PSUBD Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.sd[0] -= EX->sd[0];
        GX.sd[1] -= EX->sd[1];
        GX.sd[2] -= EX->sd[2];
        GX.sd[3] -= EX->sd[3];
        NEXT;
    _6f_0xFB:  /* PSUBQ Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.sq[0] -= EX->sq[0];
        GX.sq[1] -= EX->sq[1];
        NEXT;
    _6f_0xFC:  /* PADDB Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<16; ++i)
            GX.sb[i] += EX->sb[i];
        NEXT;
    _6f_0xFD:  /* PADDW Gx,Ex */
        nextop = F8;
        GET_EX;
        for(int i=0; i<8; ++i)
            GX.sw[i] += EX->sw[i];
        NEXT;
    _6f_0xFE:  /* PADDD Gx,Ex */
        nextop = F8;
        GET_EX;
        GX.sd[0] += EX->sd[0];
        GX.sd[1] += EX->sd[1];
        GX.sd[2] += EX->sd[2];
        GX.sd[3] += EX->sd[3];
        NEXT;
 
    _fini:
        R_EIP = ip;
        return;
}