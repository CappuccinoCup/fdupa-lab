#ifndef ANALYSIS_INTERVALANALYSIS_H
#define ANALYSIS_INTERVALANALYSIS_H

#include "dataflowAnalysis.h"

#include <algorithm>
#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace fdlang::analysis {

enum class ResultType { YES, NO, UNREACHABLE };

/**
 * Range of a variable
 * [x: {a, b}, {c, d}] means x in [a, b] v [c, d]
*/
class Range {
private:
    static constexpr int LOWER_BOUND = 0;
    static constexpr int UPPER_BOUND = 255;

    std::vector<std::pair<int, int>> rangeList;

    // get the range list
    std::vector<std::pair<int, int>> getRangeList() {
        return rangeList;
    }

    // insert an integer pair to the range list
    void insert(int s, int e) {
        s = std::max(s, LOWER_BOUND);
        e = std::min(e, UPPER_BOUND);
        if (s <= e) {
            rangeList.emplace_back(s, e);
        }
    }

    // arrange and combine integer pairs in the range list
    void arrange() {
        if (rangeList.empty()) {
            return;
        }
        sort(rangeList.begin(), rangeList.end());
        std::vector<std::pair<int, int>> newRangeList;
        size_t size = rangeList.size();
        auto& r = rangeList[0];
        for (int i = 1; i < size; i++) {
            auto& _r = rangeList[i];
            if (_r.first <= r.second + 1) {
                r.second = std::max(r.second, _r.second);
            } else {
                newRangeList.push_back(r);
                r = _r;
            }
        }
        newRangeList.push_back(r);
        rangeList = newRangeList;
    }

    // change Range to the complement of Range
    void complement() {
        Range newRange;
        int ele = LOWER_BOUND;
        for (auto& r : rangeList) {
            newRange.insert(ele, r.first - 1);
            ele = r.second + 1;
        }
        newRange.insert(ele, UPPER_BOUND);
        newRange.arrange();
        rangeList = newRange.getRangeList();
    }

public:
    Range() = default;

    Range(int s, int e) {
        this->insert(s, e);
    }

    ~Range() = default;

    // print Range
    void print() {
        for (auto& r : rangeList) {
            std::cout << "[" << r.first << ", " << r.second << "]";
        }
    }

    // set Range = Range v _Range
    // return true if Range is changed
    bool range_union(Range& _range) {
        Range newRange = *this;
        for (auto& _r : _range.getRangeList()) {
            newRange.insert(_r.first, _r.second);
        }
        newRange.arrange();
        if (!this->range_equal(newRange)) {
            rangeList = newRange.getRangeList();
            return true;
        }
        return false;
    }

    // set Range = Range ^ _Range
    // return true if Range is changed
    bool range_join(Range& _range) {
        if (this->is_empty()) {
            return false;
        }
        Range newRange;
        int idx = 0;
        for (auto& _r : _range.getRangeList()) {
            while (idx < rangeList.size() && rangeList[idx].second < _r.first) {
                idx++;
            }
            while (idx < rangeList.size() && rangeList[idx].second <= _r.second) {
                auto& r = rangeList[idx];
                newRange.insert(std::max(_r.first, r.first), std::min(_r.second, r.second));
                idx++;
            }
            if (idx < rangeList.size()) {
                auto& r = rangeList[idx];
                newRange.insert(std::max(_r.first, r.first), std::min(_r.second, r.second));
            }
        }
        newRange.arrange();
        if (!this->range_equal(newRange)) {
            rangeList = newRange.getRangeList();
            return true;
        }
        return false;
    }

    // set Range = Range - _Range
    // return true if Range is changed
    bool range_subtract(Range& _range) {
        if (this->is_empty()) {
            return false;
        }
        Range newRange = _range;
        newRange.complement();
        newRange.range_join(*this);
        newRange.arrange();
        if (!this->range_equal(newRange)) {
            rangeList = newRange.getRangeList();
            return true;
        }
        return false;
    }

    // extend Range by an integer
    void range_add(int c) {
        for (auto& r : rangeList) {
            r.first = std::min(r.first + c, UPPER_BOUND);
            r.second = std::min(r.second + c, UPPER_BOUND);
        }
        this->arrange();
    }

    // extend Range by _Range
    void range_add(Range& _range) {
        for (auto& _r : _range.getRangeList()) {
            for (auto& r : rangeList) {
                r.first = std::min(r.first + _r.first, UPPER_BOUND);
                r.second = std::min(r.second + _r.second, UPPER_BOUND);
            }
        }
        this->arrange();
    }

