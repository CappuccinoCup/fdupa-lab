#include "intervalAnalysis.h"

#include <algorithm>
#include <array>
#include <queue>
#include <vector>

using namespace fdlang;
using namespace fdlang::analysis;

/**
 * Your code starts here
 */

void IntervalAnalysis::init() {
    for (auto inst : insts) {
        auto type = inst->getInstType();
        if (type == IR::InstType::AssignInst) {
            IR::AssignInst *assignInst = (IR::AssignInst *)inst;
            auto dest = assignInst->getOperand(0);
            if (!dest->isVariable()) {
                continue;
            }
            vars.emplace(dest->getAsVariable());
            auto src = assignInst->getOperand(1);
            if (src->isVariable()) {
                vars.emplace(src->getAsVariable());
            }
        }
        if (type == IR::InstType::InputInst) {
            IR::InputInst *inputInst = (IR::InputInst *)inst;
            auto dest = inputInst->getOperand(0);
            if (!dest->isVariable()) {
                continue;
            }
            vars.emplace(dest->getAsVariable());
        }
        if (type == IR::InstType::AddInst) {
            IR::AddInst *addInst = (IR::AddInst *)inst;
            auto dest = addInst->getOperand(0);
            if (!dest->isVariable()) {
                continue;
            }
            vars.emplace(dest->getAsVariable());
            auto op1 = addInst->getOperand(1);
            if (op1->isVariable()) {
                vars.emplace(op1->getAsVariable());
            }
            auto op2 = addInst->getOperand(2);
            if (op2->isVariable()) {
                vars.emplace(op2->getAsVariable());
            }
        }
        if (type == IR::InstType::SubInst) {
            IR::SubInst *subInst = (IR::SubInst *)inst;
            auto dest = subInst->getOperand(0);
            if (!dest->isVariable()) {
                continue;
            }
            vars.emplace(dest->getAsVariable());
            auto op1 = subInst->getOperand(1);
            if (op1->isVariable()) {
                vars.emplace(op1->getAsVariable());
            }
            auto op2 = subInst->getOperand(2);
            if (op2->isVariable()) {
                vars.emplace(op2->getAsVariable());
            }
        }
        if (type == IR::InstType::GotoInst) {
            IR::GotoInst *gotoInst = (IR::GotoInst *)inst;
            targetToJump[gotoInst->getDestInst()->getLabel()] = gotoInst->getLabel();
            BranchInfo* bInfo = new BranchInfo();
            branchInfos[gotoInst->getLabel()] = bInfo;
        }
        if (type == IR::InstType::IfInst) {
            IR::IfInst *ifInst = (IR::IfInst *)inst;
            targetToJump[ifInst->getDestInst()->getLabel()] = ifInst->getLabel();
            std::string op1 = ifInst->getOperand(0)->getAsVariable();
            int op2 = ifInst->getOperand(1)->getAsNumber();
            int s, e;
            switch (ifInst->getCmpOperator())
            {
            case fdlang::IR::CmpOperator::EQ:
                s = op2;
                e = op2;
                break;
            case fdlang::IR::CmpOperator::GEQ:
                s = op2;
                e = 255;
                break;
            case fdlang::IR::CmpOperator::GT:
                s = op2 + 1;
                e = 255;
                break;
            case fdlang::IR::CmpOperator::LEQ:
                s = 0;
                e = op2;
                break;
            case fdlang::IR::CmpOperator::LT:
                s = 0;
                e = op2 - 1;
                break;
            default:
                break;
            }
            BranchInfo* bInfo = new BranchInfo(op1, s, e);
            branchInfos[ifInst->getLabel()] = bInfo;
            vars.emplace(op1);
        }
        if (type == IR::InstType::CheckIntervalInst) {
            IR::CheckIntervalInst *checkInst = (IR::CheckIntervalInst *)inst;
            int l = checkInst->getOperand(1)->getAsNumber();
            int r = checkInst->getOperand(2)->getAsNumber();
            CheckInfo *cInfo = new CheckInfo(checkInst, l, r);
            checkInfos[checkInst->getLabel()] = cInfo;
        }
    }
}

