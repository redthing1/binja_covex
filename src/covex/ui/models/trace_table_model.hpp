#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <vector>

namespace binja::covex::ui {

struct TraceRow {
  QString alias;
  QString name;
  uint64_t spans = 0;
  uint64_t unique_addresses = 0;
  uint64_t total_hits = 0;
  bool has_hitcounts = false;
};

class TraceTableModel final : public QAbstractTableModel {
public:
  enum Column {
    Alias = 0,
    Name = 1,
    Spans = 2,
    Addresses = 3,
    Hits = 4,
    ColumnCount
  };

  explicit TraceTableModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;

  void set_traces(std::vector<TraceRow> rows);
  const TraceRow *trace_at(int row) const;

private:
  std::vector<TraceRow> rows_;
};

} // namespace binja::covex::ui
