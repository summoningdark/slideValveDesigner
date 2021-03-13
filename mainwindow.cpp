#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    tracerPlot_ = -1;
    _engine = new SlideValveEngine();
    currentCrank_ = 0;
    ui->setupUi(this);

    // coloring brushes for cycle regions
    _regionBrush[CycleEnum::intake] = QBrush(QColor(0, 255, 0, 255));            // color for intake (dark green)
    _regionBrush[CycleEnum::expansion] = QBrush(QColor(150, 255, 150, 255));     // color for expansion (light green)
    _regionBrush[CycleEnum::exahust] = QBrush(QColor(100, 100, 255, 255));       // color for exahust (light blue)
    _regionBrush[CycleEnum::compression] = QBrush(QColor(255, 0, 0, 255));       // color for compression (red)

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

    connect(ui->valvePlot, SIGNAL(Resized(QCustomPlot*, QSize)), this, SLOT(squarePlot(QCustomPlot*, QSize)));

    ui->criticalPointSelect->setCurrentIndex(0);
    setCriticalPoint(0);
    tracerPlot_ = drawCycleDiagram();
    drawValveDiagram();
}

void MainWindow::updateEngineSettings()
{

    s_engineParams newSettings;
    newSettings.bore = ui->bore->value();
    newSettings.stroke = ui->stroke->value();
    newSettings.conRod = ui->conRod->value();
    newSettings.valveTravel = ui->valveTravel->value();
    newSettings.valveConRod = ui->valveConRod->value();
    newSettings.eccentricAdvance = ui->eccentricAdvance->value();
    newSettings.valvePorts.topPort[0]  = -(ui->steamPortSpace->value() - ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.botPort[0] = (ui->steamPortSpace->value() - ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.exPort[0] = ui->exahustPortWidth->value() / 2;
    newSettings.valvePorts.topPort[1] = -(ui->steamPortSpace->value() + ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.botPort[1] = (ui->steamPortSpace->value() + ui->steamPortWidth->value()) / 2;
    newSettings.valvePorts.exPort[1] = -ui->exahustPortWidth->value() / 2;
    newSettings.valveSlide.topLand[0] = -ui->valveWidth->value()/2 + ui->valveTopLand->value();
    newSettings.valveSlide.botLand[0] = (ui->valveWidth->value()/2) - ui->valveBottomLand->value();;
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
}

void MainWindow::updateCurrentAngle(double deg)
{
    ui->currentAngle->blockSignals(true);
    ui->currentAngle->setValue(deg);
    ui->currentAngle->blockSignals(false);
}

void MainWindow::updateAnimationSlider(double deg)
{
    ui->animationSlider->blockSignals(true);
    // set to closest integer degree
    ui->animationSlider->setValue(std::round(SVE::addAngles(deg, 0)));
    ui->animationSlider->blockSignals(false);
}

void MainWindow::updateCriticalPoint(double deg)
{
    ui->criticalPointSelect->blockSignals(true);
    ui->criticalPointSelect->setCurrentIndex(-1);
    ui->criticalPointSelect->blockSignals(false);
}

void MainWindow::sliderChanged(int value)
{
    setCurrentCrank((double)value);
    updateCriticalPoint(currentCrank_);
    updateCurrentAngle(currentCrank_);
    drawValveDiagram();
    if (tracerPlot_ != -1){
        ui->cyclePlot->graph(tracerPlot_)->setData(QVector<double>{currentCrank_}, QVector<double>{_engine->crank2Stroke(currentCrank_)});
        ui->cyclePlot->replot();
    }
}

void MainWindow::currentAngleChanged(double value)
{
    setCurrentCrank(value);
    updateAnimationSlider(value);
    updateCriticalPoint(value);
    drawValveDiagram();
    if (tracerPlot_ != -1){
        ui->cyclePlot->graph(tracerPlot_)->setData(QVector<double>{currentCrank_}, QVector<double>{_engine->crank2Stroke(currentCrank_)});
        ui->cyclePlot->replot();
    }
}

void MainWindow::setCriticalPoint(int value)
{
    setCurrentCrank(_engine->criticalPoints()[value]);
    updateAnimationSlider(currentCrank_);
    updateCurrentAngle(currentCrank_);
    drawValveDiagram();
    if (tracerPlot_ != -1){
        ui->cyclePlot->graph(tracerPlot_)->setData(QVector<double>{currentCrank_}, QVector<double>{_engine->crank2Stroke(currentCrank_)});
        ui->cyclePlot->replot();
    }
}

void MainWindow::setCurrentCrank(double deg)
{
    currentCrank_ = deg;
    currentStroke_ = _engine->crank2Stroke(currentCrank_);
    currentValvePos_ = _engine->crank2ValvePos(currentCrank_);
    currentTopCycle_ = _engine->crank2TopCycle(currentCrank_);
    currentBotCycle_ = _engine->crank2BotCycle(currentCrank_);

    ui->topCycle->setText(cycleNames_.find(currentTopCycle_)->second);
    ui->bottomCycle->setText(cycleNames_.find(currentBotCycle_)->second);
}

void MainWindow::squarePlot(QCustomPlot *plot, QSize s)
{
    if (s.height() > s.width())
        plot->xAxis->setScaleRatio(plot->yAxis,1.0);
    else
        plot->yAxis->setScaleRatio(plot->xAxis,1.0);
    plot->replot();
}

/*********************************** Methods which draw diagrams ***************************************************************************************/
void MainWindow::drawValveDiagram()
{
    ui->valvePlot->clearGraphs();
    ui->valvePlot->clearItems();
    ui->valvePlot->clearPlottables();

    if (_settingsOK)
    {
       // calculate some drawing parameters so the diagram looks nice
       // Note 0,0 is the center of the valve face, and the cylinder axis is below the Y axis
       auto params = _engine->getEngineParams();
       double topPortWidth = params.valvePorts.topPort[0] - params.valvePorts.topPort[1];   // width of the top steam port
       double botPortWidth = params.valvePorts.botPort[1] - params.valvePorts.botPort[0];   // width of the bottom steam port
       double valveSizeParameter = params.valveTravel / 4;                                  // atomic unit for drawing the valve and steam chest heights
       double pistonWidth = params.bore / 8;                                                  // width of piston width/piston rod and outer walls
       if (pistonWidth < 1.5*valveSizeParameter)
           pistonWidth = 1.5*valveSizeParameter;
       double outsideEdge = pistonWidth + (params.stroke + pistonWidth)/2;                            // make the outside edge 1 piston width larger than the stroke on each side
       double insideEdge = outsideEdge - pistonWidth;
       double steamChestTop = 4*valveSizeParameter;
       double topEdge = steamChestTop + pistonWidth;
       double portWall = 3 * ((topPortWidth > botPortWidth) ? topPortWidth : botPortWidth); // wall between the cylinder bore and valve face = 3 time larger port
       double cylinderBottom = -portWall - params.bore;
       double bottomEdge = cylinderBottom - pistonWidth;

       // draw cylinder1
       QCPCurve *cylinder1 = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       cylinder1->data()->set(drawCylinder1(outsideEdge, insideEdge, bottomEdge, topEdge, portWall, pistonWidth));
       cylinder1->setBrush(QBrush(QColor(0,0,0,255)));

       QCPCurve *forwardShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       forwardShade->data()->set(drawForwardShade(currentStroke_, insideEdge, portWall, cylinderBottom, pistonWidth));
       forwardShade->setBrush(_regionBrush[currentTopCycle_]);
       forwardShade->setPen(QPen(QColor(0,0,0,0)));

       QCPCurve *reverseShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       reverseShade->data()->set(drawReverseShade(currentStroke_, insideEdge, portWall, cylinderBottom, pistonWidth));
       reverseShade->setBrush(_regionBrush[currentBotCycle_]);
       reverseShade->setPen(QPen(QColor(0,0,0,0)));

       QCPCurve *piston = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       piston->data()->set(drawPiston(currentStroke_, portWall, cylinderBottom, pistonWidth));
       piston->setBrush(QBrush(QColor(150,150,150,255)));

       QCPCurve *steamChestShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       steamChestShade->data()->set(drawSteamChestShade(insideEdge, steamChestTop));
       steamChestShade->setBrush(_regionBrush[CycleEnum::intake]);  // steam chest is always full of full pressure steam
       steamChestShade->setPen(QPen(QColor(0,0,0,0)));

       QCPCurve *exahustShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       exahustShade->data()->set(drawExahustShade(portWall));
       exahustShade->setBrush(_regionBrush[CycleEnum::exahust]);  // steam chest is always full of full pressure steam
       exahustShade->setPen(QPen(QColor(0,0,0,0)));

      // draw slide valve curve
      QCPCurve *slideValve = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
      slideValve->data()->set(drawSlide(currentValvePos_, valveSizeParameter));
      slideValve->setBrush(QBrush(QColor(150,150,150,255)));

      QCPCurve *valveShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
      valveShade->data()->set(drawValveShade(currentValvePos_, valveSizeParameter));
      valveShade->setBrush(_regionBrush[CycleEnum::exahust]);
      valveShade->setPen(QPen(QColor(0,0,0,0)));

      // draw cylinder2
      QCPCurve *cylinder2 = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
      cylinder2->data()->set(drawCylinder2(portWall, insideEdge));
      cylinder2->setBrush(QBrush(QColor(0,0,0,255)));
    }
    else
    {
        // todo put text on plot indicating invalid settings
    }
    ui->valvePlot->replot();
    squarePlot(ui->valvePlot, ui->valvePlot->size());
}

int MainWindow::drawCycleDiagram()
{
    ui->cyclePlot->clearGraphs();
    ui->cyclePlot->clearItems();
    if (_settingsOK)
    {
        double stroke = _engine->getEngineParams().stroke;

        // draw shaded regions from <stroke> to the X-Axis for the bottom port regions
        // these will be over-drawn by the position curve
        int graphIndex = -1;            // for counting graphs
        double crankPos = -180;
        double crankStop = 440;

        // get the critical points
        auto foo = _engine->botCriticalPoints();
        //find the index of the first point after the starting position
        int nextIndex = _engine->nextBotCriticalPoint(crankPos);
        while (crankPos < crankStop)
        {
            // make graph from crank position to next critical point (or end whichever comes first)
            ui->cyclePlot->addGraph();      // add graph
            graphIndex++;                   // count it
            double diff = SVE::addAngles(foo[nextIndex], -crankPos);
            if (diff < 0)
                diff +=360;
            double nextCrankPos = crankPos + diff;
            if (++nextIndex ==4)
                    nextIndex = 0;
            // set the fill color based on region
            CycleEnum cycle = _engine->crank2BotCycle((crankPos+nextCrankPos)/2);
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
        foo = _engine->topCriticalPoints();
        nextIndex = _engine->nextTopCriticalPoint(crankPos);
        while (crankPos < crankStop)
        {
            // make graph from crank position to next critical point (or end whichever comes first)
            ui->cyclePlot->addGraph();      // add graph
            graphIndex++;                   // count it
            double diff = SVE::addAngles(foo[nextIndex], -crankPos);
            if (diff < 0)
                diff +=360;
            double nextCrankPos = crankPos + diff;
            if (++nextIndex ==4)
                    nextIndex = 0;
            // set the fill color based on region
            CycleEnum cycle = _engine->crank2TopCycle((crankPos + nextCrankPos) / 2);
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

        // Add tracer Graph
        ui->cyclePlot->addGraph();      // add graph
        graphIndex++;                   // count it
        ui->cyclePlot->graph(graphIndex)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::red, Qt::white, 10));
        ui->cyclePlot->graph(graphIndex)->setData(QVector<double>{currentCrank_}, QVector<double>{_engine->crank2Stroke(currentCrank_)});

        ui->cyclePlot->rescaleAxes();
        ui->cyclePlot->replot();
        return graphIndex;
    }
    else
    {
        // todo put text on plot indicating invalid settings
    }
    ui->cyclePlot->replot();
    return -1;
}
/********************************* methods to generate graphical paths for the simulation diagram ******************************************************/

QCPDataContainer<QCPCurveData> MainWindow::drawSlide(double offset, double sizeParam){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[1], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[1], 2*sizeParam));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[1], 2*sizeParam));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[1], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], sizeParam));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[0], sizeParam));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[0], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[1], 0));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawCylinder1(double outsideEdge, double insideEdge, double bottomEdge, double topEdge, double portWall, double piston){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double axis = -portWall - params.bore / 2;
    double portWidth = portWall / 3;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, -outsideEdge, bottomEdge));
    data.add(QCPCurveData(pointIndex++, -outsideEdge, topEdge));
    data.add(QCPCurveData(pointIndex++, outsideEdge, topEdge));
    data.add(QCPCurveData(pointIndex++, outsideEdge, axis + piston / 2));
    data.add(QCPCurveData(pointIndex++, insideEdge, axis + piston / 2));
    data.add(QCPCurveData(pointIndex++, insideEdge, -portWidth ));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[1], -portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[1], 0));
    data.add(QCPCurveData(pointIndex++, insideEdge, 0));
    data.add(QCPCurveData(pointIndex++, insideEdge, topEdge - piston));
    data.add(QCPCurveData(pointIndex++, -insideEdge, topEdge - piston));
    data.add(QCPCurveData(pointIndex++, -insideEdge, 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[1], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[1], -portWidth));
    data.add(QCPCurveData(pointIndex++, -insideEdge, -portWidth));
    data.add(QCPCurveData(pointIndex++, -insideEdge, axis - params.bore / 2));
    data.add(QCPCurveData(pointIndex++, insideEdge, axis - params.bore / 2));
    data.add(QCPCurveData(pointIndex++, insideEdge, axis - piston/2));
    data.add(QCPCurveData(pointIndex++, outsideEdge, axis - piston/2));
    data.add(QCPCurveData(pointIndex++, outsideEdge, bottomEdge));
    data.add(QCPCurveData(pointIndex++, -outsideEdge, bottomEdge));


    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawCylinder2(double portWall, double insideEdge){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double portWidth = portWall / 3;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, -insideEdge + portWidth, -portWall));
    data.add(QCPCurveData(pointIndex++, -insideEdge + portWidth, -portWall + portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[0], -portWall + portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[0], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[1], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[1], -2*portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], -2*portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[0], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[0], -portWall + portWidth));
    data.add(QCPCurveData(pointIndex++, insideEdge - portWidth , -portWall + portWidth));
    data.add(QCPCurveData(pointIndex++, insideEdge - portWidth , -portWall));
    data.add(QCPCurveData(pointIndex++, -insideEdge + portWidth, -portWall));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawPiston(double stroke, double portWall, double cylinderBottom, double pistonWidth){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double axis = (-portWall + cylinderBottom) / 2;
    double leftEdge = -(params.stroke / 2) - pistonWidth/2;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge, cylinderBottom));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge, -portWall));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + pistonWidth, -portWall));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + pistonWidth, axis + pistonWidth/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + 4*pistonWidth + params.stroke, axis + pistonWidth/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + 4*pistonWidth + params.stroke, axis - pistonWidth/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + pistonWidth, axis - pistonWidth/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + pistonWidth, cylinderBottom));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge, cylinderBottom));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawForwardShade(double stroke, double insideEdge, double portWall, double cylinderBottom, double pistonWidth){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double portWidth = portWall / 3;;
    double leftEdge = -(params.stroke / 2) - pistonWidth/2;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, -insideEdge, cylinderBottom));
    data.add(QCPCurveData(pointIndex++, -insideEdge, -portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[1], -portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[1], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[0], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[0], -2*portWidth));
    data.add(QCPCurveData(pointIndex++, -insideEdge + portWidth, -2*portWidth));
    data.add(QCPCurveData(pointIndex++, -insideEdge + portWidth, -portWall));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge, -portWall));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge, cylinderBottom));
    data.add(QCPCurveData(pointIndex++, -insideEdge, cylinderBottom));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawReverseShade(double stroke, double insideEdge, double portWall, double cylinderBottom, double pistonWidth){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double portWidth = portWall / 3;
    double rightEdge = -(params.stroke / 2) + pistonWidth/2;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[0], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[1], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[1], -portWidth));
    data.add(QCPCurveData(pointIndex++, insideEdge, -portWidth));
    data.add(QCPCurveData(pointIndex++, insideEdge, cylinderBottom));
    data.add(QCPCurveData(pointIndex++, stroke + rightEdge, cylinderBottom));
    data.add(QCPCurveData(pointIndex++, stroke + rightEdge, -portWall));
    data.add(QCPCurveData(pointIndex++, insideEdge - portWidth, -portWall));
    data.add(QCPCurveData(pointIndex++, insideEdge - portWidth, -2*portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[0], -2*portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[0], 0));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawValveShade(double offset, double sizeParam){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[0], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[0], sizeParam));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], sizeParam));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], 0));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawSteamChestShade(double insideEdge, double steamChestTop){
    QCPDataContainer<QCPCurveData> data;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, -insideEdge, 0));
    data.add(QCPCurveData(pointIndex++, -insideEdge, steamChestTop));
    data.add(QCPCurveData(pointIndex++, insideEdge, steamChestTop));
    data.add(QCPCurveData(pointIndex++, insideEdge, 0));
    data.add(QCPCurveData(pointIndex++, -insideEdge, 0));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawExahustShade(double portWall){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double portWidth = portWall / 3;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[1], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[1], -2*portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], -2*portWidth));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], 0));

    return data;
}

MainWindow::~MainWindow()
{
    delete ui;
}
