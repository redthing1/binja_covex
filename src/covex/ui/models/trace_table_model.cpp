#include "covex/ui/models/trace_table_model.hpp"

#include <QBrush>

namespace binja::covex::ui {

TraceTableModel::TraceTableModel(QObject *parent)
    : QAbstractTableModel(parent) {}

int TraceTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(rows_.size());
}

int TraceTableModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return ColumnCount;
}

QVariant TraceTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return {};
  }
  if (index.row() < 0 || index.row() >= static_cast<int>(rows_.size())) {
    return {};
  }
  const auto &row = rows_[static_cast<size_t>(index.row())];
  if (role == Qt::DisplayRole) {
    switch (index.column()) {
    case Alias:
      return row.alias;
    case Name:
      return row.name;
    case Spans:
      return static_cast<qulonglong>(row.spans);
    case Addresses:
      return static_cast<qulonglong>(row.unique_addresses);
    case Hits:
      if (row.has_hitcounts) {
        return static_cast<qulonglong>(row.total_hits);
      }
      return QString("-");
    default:
      break;
    }
  }
  return {};
}

QVariant TraceTableModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
  if (role != Qt::DisplayRole) {
    return {};
  }
  if (orientation == Qt::Horizontal) {
    switch (section) {
    case Alias:
      return QString("Alias");
    case Name:
      return QString("Trace");
    case Spans:
      return QString("Spans");
    case Addresses:
      return QString("Addrs");
    case Hits:
      return QString("Hits");
    default:
      break;
    }
  }
  return {};
}

void TraceTableModel::set_traces(std::vector<TraceRow> rows) {
  beginResetModel();
  rows_ = std::move(rows);
  endResetModel();
}

const TraceRow *TraceTableModel::trace_at(int row) const {
  if (row < 0 || row >= static_cast<int>(rows_.size())) {
    return nullptr;
  }
  return &rows_[static_cast<size_t>(row)];
}

} // namespace binja::covex::ui
