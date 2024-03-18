#ifndef FACTORYAREA_H
#define FACTORYAREA_H

#include <QFrame>
#include <QTableView>
#include <QHeaderView>
#include "FactorySlot/factoryslot.h"
#include "../equipmodel.h"
#include "../../Protocol/kp.h"

namespace Ui {
class FactoryArea;
}

namespace {
/* source: https://stackoverflow.com/questions/8766633/
 * how-to-determine-the-correct-size-of-a-qtablewidget */
static QSize tableSizeWhole(QTableView *view, EquipModel *model) {
    int w = view->verticalHeader()->width() + 4; // +4 seems to be needed
    for (int i = 0; i < model->columnCount(); i++)
        w += view->columnWidth(i); // seems to include gridline (on my machine)
    int h = view->horizontalHeader()->height() + 4;
    for (int i = 0; i < model->rowCount(); i++)
        h += view->rowHeight(i);
    return QSize(w, h);
}
}

class FactoryArea : public QFrame
{
    Q_OBJECT

public:
    explicit FactoryArea(QWidget *parent = nullptr);
    ~FactoryArea();

    void setDevelop(KP::FactoryState);
    void switchToDevelop();

signals:
    void rowCountHint(int);

public slots:
    void recalculateArsenalRows();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void arsenalFirst(bool checked = false);
    void arsenalPrev(bool checked = false);
    void arsenalNext(bool checked = false);
    void arsenalLast(bool checked = false);
    void developClicked(bool checked = false, int slotnum = 0);
    void doFactoryRefresh(const QJsonObject &);
    void updateArsenalEquip(const QJsonObject &);

private:
    EquipModel * getEquipModel();

    Ui::FactoryArea *ui;
    QTableView *arsenalView;

    KP::FactoryState factoryState = KP::Development;
    QList<FactorySlot *> slotfs;
};

#endif // FACTORYAREA_H
