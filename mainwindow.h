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
        void sliderChanged(int value);
        void currentAngleChanged(double value);
        void setCriticalPoint(int value);
        void squarePlot(QCustomPlot *plot, QSize s);

private:
    Ui::MainWindow *ui;
    SlideValveEngine *_engine;
    double currentCrank_;                                   // holds current crank position for interactive plots
    CycleEnum currentTopCycle_;
    CycleEnum currentBotCycle_;
    double currentStroke_;
    double currentValvePos_;
    void drawCycleDiagram();
    std::map<CycleEnum, QBrush> _regionBrush;
    QPen _thickPen;
    bool _settingsOK;

    const std::map<CycleEnum, QString> cycleNames_{
                                                   {CycleEnum::compression, "Compression"},
                                                   {CycleEnum::exahust, "Exahust"},
                                                   {CycleEnum::expansion, "Expansion"},
                                                   {CycleEnum::intake, "intake"}
                                                  };
    void updateAnimationSlider(double deg);
    void updateCurrentAngle(double deg);
    void updateCriticalPoint(double deg);
    void setCurrentCrank(double deg);
    QCPDataContainer<QCPCurveData> drawSlide(double offset, double sizeParam);
    QCPDataContainer<QCPCurveData> drawCylinder1(double outsideEdge, double insideEdge, double bottomEdge, double topEdge, double portWall, double piston);                     // generates curve data for the outer part of the cylinder
    QCPDataContainer<QCPCurveData> drawCylinder2(double portWall, double insideEdge);                     // generates curve data for the inner part of the cylinder
    QCPDataContainer<QCPCurveData> drawForwardShade(double stroke, double insideEdge, double portWall, double cylinderBottom, double pistonWidth);     // draws a curve for shading the forward cylinder space given stroke (stroke is 0 at TDC)
    QCPDataContainer<QCPCurveData> drawReverseShade(double stroke, double insideEdge, double portWall, double cylinderBottom, double pistonWidth);     // draws a curve for shading the return cylinder space given stroke (stroke is 0 at TDC)
    QCPDataContainer<QCPCurveData> drawPiston(double stroke, double portWall, double cylinderBottom, double pistonWidth);
    QCPDataContainer<QCPCurveData> drawValveShade(double offset, double sizeParam);          // draws curve for shading the interior of the valve given valve position (position is 0 at valve neutral)
    QCPDataContainer<QCPCurveData> drawSteamChestShade(double insideEdge, double steamChestTop);               // draws a curve for shading the interior of the steam chest
    QCPDataContainer<QCPCurveData> drawExahustShade(double portWall);
};
#endif // MAINWINDOW_H
