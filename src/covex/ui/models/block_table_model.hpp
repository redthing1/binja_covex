#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <vector>

namespace binja::covex::ui {

struct BlockRow {
  uint64_t address = 0;
  uint32_t size = 0;
  uint64_t hits = 0;
  QString function;
};

class BlockTableModel final : public QAbstractTableModel {
public:
  enum Column { Address = 0, Size = 1, Hits = 2, Function = 3, ColumnCount };

  explicit BlockTableModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;

  void set_blocks(std::vector<BlockRow> rows);
  const BlockRow *block_at(int row) const;

private:
  std::vector<BlockRow> rows_;
};

} // namespace binja::covex::ui
