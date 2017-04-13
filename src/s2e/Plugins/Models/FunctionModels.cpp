///
/// Copyright (C) 2017, Dependable Systems Laboratory, EPFL
/// Copyright (C) 2015-2016, Cyberhaven
/// All rights reserved.
///
/// Licensed under the Cyberhaven Research License Agreement.
///

#include <s2e/cpu.h>
#include <s2e/function_models/commands.h>

#include <klee/util/ExprTemplates.h>
#include <llvm/Support/CommandLine.h>
#include <s2e/ConfigFile.h>
#include <s2e/S2E.h>
#include <s2e/S2EExecutor.h>
#include <s2e/Utils.h>

#include <algorithm>

#include "FunctionModels.h"

using namespace klee;

namespace s2e {
namespace plugins {
namespace models {

S2E_DEFINE_PLUGIN(FunctionModels, "Plugin that implements models for libraries", "");

ref<Expr> FunctionModels::readMemory8(S2EExecutionState *state, uint64_t addr) {
    return state->readMemory8(addr);
}

void FunctionModels::handleStrlen(S2EExecutionState *state, S2E_LIBCWRAPPER_COMMAND &cmd, ref<Expr> &retExpr) {
    // Read function arguments
    uint64_t stringAddr = (uint64_t) cmd.Strlen.str;

    // Assemble the string length expression
    size_t len;
    if (strlenHelper(state, stringAddr, len, retExpr)) {
        cmd.needOrigFunc = 0;
    } else {
        cmd.needOrigFunc = 1;
    }
}

void FunctionModels::handleStrcmp(S2EExecutionState *state, S2E_LIBCWRAPPER_COMMAND &cmd, ref<Expr> &retExpr) {
    // Read function arguments
    uint64_t stringAddrs[2];
    stringAddrs[0] = (uint64_t) cmd.Strcmp.str1;
    stringAddrs[1] = (uint64_t) cmd.Strcmp.str2;

    // Assemble the string compare expression
    if (strcmpHelper(state, stringAddrs, retExpr)) {
        cmd.needOrigFunc = 0;
    } else {
        cmd.needOrigFunc = 1;
    }
}

void FunctionModels::handleStrncmp(S2EExecutionState *state, S2E_LIBCWRAPPER_COMMAND &cmd, ref<Expr> &retExpr) {
    // Read function arguments
    uint64_t stringAddrs[2];
    stringAddrs[0] = (uint64_t) cmd.Strncmp.str1;
    stringAddrs[1] = (uint64_t) cmd.Strncmp.str2;
    size_t nSize = cmd.Strncmp.n;

    // Assemble the string compare expression
    if (strncmpHelper(state, stringAddrs, nSize, retExpr)) {
        cmd.needOrigFunc = 0;
    } else {
        cmd.needOrigFunc = 1;
    }
}

void FunctionModels::handleStrcpy(S2EExecutionState *state, S2E_LIBCWRAPPER_COMMAND &cmd) {
    // Read function arguments
    uint64_t stringAddrs[2];
    stringAddrs[0] = (uint64_t) cmd.Strcpy.dst;
    stringAddrs[1] = (uint64_t) cmd.Strcpy.src;

    // Perform the string copy. We don't use the return expression here because it is just a concrete address
    ref<Expr> retExpr;
    if (strcpyHelper(state, stringAddrs, retExpr)) {
        cmd.needOrigFunc = 0;
    } else {
        cmd.needOrigFunc = 1;
    }
}

void FunctionModels::handleStrncpy(S2EExecutionState *state, S2E_LIBCWRAPPER_COMMAND &cmd) {
    // Read function arguments
    uint64_t stringAddrs[2];
    stringAddrs[0] = (uint64_t) cmd.Strncpy.dst;
    stringAddrs[1] = (uint64_t) cmd.Strncpy.src;
    uint64_t numBytes = cmd.Strncpy.n;

    // Perform the string copy. We don't use the return expression here because it is just a concrete address
    ref<Expr> retExpr;
    if (strncpyHelper(state, stringAddrs, numBytes, retExpr)) {
        cmd.needOrigFunc = 0;
    } else {
        cmd.needOrigFunc = 1;
    }
}

void FunctionModels::handleMemcpy(S2EExecutionState *state, S2E_LIBCWRAPPER_COMMAND &cmd) {
    // Read function arguments
    uint64_t memAddrs[2];
    memAddrs[0] = (uint64_t) cmd.Memcpy.dst;
    memAddrs[1] = (uint64_t) cmd.Memcpy.src;
    uint64_t numBytes = (int) cmd.Memcpy.n;

    // Perform the memory copy. We don't use the return expression here because it is just a concrete address
    ref<Expr> retExpr;
    if (memcpyHelper(state, memAddrs, numBytes, retExpr)) {
        cmd.needOrigFunc = 0;
    } else {
        cmd.needOrigFunc = 1;
    }
}

void FunctionModels::handleMemcmp(S2EExecutionState *state, S2E_LIBCWRAPPER_COMMAND &cmd, ref<Expr> &retExpr) {
    // Read function arguments
    uint64_t memAddrs[2];
    memAddrs[0] = (uint64_t) cmd.Memcmp.str1;
    memAddrs[1] = (uint64_t) cmd.Memcmp.str2;
    uint64_t numBytes = (int) cmd.Memcmp.n;

    // Assemble the memory compare expression
    if (memcmpHelper(state, memAddrs, numBytes, retExpr)) {
        cmd.needOrigFunc = 0;
    } else {
        cmd.needOrigFunc = 1;
    }
}

void FunctionModels::handleStrcat(S2EExecutionState *state, S2E_LIBCWRAPPER_COMMAND &cmd) {
    // Read function arguments
    uint64_t stringAddrs[2];
    stringAddrs[0] = (uint64_t) cmd.Strcat.dst;
    stringAddrs[1] = (uint64_t) cmd.Strcat.src;

    getDebugStream(state) << "Handling strcat(" << hexval(stringAddrs[0]) << ", " << hexval(stringAddrs[1]) << ")\n";

    // Assemble the string concatination expression. We don't use the return expression here because it is just a
    // concrete address
    ref<Expr> retExpr;
    if (strcatHelper(state, stringAddrs, retExpr)) {
        cmd.needOrigFunc = 0;
    } else {
        cmd.needOrigFunc = 1;
    }

    return;
}

void FunctionModels::handleStrncat(S2EExecutionState *state, S2E_LIBCWRAPPER_COMMAND &cmd) {
    // Read function arguments
    uint64_t stringAddrs[2];
    stringAddrs[0] = (uint64_t) cmd.Strncat.dst;
    stringAddrs[1] = (uint64_t) cmd.Strncat.src;
    uint64_t numBytes = (int) cmd.Strncat.n;

    getDebugStream(state) << "Handling strncat(" << hexval(stringAddrs[0]) << ", " << hexval(stringAddrs[1]) << ", "
                          << numBytes << ")\n";

    // Assemble the string concatination expression. We don't use the return expression here because it is just a
    // concrete address
    ref<Expr> retExpr;
    if (strcatHelper(state, stringAddrs, retExpr, true, numBytes)) {
        cmd.needOrigFunc = 0;
    } else {
        cmd.needOrigFunc = 1;
    }

    return;
}

#define UPDATE_RET_VAL(CmdType, cmd)                                         \
    do {                                                                     \
        uint32_t offRet = offsetof(S2E_LIBCWRAPPER_COMMAND, CmdType.ret);    \
                                                                             \
        if (!state->mem()->writeMemory(guestDataPtr, cmd)) {                 \
            getWarningsStream(state) << "Could not write to guest memory\n"; \
        }                                                                    \
                                                                             \
        if (!state->mem()->writeMemory(guestDataPtr + offRet, retExpr)) {    \
            getWarningsStream(state) << "Could not write to guest memory\n"; \
        }                                                                    \
    } while (0)

void FunctionModels::handleOpcodeInvocation(S2EExecutionState *state, uint64_t guestDataPtr, uint64_t guestDataSize) {
    S2E_LIBCWRAPPER_COMMAND command;

    if (guestDataSize != sizeof(command)) {
        getWarningsStream(state) << "S2E_LIBCWRAPPER_COMMAND: "
                                 << "mismatched command structure size " << guestDataSize << "\n";
        exit(-1);
    }

    if (!state->mem()->readMemoryConcrete(guestDataPtr, &command, guestDataSize)) {
        getWarningsStream(state) << "S2E_LIBCWRAPPER_COMMAND: could not read transmitted data\n";
        exit(-1);
    }

    switch (command.Command) {
        case LIBCWRAPPER_STRCPY: {
            handleStrcpy(state, command);
            if (!state->mem()->writeMemory(guestDataPtr, command)) {
                getWarningsStream(state) << "Could not write to guest memory\n";
            }
        } break;

        case LIBCWRAPPER_STRNCPY: {
            handleStrncpy(state, command);
            if (!state->mem()->writeMemory(guestDataPtr, command)) {
                getWarningsStream(state) << "Could not write to guest memory\n";
            }
        } break;

        case LIBCWRAPPER_STRLEN: {
            ref<Expr> retExpr;
            handleStrlen(state, command, retExpr);
            UPDATE_RET_VAL(Strlen, command);
        } break;

        case LIBCWRAPPER_STRCMP: {
            ref<Expr> retExpr;
            handleStrcmp(state, command, retExpr);
            UPDATE_RET_VAL(Strcmp, command);
        } break;

        case LIBCWRAPPER_STRNCMP: {
            ref<Expr> retExpr;
            handleStrncmp(state, command, retExpr);
            UPDATE_RET_VAL(Strncmp, command);
        } break;

        case LIBCWRAPPER_MEMCPY: {
            handleMemcpy(state, command);
            if (!state->mem()->writeMemory(guestDataPtr, command)) {
                getWarningsStream(state) << "Could not write to guest memory\n";
            }
        } break;

        case LIBCWRAPPER_MEMCMP: {
            ref<Expr> retExpr;
            handleMemcmp(state, command, retExpr);
            UPDATE_RET_VAL(Memcmp, command);
        } break;

        case LIBCWRAPPER_STRCAT: {
            handleStrcat(state, command);
            if (!state->mem()->writeMemory(guestDataPtr, command)) {
                getWarningsStream(state) << "Could not write to guest memory\n";
            }
        } break;

        case LIBCWRAPPER_STRNCAT: {
            handleStrncat(state, command);
            if (!state->mem()->writeMemory(guestDataPtr, command)) {
                getWarningsStream(state) << "Could not write to guest memory\n";
            }
        } break;

        default: {
            getWarningsStream(state) << "Invalid command " << hexval(command.Command) << "\n";
            exit(-1);
        }
    }
}

} // namespace models
} // namespace plugins
} // namespace s2e