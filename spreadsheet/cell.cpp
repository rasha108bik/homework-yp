#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <optional>
#include <stack>

class Cell::Impl {
public:
    virtual ~Impl() = default;

    virtual Value GetValue() const = 0;

    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const {
        return {};
    }

    virtual bool IsCacheValid() const {
        return true;
    }

    virtual void InvalidateCache() {}
};

class Cell::EmptyImpl : public Impl {
public:
    Value GetValue() const override {
        return "";
    }

    std::string GetText() const override {
        return "";
    }
};

class Cell::TextImpl : public Impl {
public:
    explicit TextImpl(std::string text) : text_(std::move(text)) {
        if (text_.empty()) {
            throw std::logic_error(
                    "TextImpl should not contain empty text, use EmptyImpl for this "
                    "purpose.");
        }
    }

    Value GetValue() const override {
        if (text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    explicit FormulaImpl(std::string text, const SheetInterface &sheet) : sheet_(sheet) {
        if (text.empty() || text[0] != FORMULA_SIGN) {
            throw std::logic_error("A formula should start with '=' sign");
        }
        formula_ = ParseFormula(text.substr(1));
    }

    Value GetValue() const override {
        if (!cachedValue_) {
            cachedValue_ = formula_->Evaluate(sheet_);
        }
        return std::visit([](const auto &x) { return Value(x); }, *cachedValue_);
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

    bool IsCacheValid() const override {
        return cachedValue_.has_value();
    }

    void InvalidateCache() override {
        cachedValue_.reset();
    }

private:
    const SheetInterface &sheet_;
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<FormulaInterface::Value> cachedValue_;
};

Cell::Cell(Sheet &sheet)
    : impl_(std::make_unique<EmptyImpl>()),
    sheet_(sheet) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> newImpl;
    if (text.empty()) {
        newImpl = std::make_unique<EmptyImpl>();
    } else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        newImpl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    } else {
        newImpl = std::make_unique<TextImpl>(std::move(text));
    }

    if (WouldIntroduceCircularDependency(*newImpl)) {
        throw CircularDependencyException(
                "Setting this formula would introduce circular dependency!");
    }

    impl_ = std::move(newImpl);

    UpdateRefs();
    InvalidateCacheRecursive(true);
}

void Cell::Clear() {
    Set("");
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::WouldIntroduceCircularDependency(const Impl &newImpl) const {
    if (newImpl.GetReferencedCells().empty()) {
        return false;
    }

    std::unordered_set<const Cell *> referenced;
    for (const auto &pos : newImpl.GetReferencedCells()) {
        referenced.insert(sheet_.GetConcreteCell(pos));
    }

    std::unordered_set<const Cell *> visited;
    std::stack<const Cell *> toVisit;
    toVisit.push(this);
    while (!toVisit.empty()) {
        const Cell *current = toVisit.top();
        toVisit.pop();
        visited.insert(current);
        if (referenced.find(current) != referenced.end()) {
            return true;
        }
        for (const Cell *incoming : current->inRefs_) {
            if (visited.find(incoming) == visited.end()) {
                toVisit.push(incoming);
            }
        }
    }

    return false;
}

void Cell::UpdateRefs() {
    for (Cell *outgoing : outRefs_) {
        outgoing->inRefs_.erase(this);
    }
    outRefs_.clear();
    for (const auto &pos : impl_->GetReferencedCells()) {
        Cell *outgoing = sheet_.GetConcreteCell(pos);
        if (!outgoing) {
            sheet_.SetCell(pos, "");
            outgoing = sheet_.GetConcreteCell(pos);
        }
        outRefs_.insert(outgoing);
        outgoing->inRefs_.insert(this);
    }
}

void Cell::InvalidateCacheRecursive(bool force) {
    if (impl_->IsCacheValid() || force) {
        impl_->InvalidateCache();
        for (Cell *incoming : inRefs_) {
            incoming->InvalidateCacheRecursive();
        }
    }
}