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
    _regionBrush[CycleEnum::exahust] = QBrush(QColor(100, 100, 255, 255));       // color for exahust
    _regionBrush[CycleEnum::compression] = QBrush(QColor(255, 0, 0, 255));       // color for compression

    _thickPen = QPen(Qt::black, 3);

    _settingsOK = true;

    // enable user interaction with the plots (panning and zoomin)
    ui->cyclePlot->setInteraction(QCP::iRangeDrag, true);
    ui->cyclePlot->setInteraction(QCP::iRangeZoom, true);
    ui->cyclePlot->xAxis->setLabel("Crank Position (degrees)");
    ui->cyclePlot->yAxis->setLabel("Piston Offset from TDC ");

    ui->valvePlot->setInteraction(QCP::iRangeDrag, true);
    ui->valvePlot->setInteraction(QCP::iRangeZoom, true);
    ui->valvePlot->xAxis->setLabel("Valve Position");

    connect(ui->valvePlot, SIGNAL(Resized(QCustomPlot*)), this, SLOT(squarePlotY(QCustomPlot*)));

    drawCycleDiagram();
    drawValveDiagram();
    updateAnimationDiagram();
}

void MainWindow::drawCycleDiagram()
{
    ui->cyclePlot->clearGraphs();
    ui->cyclePlot->clearItems();

    if (_settingsOK)
    {
        double stroke = _engine->getEngineParams().stroke;        

        // draw shaded regions from <stroke> to the X-Axis for the return stroke regions
        // these will be over-drawn by the position curve for the forward stroke
        int graphIndex = -1;            // for counting graphs
        double crankPos = -180;
        double crankStop = 440;

        while (crankPos < crankStop)
        {
            // make graph from crank position to next critical point (or end whichever comes first)
            ui->cyclePlot->addGraph();      // add graph
            graphIndex++;                   // count it
            // set the fill color based on region
            CycleEnum cycle = _engine->crank2Cycle(crankPos, true);
            double nextCrankPos = _engine-> nextCycle(crankPos, true);
            if (nextCrankPos > crankStop)
               nextCrankPos = crankStop;
            ui->cyclePlot->graph(graphIndex)->setBrush(_regionBrush[cycle]);
            QVector<double> rX(2), rY(2);
            rX[0] = crankPos;
            rY[0] = stroke;
            rX[1] = nextCrankPos;
            rY[1] = stroke;
            ui->cyclePlot->graph(graphIndex)->setData(rX, rY);
            crankPos = nextCrankPos;
        }       

        crankPos = -180;
        while (crankPos < crankStop)
        {
            // make graph from crank position to next critical point (or end whichever comes first)
            ui->cyclePlot->addGraph();      // add graph
            graphIndex++;                   // count it
            // set the fill color based on region
            CycleEnum cycle = _engine->crank2Cycle(crankPos, false);
            double nextCrankPos = _engine->nextCycle(crankPos, false);
            if (nextCrankPos > crankStop)
                nextCrankPos = crankStop;
            ui->cyclePlot->graph(graphIndex)->setPen(_thickPen);
            ui->cyclePlot->graph(graphIndex)->setBrush(_regionBrush[cycle]);         
            QVector<double> fX, fY;
            fX.append(crankPos);
            fY.append(_engine->crank2Stroke(crankPos));
            for (int step=0; crankPos < nextCrankPos; step++)
            {
                crankPos += .01;                // step the position
                if (crankPos > nextCrankPos)    // check for overshoot
                    crankPos = nextCrankPos;
                fX.append(crankPos);
                fY.append(_engine->crank2Stroke(crankPos));
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

void MainWindow::drawValveDiagram()
{
    ui->valvePlot->clearGraphs();
    ui->valvePlot->clearItems();
    ui->valvePlot->clearPlottables();

    if (_settingsOK)
    {
       s_engineParams params = _engine->getEngineParams();
       //make valve bridge blocks 1/4 total valve travel
       double portsY = params.valveTravel / 4;
       double portsX[2] = {params.valvePorts.botPort[1] + params.valveTravel, params.valvePorts.topPort[1] - params.valveTravel};
       QVector<double> fTermX(2), fBridgeX(2), rBridgeX(2), rTermX(2), Y(2);
       Y[0] = -portsY;
       Y[1] = -portsY;
       fTermX[0] = portsX[1]; // make nice and far from center
       fTermX[1] = params.valvePorts.topPort[1];

       fBridgeX[0] = params.valvePorts.topPort[0];
       fBridgeX[1] = params.valvePorts.exPort[1];

       rBridgeX[0] = params.valvePorts.exPort[0];
       rBridgeX[1] = params.valvePorts.botPort[0];

       rTermX[0] = params.valvePorts.botPort[1];
       rTermX[1] = portsX[0];

       ui->valvePlot->addGraph();
       ui->valvePlot->addGraph();
       ui->valvePlot->addGraph();
       ui->valvePlot->addGraph();
       ui->valvePlot->graph(0)->setData(fTermX, Y);
       ui->valvePlot->graph(1)->setData(fBridgeX, Y);
       ui->valvePlot->graph(2)->setData(rBridgeX, Y);
       ui->valvePlot->graph(3)->setData(rTermX, Y);

       ui->valvePlot->graph(0)->setBrush(QBrush(QColor(0,0,0,255)));
       ui->valvePlot->graph(1)->setBrush(QBrush(QColor(0,0,0,255)));
       ui->valvePlot->graph(2)->setBrush(QBrush(QColor(0,0,0,255)));
       ui->valvePlot->graph(3)->setBrush(QBrush(QColor(0,0,0,255)));

       // make path for the slide valve
       QVector<double> slideValveX(9), slideValveY(9);

      slideValveX[0] = params.valveSlide.topLand[1];
      slideValveY[0] = 0;
      slideValveX[1] = params.valveSlide.topLand[1];
      slideValveY[1] = 2*portsY;
      slideValveX[2] = params.valveSlide.botLand[1];
      slideValveY[2] = 2*portsY;
      slideValveX[3] = params.valveSlide.botLand[1];
      slideValveY[3] = 0;
      slideValveX[4] = params.valveSlide.botLand[0];
      slideValveY[4] = 0;
      slideValveX[5] = params.valveSlide.botLand[0];
      slideValveY[5] = portsY;
      slideValveX[6] = params.valveSlide.topLand[0];
      slideValveY[6] = portsY;
      slideValveX[7] = params.valveSlide.topLand[0];
      slideValveY[7] = 0;
      slideValveX[8] = params.valveSlide.topLand[1];
      slideValveY[8] = 0;

      // move slide to position
      double shift = 0;
      if (ui->valvePlotSlidePosition->currentIndex() == 1)
          shift = -params.valveTravel/2.0;
      else if (ui->valvePlotSlidePosition->currentIndex() == 2)
          shift = params.valveTravel/2.0;
      for (int i=0; i<8; i++){
          slideValveX[i] += shift;
      }

      // draw slide valve curve
      QCPCurve *slideValve = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
      slideValve->setData(slideValveX, slideValveY);
      slideValve->setBrush(QBrush(QColor(150,150,150,255)));

      ui->valvePlot->xAxis->setRange(portsX[1]-.25*params.valveTravel, portsX[0] +.25*params.valveTravel);
      ui->valvePlot->yAxis->setRange(-portsY, 2*portsY);
    }
    else
    {
        // todo put text on plot indicating invalid settings
    }
    ui->valvePlot->replot();
    squarePlotY(ui->valvePlot);
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
    newSettings.eccentricAdvance = SVE::deg2Rad(ui->eccentricAdvance->value());
    newSettings.valvePorts.topPort[0]  = -(ui->steamPortSpace->value() - ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.botPort[0] = (ui->steamPortSpace->value() - ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.exPort[0] = ui->exahustPortWidth->value() / 2;
    newSettings.valvePorts.topPort[1] = -(ui->steamPortSpace->value() + ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.botPort[1] = (ui->steamPortSpace->value() + ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.exPort[1] = -ui->exahustPortWidth->value() / 2;
    newSettings.valveSlide.topLand[0] = -ui->valveWidth->value()/2 - ui->valveTopLand->value();
    newSettings.valveSlide.botLand[0] = (ui->valveWidth->value()/2) + ui->valveBottomLand->value();;
    newSettings.valveSlide.topLand[1] = -ui->valveWidth->value()/2;
    newSettings.valveSlide.botLand[1] = (ui->valveWidth->value()/2);

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
    drawValveDiagram();
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

