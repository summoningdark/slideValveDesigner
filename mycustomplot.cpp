#include "mycustomplot.h"


void myCustomPlot::resizeEvent (QResizeEvent *event)
{
    QCustomPlot::resizeEvent(event);
    emit Resized(this);
};
