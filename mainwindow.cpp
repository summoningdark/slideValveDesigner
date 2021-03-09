#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _engine = new SlideValveEngine();

    // coloring brushes for cycle regions
    _regionBrush[CycleEnum::intake] = QBrush(QColor(0, 255, 0, 255));       // color for intake
    _regionBrush[CycleEnum::expansion] = QBrush(QColor(100, 255, 100, 255));   // color for expansion
    _regionBrush[CycleEnum::exahust] = QBrush(QColor(0, 0, 255, 255));       // color for exahust
    _regionBrush[CycleEnum::compression] = QBrush(QColor(255, 0, 0, 255));       // color for compression

    _thickPen = QPen(Qt::black, 3);

    _settingsOK = true;

    // enable user interaction with the plots (panning and zoomin)
    ui->cyclePlot->setInteraction(QCP::iRangeDrag, true);
    ui->cyclePlot->setInteraction(QCP::iRangeZoom, true);
    ui->cyclePlot->xAxis->setLabel("Crank Position (degrees)");
    ui->cyclePlot->yAxis->setLabel("Piston Offset from TDC ");

    drawCycleDiagram();
    updateAnimationDiagram();
}

void MainWindow::drawCycleDiagram()
{
    ui->cyclePlot->clearGraphs();
    ui-> cyclePlot->clearItems();

    if (_settingsOK)
    {
        double stroke = _engine->getEngineParams().stroke;        

        // draw shaded regions from <stroke> to the X-Axis for the return stroke regions
        // these will be over-drawn by the position curve for the forward stroke
        int graphIndex = -1;            // for counting graphs
        double crankPos = -M_PI;
        double crankStop = 3 * M_PI;

        while (crankPos < crankStop)
        {
            // make graph from crank position to next critical point (or end whichever comes first)
            ui->cyclePlot->addGraph();      // add graph
            graphIndex++;                   // count it
            // set the fill color based on region
            CycleEnum cycle = _engine->crankRad2Cycle(crankPos, true);
            double nextCrankPos = _engine-> nextCycleRad(crankPos, true);
            if (nextCrankPos > crankStop)
               nextCrankPos = crankStop;
            ui->cyclePlot->graph(graphIndex)->setBrush(_regionBrush[cycle]);
            QVector<double> rX(2), rY(2);
            rX[0] = SVE::rad2Deg(crankPos);
            rY[0] = stroke;
            rX[1] = SVE::rad2Deg(nextCrankPos);
            rY[1] = stroke;
            ui->cyclePlot->graph(graphIndex)->setData(rX, rY);
            crankPos = nextCrankPos;
        }

        ui->cyclePlot->addLayer("fStroke");         // add a layer so the next graph go over the previous ones
        ui->cyclePlot->setCurrentLayer("fStroke");  // set the layer as active

        crankPos = -M_PI;
        while (crankPos < crankStop)
        {
            // make graph from crank position to next critical point (or end whichever comes first)
            ui->cyclePlot->addGraph();      // add graph
            graphIndex++;                   // count it
            // set the fill color based on region
            CycleEnum cycle = _engine->crankRad2Cycle(crankPos, false);
            double nextCrankPos = _engine->nextCycleRad(crankPos, false);
            if (nextCrankPos > crankStop)
                nextCrankPos = crankStop;
            ui->cyclePlot->graph(graphIndex)->setPen(_thickPen);
            ui->cyclePlot->graph(graphIndex)->setBrush(_regionBrush[cycle]);         
            QVector<double> fX, fY;
            fX.append(SVE::rad2Deg(crankPos));
            fY.append(_engine->crankRad2Stroke(crankPos));
            for (int step=0; crankPos < nextCrankPos; step++)
            {
                crankPos += .01;                // step the position
                if (crankPos > nextCrankPos)    // check for overshoot
                    crankPos = nextCrankPos;
                fX.append(SVE::rad2Deg(crankPos));
                fY.append(_engine->crankRad2Stroke(crankPos));
            }
            ui->cyclePlot->graph(graphIndex)->setData(fX, fY);
        }
        ui->cyclePlot->rescaleAxes();
    }
    else
    {
        // todo put text on plot indicating invalid settings
    }
    ui->cyclePlot->replot();
}



void MainWindow::updateAnimationDiagram()
{

}

void MainWindow::updateEngineSettings()
{

    s_engineParams newSettings;
    newSettings.bore = 1;
    newSettings.stroke = ui->stroke->value();
    newSettings.conRod = ui->conRod->value();
    newSettings.valveTravel = ui->valveTravel->value();
    newSettings.valveConRod = ui->valveConRod->value();
    newSettings.eccentricAdvance = ui->eccentricAdvance->value() * M_PI / 180.0;
    newSettings.valvePorts.topPort[0] = (ui->steamPortSpace->value() - ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.botPort[0] = -(ui->steamPortSpace->value() - ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.exPort[0] = -ui->exahustPortWidth->value() / 2;
    newSettings.valvePorts.topPort[1] = (ui->steamPortSpace->value() + ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.botPort[1] = -(ui->steamPortSpace->value() - ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.exPort[1] = ui->exahustPortWidth->value() / 2;
    newSettings.valveSlide.topLand[0] = ui->valveWidth->value()/2 - ui->valveTopLand->value();
    newSettings.valveSlide.botLand[0] = -(ui->valveWidth->value()/2) + ui->valveBottomLand->value();;
    newSettings.valveSlide.topLand[1] = ui->valveWidth->value()/2;
    newSettings.valveSlide.botLand[1] = -(ui->valveWidth->value()/2);

    ErrorEnum ret = _engine->setEngineParams(newSettings);
    if (ret == ErrorEnum::none)
    {
        _settingsOK = true;
    }
    else
    {
        // Todo: error dialog
        _settingsOK = false;
    }
    drawCycleDiagram();
    updateAnimationDiagram();
}

void MainWindow::setAnimationSlider()
{
    updateAnimationDiagram();
}

void MainWindow::squarePlotY(QCustomPlot *plot)
{
    plot->yAxis->setScaleRatio(plot->xAxis,1.0);
    plot->replot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

