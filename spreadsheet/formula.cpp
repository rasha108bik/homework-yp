#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
        case Category::Ref:
            return "#REF!";
        case Category::Value:
            return "#VALUE!";
        case Category::Div0:
            return "#DIV/0!";
    }
    return "";
}

namespace {
    double GetDoubleFrom(const std::string &str) {
        double value = 0;
        if (!str.empty()) {
            std::istringstream in(str);
            if (!(in >> value) || !in.eof()) {
                throw FormulaError(FormulaError::Category::Value);
            }
        }
        return value;
    }

    double GetDoubleFrom(double value) {
        return value;
    }

    double GetDoubleFrom(FormulaError error) {
        throw FormulaError(error);
    }

    double GetCellValue(const CellInterface *cell) {
        if (!cell) return 0;
        return std::visit([](const auto &value) { return GetDoubleFrom(value); },
                          cell->GetValue());
    }
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
    // Реализуйте следующие методы:
    explicit Formula(const std::string& expression) try
    : ast_(ParseFormulaAST(expression)) {
    } catch (const std::exception &exc) {
        std::throw_with_nested(FormulaException(exc.what()));
    }

    Value Evaluate(const SheetInterface& sheetInterface) const override {
        try {
            auto cell_func = [&sheetInterface](Position position) -> double {
                if (!position.IsValid()) {
                    throw FormulaError(FormulaError::Category::Ref);
                }
                const auto *cell = sheetInterface.GetCell(position);
                return GetCellValue(cell);
            };

            return ast_.Execute(cell_func);
        } catch (FormulaError& error) {
            return error;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells;
        for (auto cell : ast_.GetCells()) {
            if (cell.IsValid()) {
                cells.push_back(cell);
            }
        }

        // cells are already sorted, but some might be duplicates
        cells.resize(std::unique(cells.begin(), cells.end()) - cells.begin());
        return cells;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}