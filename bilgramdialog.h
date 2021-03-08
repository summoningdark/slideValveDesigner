#ifndef BILGRAMDIALOG_H
#define BILGRAMDIALOG_H

#include <QDialog>
#include "qcustomplot.h"

namespace Ui {
class BilgramDialog;
}

class BilgramDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BilgramDialog(QWidget *parent = nullptr);
    ~BilgramDialog();

    private slots:
    void drawCrankDiagram();
    void drawBilgramDiagram();

private:
    Ui::BilgramDialog *ui;
    void squarePlotY(QCustomPlot *plot);
};

#endif // BILGRAMDIALOG_H
