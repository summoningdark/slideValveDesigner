#ifndef MYCUSTOMPLOT_H
#define MYCUSTOMPLOT_H
#include "qcustomplot.h"

class myCustomPlot: public QCustomPlot
{
    Q_OBJECT
public:
    myCustomPlot(QWidget * parent = 0) : QCustomPlot(parent){};
protected:
    virtual void resizeEvent (QResizeEvent *event);

signals:
    void Resized(QCustomPlot *self, QSize s);
};

#endif // MYCUSTOMPLOT_H
