#include <cinttypes>
#include <thirdparty/loguru/loguru.hpp>
#include "ppcemu.h"
#include "ppcmmu.h"
#include "ppcdefs.h"
#include "jittables.h"
#include "nuinterpreter.h"

#if defined(USE_DTC)

#define CAST(name)          &&lab_##name

struct InterpInstr {
    void*       emu_fn;
    InstrInfo   info;
};

#define REG_D(opcode)   ((opcode >> 21) & 0x1F)
#define REG_A(opcode)   ((opcode >> 16) & 0x1F)
#define REG_B(opcode)   ((opcode >> 11) & 0x1F)
#define REG_S           REG_D
#define BR_BO           REG_D
#define BR_BI           REG_A
#define SIMM(opcode)    ((int32_t)((int16_t)(opcode & 0xFFFF)))
#define UIMM(opcode)    ((uint32_t)((uint16_t)(opcode & 0xFFFF)))

/** mask generator for rotate and shift instructions (§ 4.2.1.4 PowerpC PEM) */
static inline uint32_t rot_mask(unsigned rot_mb, unsigned rot_me) {
    uint32_t m1 = 0xFFFFFFFFUL >> rot_mb;
    uint32_t m2 = 0xFFFFFFFFUL << (31 - rot_me);
    return ((rot_mb <= rot_me) ? m2 & m1 : m1 | m2);
}

CachedInstr code_block[256];

