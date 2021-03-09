#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include "qcustomplot.h"
#include "slidevalveengine.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    private slots:        
        void drawCycleDiagram();
        void updateEngineSettings();
        void updateAnimationDiagram();
        void setAnimationSlider();

private:
    Ui::MainWindow *ui;
    SlideValveEngine *_engine;
    std::map<CycleEnum, QBrush> _regionBrush;
    QPen _thickPen;
    bool _settingsOK;
    void squarePlotY(QCustomPlot *plot);
};
#endif // MAINWINDOW_H