    // reduce Range by an integer
    void range_minus(int c) {
        for (auto& r : rangeList) {
            r.first = std::max(r.first - c, LOWER_BOUND);
            r.second = std::min(r.second - c, LOWER_BOUND);
        }
        this->arrange();
    }

    // reduce Range by _Range
    void range_minus(Range& _range) {
        for (auto& _r : _range.getRangeList()) {
            for (auto& r : rangeList) {
                r.first = std::max(r.first - _r.second, LOWER_BOUND);
                r.second = std::max(r.second - _r.first, LOWER_BOUND);
            }
        }
        this->arrange();
    }

    // compare Range with _Range
    // return true if result is equal
    bool range_equal(Range& _range) {
        size_t rlSize = rangeList.size();
        auto _rl = _range.getRangeList();
        if (rlSize != _rl.size()) {
            return false;
        }
        for (int i = 0; i < rlSize; i++) {
            if (rangeList[i].first != _rl[i].first || rangeList[i].second != _rl[i].second) {
                return false;
            }
        }
        return true;
    }

    // return true if Range is a subset of _Range
    bool is_subset_of(Range& _range) {
        int _idx = 0;
        auto _rl = _range.getRangeList();
        for (auto& r : rangeList) {
            while (_idx < _rl.size() && _rl[_idx].second < r.first) {
                _idx++;
            }
            if (_idx == _rl.size() || _rl[_idx].first > r.first || _rl[_idx].second < r.second) {
                return false;
            }
        }
        return true;
    }

    // return true if Range is an empty set
    bool is_empty() {
        return rangeList.empty();
    }
};

/**
 * Range of multiple variables
*/
class VarRange {
private:
    std::unordered_map<std::string, Range> varRange;

    bool containVar(const std::string& var) {
        return varRange.find(var) != varRange.end();
    }

public:
    VarRange() = default;

    ~VarRange() = default;

    // print Range of all variables
    void print() {
        for (auto& vr : varRange) {
            std::cout << vr.first << ": ";
            vr.second.print();
            std::cout << " ";
        }
    }

    // get the Range of a variable
    Range getVar(const std::string& var) {
        if (!this->containVar(var)) {
            return Range();
        }
        return varRange[var];
    }

    // get all variables
    std::unordered_set<std::string> getVarSet() {
        std::unordered_set<std::string> res;
        for (auto& vr : varRange) {
            res.emplace(vr.first);
        }
        return res;
    }

    // insert or replace the Range of a variable
    void insertVar(const std::string& var, Range range) {
        if (!range.is_empty()) {
            varRange[var] = range;
        }
    }

    // union the Range of each variable
    // return true if VarRange is changed
    bool range_union(VarRange& _varRange) {
        bool changed = false;
        auto _varSet = _varRange.getVarSet();
        for (auto& _v : _varSet) {
            if (this->containVar(_v)) {
                auto _range = _varRange.getVar(_v);
                changed = changed | varRange[_v].range_union(_range);
            } else {
                this->insertVar(_v, _varRange.getVar(_v));
                changed = true;
            }
        }
        return changed;
    }

    // join the Range of each variable
    // return true if VarRange is changed
    bool range_join(VarRange& _varRange) {
        bool changed = false;
        auto _varSet = _varRange.getVarSet();
        for (auto& _v : _varSet) {
            if (this->containVar(_v)) {
                auto _range = _varRange.getVar(_v);
                changed |= varRange[_v].range_join(_range);
            }
        }
        return changed;
    }

    // subtract the Range of each variable
    // return true if VarRange is changed
    bool range_subtract(VarRange& _varRange) {
        bool changed = false;
        auto _varSet = _varRange.getVarSet();
        for (auto& _v : _varSet) {
            if (this->containVar(_v)) {
                auto _range = _varRange.getVar(_v);
                changed |= varRange[_v].range_subtract(_range);
            }
        }
        return changed;
    }

    // return true if VarRange is equal to _VarRange
    bool range_equal(VarRange& _varRange) {
        auto varSet = _varRange.getVarSet();
        for (auto& _v : varSet) {
            if (!this->containVar(_v)) {
                return false;
            }
            auto _range = _varRange.getVar(_v);
            if (!varRange[_v].range_equal(_range)) {
                return false;
            }
        }
        return true;
    }
};

