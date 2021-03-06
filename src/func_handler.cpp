#include "func_handler.h"
#include "instruction_handler.h"
#include <iostream>

using namespace intervalai;

FuncHandler::FuncHandler(goto_modelt *m, unsigned int widen_limit, RunMode mode)
    : mode(mode), widen_limit(widen_limit), instruction_handler(mode) {
    assert(m != nullptr);
    model = m;
    prompt = "IntervalAI>> ";
    if (mode == RunMode::Interactive) {
        std::cout << "IntervalAI will run in interactive mode. Each "
                     "instruction will be executed step-wise. Use the "
                     "following commands:\np : display the current state and "
                     "the next instruction\nn : execute the next "
                     "instruction\nw : maximum number of iterations after "
                     "which the loop will be widened"
                  << std::endl;
        std::cout << "When a branch is encountered, you will be prompted to "
                     "specify which branch to take. The specified branch will "
                     "be used for verification."
                  << std::endl
                  << std::endl;
    } else {
        std::cout << "IntervalAI will run in automated mode." << std::endl
                  << std::endl;
    }
}

bool FuncHandler::handleInstruction(std::_List_iterator<instructiont> current) {
    displayInfo(current);
    auto instruction = *current;
    if (instruction.is_return()) {
        Interval interval = instruction_handler.handleReturn(instruction);
        func_return = interval;
        loop_count.clear();
        return true;
    }
    if (instruction.is_end_function()) {
        loop_count.clear();
        return true;
    }
    if (instruction.is_function_call()) {
        auto operands = instruction_handler.handleFunctionCall(instruction);
        auto params = model->goto_functions.function_map[std::get<1>(operands)]
                          .type.parameters();
        auto argument_it = std::get<2>(operands).begin();

        for (auto &param : params) {
            instruction_handler.expr_handler
                .symbol_table[param.get_identifier()] = *argument_it;
            argument_it++;
        }

        auto return_val = handleFunc(id2string(std::get<1>(operands)));
        if (!return_val.first) {
            return false;
        }
        if (id2string(std::get<0>(operands)) != "") {
            instruction_handler.expr_handler
                .symbol_table[id2string(std::get<0>(operands))] =
                return_val.second;
        }
    }
    if (instruction.is_goto()) {
        if (instruction.is_backwards_goto()) {
            if (loop_count.find(instruction.location_number) ==
                loop_count.end()) {
                loop_count[instruction.location_number] = 0;
            }
            loop_count[instruction.location_number] += 1;
            if (loop_count[instruction.location_number] >= widen_limit) {
                std::cout << "Limit on number of iterations (" << widen_limit
                          << ") reached." << std::endl
                          << "Widening will be applied. Instruction : "
                          << instruction.location_number << ", "
                          << instruction.to_string() << std::endl;
                auto iter = current;
                while (iter != instruction.targets.front()) {
                    if ((*iter).is_assign()) {
                        auto assign = static_cast<code_assignt &>((*iter).code);
                        instruction_handler.expr_handler.symbol_table
                            [assign.lhs().get_named_sub()["identifier"].id()] =
                            Interval();
                    }
                    iter--;
                }
                return handleInstruction(++current);
            }
        }
        intervalai::tribool guard = instruction_handler.handleGoto(*current);
        if (guard == intervalai::tribool::True) {
            return handleInstruction(instruction.targets.front());
        } else if (guard == intervalai::tribool::False) {
            return handleInstruction(++current);
        } else {
            if (mode == RunMode::Interactive) {
                std::string input;
                std::cout << prompt;
                std::cout << "A branch has been encountered. Instruction : "
                          << (*current).location_number << ", "
                          << (*current).to_string() << std::endl;
                std::cout
                    << "Select an option : (1) Then Branch, (2) Else Branch"
                    << std::endl;
                std::cin >> input;
                if (input == "1") {
                    std::cout << "Selecting the Then Branch" << std::endl;
                    return handleInstruction(instruction.targets.front());
                } else {
                    std::cout << "Selecting the Else Branch" << std::endl;
                    return handleInstruction(++current);
                }
            } else {
                bool branch1 = handleInstruction(instruction.targets.front());
                if (branch1) {
                    return handleInstruction(++current);
                } else {
                    return false;
                }
            }
        }
    }
    bool safe = instruction_handler.handleInstruction(instruction);
    if (!safe) {
        return false;
    }
    return handleInstruction(++current);
}

void FuncHandler::displayInfo(std::_List_iterator<instructiont> current) {
    if (mode == RunMode::Automated) {
        return;
    }
    std::string input;

    while (true) {
        std::cout << prompt;
        std::cin >> input;
        if (input == "n") {
            std::cout << "Executed instruction : " << (*current).location_number
                      << ", " << (*current).to_string() << std::endl;
            return;
        } else if (input == "p") {
            for (auto &sym : instruction_handler.expr_handler.symbol_table) {
                std::cout << sym.first << " : " << sym.second.to_string()
                          << std::endl;
            }
            std::cout << "Next instruction : " << (*current).location_number
                      << ", " << (*current).to_string() << std::endl;
        } else {
            std::cout << "Invalid command" << std::endl;
        }
    }
}

std::pair<bool, Interval> FuncHandler::handleFunc(std::string func_name) {
    auto instructions =
        model->goto_functions.function_map[func_name].body.instructions;
    intervalai::InstructionHandler instruction_handler;
    auto params =
        model->goto_functions.function_map[func_name].type.parameters();

    bool is_safe = handleInstruction(instructions.begin());
    return std::make_pair(is_safe, func_return);
}
