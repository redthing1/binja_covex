#include "covex/ui/models/block_table_model.hpp"

#include <QString>

namespace binja::covex::ui {

BlockTableModel::BlockTableModel(QObject *parent)
    : QAbstractTableModel(parent) {}

int BlockTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(rows_.size());
}

int BlockTableModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return ColumnCount;
}

QVariant BlockTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return {};
  }
  if (index.row() < 0 || index.row() >= static_cast<int>(rows_.size())) {
    return {};
  }
  const auto &row = rows_[static_cast<size_t>(index.row())];
  if (role == Qt::DisplayRole) {
    switch (index.column()) {
    case Address:
      return QString("0x%1").arg(static_cast<qulonglong>(row.address), 0, 16);
    case Size:
      return static_cast<qulonglong>(row.size);
    case Hits:
      return static_cast<qulonglong>(row.hits);
    case Function:
      return row.function;
    default:
      break;
    }
  }
  return {};
}

QVariant BlockTableModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
  if (role != Qt::DisplayRole) {
    return {};
  }
  if (orientation == Qt::Horizontal) {
    switch (section) {
    case Address:
      return QString("Address");
    case Size:
      return QString("Size");
    case Hits:
      return QString("Hits");
    case Function:
      return QString("Function");
    default:
      break;
    }
  }
  return {};
}

void BlockTableModel::set_blocks(std::vector<BlockRow> rows) {
  beginResetModel();
  rows_ = std::move(rows);
  endResetModel();
}

const BlockRow *BlockTableModel::block_at(int row) const {
  if (row < 0 || row >= static_cast<int>(rows_.size())) {
    return nullptr;
  }
  return &rows_[static_cast<size_t>(row)];
}

} // namespace binja::covex::ui
