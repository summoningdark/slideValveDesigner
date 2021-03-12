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
        void updateEngineSettings();
        void drawValveDiagram();
        void setAnimationSlider(double deg);
        void sliderChanged(int value);
        void setCriticalPoint(int value);
        void squarePlot(QCustomPlot *plot, QSize s);

private:
    Ui::MainWindow *ui;
    SlideValveEngine *_engine;
    double currentCrank_;                                   // holds current crank position for interactive plots
    void drawCycleDiagram();
    std::map<CycleEnum, QBrush> _regionBrush;
    QPen _thickPen;
    bool _settingsOK;
    QCPDataContainer<QCPCurveData> drawSlide(double offset);
    QCPDataContainer<QCPCurveData> drawCylinder1();                     // generates curve data for the outer part of the cylinder
    QCPDataContainer<QCPCurveData> drawCylinder2();                     // generates curve data for the inner part of the cylinder
    QCPDataContainer<QCPCurveData> drawForwardShade(double stroke);     // draws a curve for shading the forward cylinder space given stroke (stroke is 0 at TDC)
    QCPDataContainer<QCPCurveData> drawReverseShade(double stroke);     // draws a curve for shading the return cylinder space given stroke (stroke is 0 at TDC)
    QCPDataContainer<QCPCurveData> drawPiston(double stroke);
    QCPDataContainer<QCPCurveData> drawValveShade(double offset);          // draws curve for shading the interior of the valve given valve position (position is 0 at valve neutral)
    QCPDataContainer<QCPCurveData> drawSteamChestShade();               // draws a curve for shading the interior of the steam chest
    QCPDataContainer<QCPCurveData> drawExahustShade();
};
#endif // MAINWINDOW_H