void IntervalAnalysis::iter() {
    bool changed = true;
    while (changed)
    {
    // debug
    std::cout << std::endl << "=======" << std::endl;
    // debug
    changed = false;
    for (auto& v : vars) {
        currRange.insertVar(v, Range(0, 0));
    }
    for (auto inst : insts)
    {   
        // debug
        std::cout << "CurrRange: ";
        currRange.print();
        std::cout << std::endl;
        // debug
        auto type = inst->getInstType();
        if (type == IR::InstType::AssignInst) {
            IR::AssignInst *assignInst = (IR::AssignInst *)inst;
            auto dest = assignInst->getOperand(0);
            if (!dest->isVariable()) {
                continue;
            }
            auto src = assignInst->getOperand(1);
            if (src->isNumber()) {
                auto value = src->getAsNumber();
                currRange.insertVar(dest->getAsVariable(), Range(value, value));
            }
            if (src->isVariable()) {
                auto value = src->getAsVariable();
                currRange.insertVar(dest->getAsVariable(), currRange.getVar(value));
            }
        }
        if (type == IR::InstType::InputInst) {
            IR::InputInst *inputInst = (IR::InputInst *)inst;
            auto dest = inputInst->getOperand(0);
            if (!dest->isVariable()) {
                continue;
            }
            currRange.insertVar(dest->getAsVariable(), Range(0, 255));
        }
        if (type == IR::InstType::AddInst) {
            IR::AddInst *addInst = (IR::AddInst *)inst;
            auto dest = addInst->getOperand(0);
            if (!dest->isVariable()) {
                continue;
            }
            auto op1 = addInst->getOperand(1);
            auto op2 = addInst->getOperand(2);
            Range op1R, op2R;
            if (op1->isNumber()) {
                op1R = Range(op1->getAsNumber(), op1->getAsNumber());
            }
            if (op1->isVariable()) {
                op1R = currRange.getVar(op1->getAsVariable());
            }
            if (op2->isNumber()) {
                op2R = Range(op2->getAsNumber(), op2->getAsNumber());
            }
            if (op2->isVariable()) {
                op2R = currRange.getVar(op2->getAsVariable());
            }
            op1R.range_add(op2R);
            currRange.insertVar(dest->getAsVariable(), op1R);
        }
        if (type == IR::InstType::SubInst) {
            IR::SubInst *subInst = (IR::SubInst *)inst;
            auto dest = subInst->getOperand(0);
            if (!dest->isVariable()) {
                continue;
            }
            auto op1 = subInst->getOperand(1);
            auto op2 = subInst->getOperand(2);
            Range op1R, op2R;
            if (op1->isNumber()) {
                op1R = Range(op1->getAsNumber(), op1->getAsNumber());
            }
            if (op1->isVariable()) {
                op1R = currRange.getVar(op1->getAsVariable());
            }
            if (op2->isNumber()) {
                op2R = Range(op2->getAsNumber(), op2->getAsNumber());
            }
            if (op2->isVariable()) {
                op2R = currRange.getVar(op2->getAsVariable());
            }
            op1R.range_minus(op2R);
            currRange.insertVar(dest->getAsVariable(), op1R);
        }
        if (type == IR::InstType::GotoInst) {
            IR::GotoInst *gotoInst = (IR::GotoInst *)inst;
            auto bInfo = branchInfos[gotoInst->getLabel()];
            changed = changed || bInfo->setGotoRange(currRange);
            if (gotoInst->getDestInst()->getLabel() > gotoInst->getLabel()) {
                // Only jump forward needs a new iteration
                changed = false;
            }
        }
        if (type == IR::InstType::IfInst) {
            IR::IfInst *ifInst = (IR::IfInst *)inst;
            auto bInfo = branchInfos[ifInst->getLabel()];
            changed = changed || bInfo->setIfRange(currRange);
            if (ifInst->getDestInst()->getLabel() > ifInst->getLabel()) {
                // Only jump forward needs a new iteration
                changed = false;
            }
        }
        if (type == IR::InstType::LabelInst) {
            IR::LabelInst *labelInst = (IR::LabelInst *)inst;
            auto jumpRange = branchInfos[targetToJump[labelInst->getLabel()]]->getJumpRange();
            currRange.range_union(jumpRange);
        }
        if (type == IR::InstType::CheckIntervalInst) {
            IR::CheckIntervalInst *checkInst = (IR::CheckIntervalInst *)inst;
            auto cInfo = checkInfos[checkInst->getLabel()];
            cInfo->updateRealRange(currRange);
        }
    }
    }
}

void IntervalAnalysis::done() {
    for (auto& cInfo : checkInfos) {
        results[cInfo.second->getInst()] = cInfo.second->getResultType();
    }
    for (auto& bInfo : branchInfos) {
        delete bInfo.second;
    }
    branchInfos.clear();
    for (auto& cInfo : checkInfos) {
        delete cInfo.second;
    }
    checkInfos.clear();
}

void IntervalAnalysis::run() {
    init();
    iter();
    done();
}
