#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet &sheet);
    ~Cell();

    void Set(std::string text);

    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    bool WouldIntroduceCircularDependency(const Impl &newImpl) const;
    void UpdateRefs();
    void InvalidateCacheRecursive(bool force = false);

    std::unique_ptr<Impl> impl_;
    Sheet &sheet_;
    std::unordered_set<Cell *> inRefs_;
    std::unordered_set<Cell *> outRefs_;
};