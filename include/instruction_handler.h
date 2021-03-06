#ifndef INSTRUCTION_HANDLER_H
#define INSTRUCTION_HANDLER_H

#include "expr_handler.h"
#include "interval_domain.h"
#include "io_utils.h"
#include <goto-programs/goto_program_template.h>
#include <util/std_code.h>

typedef goto_program_templatet<codet, exprt>::instructiont instructiont;

namespace intervalai {

class InstructionHandler {

  private:
    RunMode mode;

  public:
    ExprHandler expr_handler;
    InstructionHandler(RunMode mode = RunMode::Automated);
    bool handleInstruction(instructiont);

    tribool handleGoto(instructiont);
    void handleAssume(instructiont);
    tribool handleAssert(instructiont);
    void handleOther(instructiont);
    void handleSkip(instructiont);
    Interval handleReturn(instructiont);
    void handleAssign(instructiont);
    void handleDecl(instructiont);
    void handleDead(instructiont);
    std::tuple<dstringt, dstringt, std::vector<Interval>>
        handleFunctionCall(instructiont);
};

}; // namespace intervalai

#endif