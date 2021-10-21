#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {
    // Сначала очистим ячейки во избежания возникновения висячих указателей в
    // процессе удаления. Критической необходимости в этом нет, но так безопаснее.
    for (auto &row : cells_) {
        for (auto &cell : row) {
            if (cell) {
                cell->Clear();
            }
        }
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position passed to Sheet::SetCell()"s);
    }
    ResizeToIncludePosition(pos);
    auto &cell = cells_[pos.row][pos.col];
    if (!cell) {
        cell = std::make_unique<Cell>(*this);
    }
    cell->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetConcreteCell(pos);
}
CellInterface* Sheet::GetCell(Position pos) {
    return GetConcreteCell(pos);
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position passed to Sheet::GetCell()"s);
    }
    if (pos.row >= static_cast<int>(cells_.size()) || pos.col >= static_cast<int>(cells_[pos.row].size())) {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

Cell* Sheet::GetConcreteCell(Position pos) {
    return const_cast<Cell *>(
            static_cast<const Sheet &>(*this).GetConcreteCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position passed to Sheet::ClearCell()"s);
    }
    if (pos.row < static_cast<int>(cells_.size()) && pos.col < static_cast<int>(cells_[pos.row].size())) {
        if (auto &cell = cells_[pos.row][pos.col]) {
            cell->Clear();
            cell.reset();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size size;
    for (int row = 0; row < static_cast<int>(cells_.size()); ++row) {
        for (int col = static_cast<int>(cells_[row].size()) - 1; col >= 0; --col) {
            if (const auto &cell = cells_[row][col]) {
                if (!cell->GetText().empty()) {
                    size.rows = std::max(size.rows, row + 1);
                    size.cols = std::max(size.cols, col + 1);
                    break;
                }
            }
        }
    }
    return size;
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintCells(output, [&](const CellInterface &cell) {
        std::visit([&](const auto &value) { output << value; }, cell.GetValue());
    });
}
void Sheet::PrintTexts(std::ostream& output) const {
    PrintCells(output,
               [&output](const CellInterface &cell) { output << cell.GetText(); });
}

void Sheet::PrintCells(
        std::ostream &output,
        const std::function<void(const CellInterface &)> &printCell) const {
    auto size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            if (col < static_cast<int>(cells_[row].size())) {
                if (const auto &cell = cells_[row][col]) {
                    printCell(*cell);
                }
            }
        }
        output << '\n';
    }
}

void Sheet::ResizeToIncludePosition(Position pos) {
    cells_.resize(std::max(pos.row + 1, static_cast<int>(cells_.size())));
    cells_[pos.row].resize(std::max(pos.col + 1, static_cast<int>(cells_[pos.row].size())));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}