/**
 * Information of branch IR (Goto & If)
*/
class BranchInfo {
private:
    VarRange jumpRange;     // Range of variables when this IR jump
    std::string condX;      // Condition variable
    Range condRange;        // Range of condition

public:
    BranchInfo() = default;

    BranchInfo(const std::string& x, int s, int e) {
        condX = x;
        condRange = Range(s, e);
    }

    ~BranchInfo() = default;

    VarRange getJumpRange() {
        return jumpRange;
    }

    bool setGotoRange(VarRange& currRange) {
        bool changed = false;
        changed = !jumpRange.range_equal(currRange);
        if (changed) {
            jumpRange = currRange;
        }
        currRange = VarRange();
        return changed;
    }

    bool setIfRange(VarRange& currRange) {
        bool changed = false;

        auto jxRange = jumpRange.getVar(condX);
        auto newJXRange = currRange.getVar(condX);
        newJXRange.range_join(condRange);
        if (newJXRange.is_empty()) {
            if (!jumpRange.getVarSet().empty()) {
                jumpRange = VarRange();
                changed = true;
            }
        } else if (!jxRange.range_equal(newJXRange)) {
            auto tmp = currRange;
            tmp.insertVar(condX, newJXRange);
            jumpRange = tmp;
            changed = true;
        }

        auto newXRange = currRange.getVar(condX);
        newXRange.range_subtract(newJXRange);
        if (newXRange.is_empty()) {
            if (!currRange.getVarSet().empty()) {
                currRange = VarRange();
            }
        } else {
            auto tmp = VarRange();
            tmp.insertVar(condX, condRange);
            tmp.range_join(currRange);
            currRange.range_subtract(tmp);
        }

        return changed;
    }
};

/**
 * Information of CheckInterval IR
*/
class CheckInfo {
private:
    fdlang::IR::CheckIntervalInst* inst;    // This CheckInterval IR
    Range checkRange;                       // Variable range this IR expected
    Range realRange;                        // Variable range when this IR is executed 

public:
    CheckInfo(fdlang::IR::CheckIntervalInst* inst, int s, int e) {
        this->inst = inst;
        checkRange = Range(s, e);
    }

    ~CheckInfo() = default;

    fdlang::IR::CheckIntervalInst* getInst() {
        return inst;
    }

    void updateRealRange(VarRange& currRange) {
        realRange = currRange.getVar(inst->getOperand(0)->getAsVariable());
    }

    ResultType getResultType() {
        if (realRange.is_empty()) {
            return ResultType::UNREACHABLE;
        }
        if (realRange.is_subset_of(checkRange)) {
            return ResultType::YES;
        }
        return ResultType::NO;
    }
};

class IntervalAnalysis : public DataflowAnalysis {
private:
    std::map<IR::CheckIntervalInst *, ResultType> results;

public:
    IntervalAnalysis(const IR::Insts &insts) : DataflowAnalysis(insts) {}

    // DO NOT MODIFY THIS FUNCTION
    void dumpResult(std::ostream &out) override {
        using Location = std::pair<size_t, size_t>;

        std::vector<std::pair<Location, ResultType>> ans;
        for (auto [checkInst, result] : results) {
            ans.emplace_back(
                (Location){checkInst->getLine(), checkInst->getLabel()},
                result);
        }
        std::sort(
            ans.begin(), ans.end(),
            [](const std::pair<Location, ResultType> &x,
               const std::pair<Location, ResultType> &y) { return x < y; });

        for (auto [loc, result] : ans) {
            auto [line, label] = loc;
            out << "Line " << line << ": ";
            if (result == ResultType::UNREACHABLE) {
                out << "Unreachable" << std::endl;
                continue;
            }
            out << (result == ResultType::YES ? "YES" : " NO") << std::endl;
        }
    }

    void run() override;

private:
    /**
     * Your code starts here
     */

    // Analysis structures
    std::unordered_map<int, int> targetToJump;          // Jump target to jump IR
    std::unordered_map<int, BranchInfo*> branchInfos;   // Jump IR label to branch info
    std::unordered_map<int, CheckInfo*> checkInfos;     // CheckInterval IR label to check info
    std::unordered_set<std::string> vars;               // All variables
    VarRange currRange;                                 // Current range of all variables
    
    // Analysis passes
    void init();                                        // Prepare analysis structures
    void iter();                                        // Iterate util branchInfos stable
    void done();                                        // Write results and tear down
};

} // namespace fdlang::analysis
#endif