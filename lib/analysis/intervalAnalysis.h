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
    std::vector<std::pair<int, int>> range;

    std::vector<std::pair<int, int>> getRange() {
        return range;
    }

    void add(int s, int e) {
        s = std::max(s, LOWER_BOUND);
        e = std::min(e, UPPER_BOUND);
        if (s <= e) {
            range.emplace_back(s, e);
        }
    }

    void complement() {
        Range newRange;
        int ele = LOWER_BOUND;
        for (auto& r : range) {
            newRange.add(ele, r.first - 1);
            ele = r.second + 1;
        }
        newRange.add(ele, UPPER_BOUND);
        newRange.arrange();
        range = newRange.getRange();
    }

    void arrange() {
        if (range.empty()) {
            return;
        }
        sort(range.begin(), range.end());
        std::vector<std::pair<int, int>> newRange;
        size_t size = range.size();
        auto& r = range[0];
        for (int i = 1; i < size; i++) {
            auto& _r = range[i];
            if (_r.first <= r.second + 1) {
                r.second = std::max(r.second, _r.second);
            } else {
                newRange.push_back(r);
                r = _r;
            }
        }
        newRange.push_back(r);
        range = newRange;
    }

public:
    Range() = default;

    Range(int s, int e) {
        this->add(s, e);
    }

    ~Range() = default;

    // debug
    void print() {
        for (auto& r : range) {
            std::cout << "[" << r.first << ", " << r.second << "]";
        }
    }
    // debug

    // set range = range v _range
    // return true if range is changed
    bool range_union(Range& _range) {
        Range newRange = *this;
        for (auto& _r : _range.getRange()) {
            newRange.add(_r.first, _r.second);
        }
        newRange.arrange();
        if (!this->range_equal(newRange)) {
            range = newRange.getRange();
            return true;
        }
        return false;
    }

    // set range = range ^ _range
    // return true if range is changed
    bool range_join(Range& _range) {
        if (this->is_empty()) {
            return false;
        }
        Range newRange;
        int idx = 0;
        for (auto& _r : _range.getRange()) {
            while (idx < range.size() && range[idx].second < _r.first) {
                idx++;
            }
            while (idx < range.size() && range[idx].second <= _r.second) {
                auto& r = range[idx];
                newRange.add(std::max(_r.first, r.first), std::min(_r.second, r.second));
                idx++;
            }
            if (idx < range.size()) {
                auto& r = range[idx];
                newRange.add(std::max(_r.first, r.first), std::min(_r.second, r.second));
            }
        }
        newRange.arrange();
        if (!this->range_equal(newRange)) {
            range = newRange.getRange();
            return true;
        }
        return false;
    }

    // set range = range - _range
    // return true if range is changed
    bool range_subtract(Range& _range) {
        if (this->is_empty()) {
            return false;
        }
        Range newRange = _range;
        newRange.complement();
        newRange.range_join(*this);
        newRange.arrange();
        if (!this->range_equal(newRange)) {
            range = newRange.getRange();
            return true;
        }
        return false;
    }

    // extend range by an integer
    void range_add(int c) {
        for (auto& r : range) {
            r.first = std::min(r.first + c, 255);
            r.second = std::min(r.second + c, 255);
        }
        arrange();
    }

    // extend range by a range
    void range_add(Range& _range) {
        for (auto& _r : _range.getRange()) {
            for (auto& r : range) {
                r.first = std::min(r.first + _r.first, 255);
                r.second = std::min(r.second + _r.second, 255);
            }
        }
        arrange();
    }

    // reduce range by an integer
    void range_minus(int c) {
        for (auto& r : range) {
            r.first = std::max(r.first - c, 0);
            r.second = std::min(r.second - c, 0);
        }
        arrange();
    }

    // reduce range by a range
    void range_minus(Range& _range) {
        for (auto& _r : _range.getRange()) {
            for (auto& r : range) {
                r.first = std::max(r.first - _r.second, 0);
                r.second = std::max(r.second - _r.first, 0);
            }
        }
        arrange();
    }

    // compare range and _range
    // return true if range is equal to _range
    bool range_equal(Range& _range) {
        size_t rSize = range.size();
        auto _r = _range.getRange();
        if (rSize != _r.size()) {
            return false;
        }
        for (int i = 0; i < rSize; i++) {
            if (range[i].first != _r[i].first || range[i].second != _r[i].second) {
                return false;
            }
        }
        return true;
    }

    // return true if range is a subset of _range
    bool is_subset_of(Range& _range) {
        int _idx = 0;
        auto _r = _range.getRange();
        for (auto& r : range) {
            while (_idx < _r.size() && _r[_idx].second < r.first) {
                _idx++;
            }
            if (_idx == _r.size() || _r[_idx].first > r.first || _r[_idx].second < r.second) {
                return false;
            }
        }
        return true;
    }

    // return true if range is an empty set
    bool is_empty() {
        return range.empty();
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

    // debug
    void print() {
        for (auto& vr : varRange) {
            std::cout << vr.first << ": ";
            vr.second.print();
            std::cout << " ";
        }
    }
    // debug

    Range getVar(const std::string& var) {
        return varRange[var];
    }

    std::unordered_set<std::string> getVarSet() {
        std::unordered_set<std::string> res;
        for (auto& vr : varRange) {
            res.emplace(vr.first);
        }
        return res;
    }

    void insertVar(const std::string& var, Range range) {
        if (!range.is_empty()) {
            varRange[var] = range;
        }
    }

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

    bool range_join(VarRange& _varRange) {
        bool changed = false;
        auto _varSet = _varRange.getVarSet();
        for (auto& _v : _varSet) {
            if (this->containVar(_v)) {
                auto _range = _varRange.getVar(_v);
                changed = changed | varRange[_v].range_join(_range);
            }
        }
        return changed;
    }

    bool range_subtract(VarRange& _varRange) {
        bool changed = false;
        auto _varSet = _varRange.getVarSet();
        for (auto& _v : _varSet) {
            if (this->containVar(_v)) {
                auto _range = _varRange.getVar(_v);
                changed = changed | varRange[_v].range_subtract(_range);
            }
        }
        return changed;
    }

    bool range_equal(VarRange& _varRange) {
        auto varSet = _varRange.getVarSet();
        for (auto& _v : varSet) {
            if (!this->containVar(_v)) {
                return false;
            }
            auto _vr = _varRange.getVar(_v);
            if (!varRange[_v].range_equal(_vr)) {
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

    BranchInfo(std::string x, int s, int e) {
        condX = std::move(x);
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

        // std::cout << "New Jump Range: " << std::endl;
        // jumpRange.print();

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

        // std::cout << "New No Jump Range: " << std::endl;
        // currRange.print();

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