void NuInterpExec(uint32_t next_pc) {
    /* WARNING: entries in this table MUST be in the same order as constants in PPCInstr! */
    InterpInstr interp_tab[] = {
        {nullptr,        {InstrOps::opNone,   CFlowType::CFL_NONE,           0}},
        {CAST(addi),     {InstrOps::opDASimm, CFlowType::CFL_NONE,           1}},
        {CAST(adde),     {InstrOps::opDAB,    CFlowType::CFL_NONE,           1}},
        {CAST(addze),    {InstrOps::opDA,     CFlowType::CFL_NONE,           1}},
        {CAST(andidot),  {InstrOps::opSAUimm, CFlowType::CFL_NONE,           1}},
        {CAST(lwz),      {InstrOps::opDASimm, CFlowType::CFL_NONE,           1}},
        {CAST(lwzu),     {InstrOps::opDASimm, CFlowType::CFL_NONE,           1}},
        {CAST(lbz),      {InstrOps::opDASimm, CFlowType::CFL_NONE,           1}},
        {CAST(lhz),      {InstrOps::opDASimm, CFlowType::CFL_NONE,           1}},
        {CAST(rlwinm),   {InstrOps::opRot,    CFlowType::CFL_NONE,           1}},
        {CAST(srawidot), {InstrOps::opSASh,   CFlowType::CFL_NONE,           1}},
        {CAST(bc),       {InstrOps::opBrRel,  CFlowType::CFL_COND_BRANCH,    0}},
        {CAST(bdnz),     {InstrOps::opBrRel,  CFlowType::CFL_COND_BRANCH,    0}},
        {CAST(bdz),      {InstrOps::opBrRel,  CFlowType::CFL_COND_BRANCH,    0}},
        {CAST(bclr),     {InstrOps::opBrLink, CFlowType::CFL_COND_BRANCH,    0}},
        {CAST(mtspr),    {InstrOps::opSSpr,   CFlowType::CFL_NONE,           2}},
        {CAST(bexit),    {InstrOps::opNone,   CFlowType::CFL_UNCOND_BRANCH,  0}},
    };

    CachedInstr* c_instr;
    uint32_t opcode, main_opcode;
    unsigned instr_index;
    uint8_t* pc_real;
    bool     done;

    pc_real = quickinstruction_translate(next_pc);

    c_instr = &code_block[0];

    done = false;

    while (!done) {
        opcode      = ppc_cur_instruction;
        main_opcode = opcode >> 26;

        instr_index = main_index_tab[main_opcode];
        if (!instr_index) {
            switch (main_opcode) {
            case 16:
                instr_index = subgrp16_index_tab[opcode & 3];
                break;
            case 18:
                instr_index = subgrp18_index_tab[opcode & 3];
                break;
            case 19:
                instr_index = subgrp19_index_tab[opcode & 0x7FF];
                break;
            case 31:
                instr_index = subgrp31_index_tab[opcode & 0x7FF];
                break;
            case 59:
                instr_index = subgrp59_index_tab[opcode & 0x3F];
                break;
            case 63:
                instr_index = subgrp63_index_tab[opcode & 0x7FF];
                break;
            default:
                instr_index = 0;
            }

            if (!instr_index) {
                LOG_F(INFO, "Illegal opcode 0x%08X - Program exception!", opcode);
                return;
            }
        }

        const InterpInstr* p_instr = &interp_tab[instr_index];

        c_instr->call_me = p_instr->emu_fn;

        /* pre-decode operands, immediate values etc. */
        switch (p_instr->info.ops_fmt) {
        case InstrOps::opDA:
            c_instr->d1   = REG_D(opcode);
            c_instr->d2   = REG_A(opcode);
            break;
        case InstrOps::opDAB:
            c_instr->d1   = REG_D(opcode);
            c_instr->d2   = REG_A(opcode);
            c_instr->d3   = REG_B(opcode);
            break;
        case InstrOps::opDASimm:
            c_instr->d1   = REG_D(opcode);
            c_instr->d2   = REG_A(opcode);
            c_instr->simm = SIMM(opcode);
            break;
        case InstrOps::opSAUimm:
            c_instr->d1   = REG_S(opcode);
            c_instr->d2   = REG_A(opcode);
            c_instr->uimm = UIMM(opcode);
            break;
        case InstrOps::opSASh:
            c_instr->d1   = REG_S(opcode);
            c_instr->d2   = REG_A(opcode);
            c_instr->d3   = (opcode >> 11) & 0x1F;  // shift
            c_instr->uimm = (1 << c_instr->d3) - 1; // mask
            break;
        case InstrOps::opRot:
            c_instr->d1   = REG_S(opcode);
            c_instr->d2   = REG_A(opcode);
            c_instr->d3   = (opcode >> 11) & 0x1F;  // shift
            c_instr->d4   = opcode & 1; // Rc bit
            c_instr->uimm = rot_mask((opcode >> 6) & 31, (opcode >> 1) & 31); //mask
            break;
        case InstrOps::opSSpr:
            c_instr->d1   = REG_S(opcode);
            c_instr->uimm = (REG_B(opcode) << 5) | REG_A(opcode);
            break;
        case InstrOps::opBrRel:
            c_instr->bt = SIMM(opcode) >> 2;
            switch (BR_BO(opcode) & 0x1E) {
            case 12:
            case 14:
                c_instr->call_me = interp_tab[PPCInstr::bc].emu_fn;
                c_instr->uimm = 0x80000000UL >> BR_BI(opcode);
                break;
            case 16:
                c_instr->call_me = interp_tab[PPCInstr::bdnz].emu_fn;
                break;
            case 18:
                c_instr->call_me = interp_tab[PPCInstr::bdz].emu_fn;
                break;
            default:
                LOG_F(ERROR, "Unsupported opcode 0x%08X - Program exception!", opcode);
                return;
            }
            break;
        case InstrOps::opBrLink:
            switch (BR_BO(opcode) & 0x1E) {
            case 20: // blr
                c_instr->call_me = interp_tab[PPCInstr::bexit].emu_fn;
                c_instr->uimm    = next_pc;
                done = true;
                continue;
            default:
                LOG_F(ERROR, "Unsupported opcode 0x%08X - Program exception!", opcode);
                return;
            }
            break;
        default:
            LOG_F(ERROR, "Unknown opcode format %d!", p_instr->info.ops_fmt);
            return;
        }

        c_instr++;

        next_pc += 4;
        pc_real += 4;
        ppc_set_cur_instruction(pc_real);
    } // while

    //LOG_F(INFO, "PreDecode completed!");

    CachedInstr* code = &code_block[0];

    goto *(code->call_me); // begin code execution

    #define GEN_OP(name,body)   lab_##name: { body; }
    #define NEXT                goto *((++code)->call_me)

    #include "interpops.h" // auto-generate labeled emulation blocks
}

#endif
