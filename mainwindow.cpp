#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _engine = new SlideValveEngine();
    currentCrank_ = 0;

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

    connect(ui->valvePlot, SIGNAL(Resized(QCustomPlot*, QSize)), this, SLOT(squarePlot(QCustomPlot*, QSize)));

    drawCycleDiagram();
    drawValveDiagram();
}

void MainWindow::drawCycleDiagram()
{
    ui->cyclePlot->clearGraphs();
    ui->cyclePlot->clearItems();
    return;
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
       // need some values for offsets etc
       double stroke = _engine->crank2Stroke(currentCrank_);
       double valvePos = _engine->crank2ValvePos(currentCrank_);

       // draw cylinder1
       QCPCurve *cylinder1 = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       cylinder1->data()->set(drawCylinder1());
       cylinder1->setBrush(QBrush(QColor(0,0,0,255)));

       QCPCurve *forwardShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       forwardShade->data()->set(drawForwardShade(stroke));
       forwardShade->setBrush(_regionBrush[_engine->crank2Cycle(currentCrank_, false)]);
       forwardShade->setPen(QPen(QColor(0,0,0,0)));

       QCPCurve *reverseShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       reverseShade->data()->set(drawReverseShade(stroke));
       reverseShade->setBrush(_regionBrush[_engine->crank2Cycle(currentCrank_, true)]);
       reverseShade->setPen(QPen(QColor(0,0,0,0)));

       QCPCurve *piston = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       piston->data()->set(drawPiston(stroke));
       piston->setBrush(QBrush(QColor(150,150,150,255)));

       QCPCurve *steamChestShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       steamChestShade->data()->set(drawSteamChestShade());
       steamChestShade->setBrush(_regionBrush[CycleEnum::intake]);  // steam chest is always full of full pressure steam
       steamChestShade->setPen(QPen(QColor(0,0,0,0)));

       QCPCurve *exahustShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
       exahustShade->data()->set(drawExahustShade());
       exahustShade->setBrush(_regionBrush[CycleEnum::exahust]);  // steam chest is always full of full pressure steam
       exahustShade->setPen(QPen(QColor(0,0,0,0)));

      // draw slide valve curve
      QCPCurve *slideValve = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
      slideValve->data()->set(drawSlide(valvePos));
      slideValve->setBrush(QBrush(QColor(150,150,150,255)));

      QCPCurve *valveShade = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
      valveShade->data()->set(drawValveShade(valvePos));
      valveShade->setBrush(_regionBrush[CycleEnum::exahust]);
      valveShade->setPen(QPen(QColor(0,0,0,0)));

      // draw cylinder2
      QCPCurve *cylinder2 = new QCPCurve(ui->valvePlot->xAxis, ui->valvePlot->yAxis);
      cylinder2->data()->set(drawCylinder2());
      cylinder2->setBrush(QBrush(QColor(0,0,0,255)));
    }
    else
    {
        // todo put text on plot indicating invalid settings
    }
    ui->valvePlot->replot();
    squarePlot(ui->valvePlot, ui->valvePlot->size());
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

void MainWindow::setAnimationSlider(double deg)
{
    ui->animationSlider->blockSignals(true);
    // set to closest integer degree
    ui->animationSlider->setValue(std::round(SVE::addAngles(deg, 0)));
    ui->animationSlider->blockSignals(false);
}

void MainWindow::sliderChanged(int value)
{
    ui->criticalPointSelect->blockSignals(true);
    ui->criticalPointSelect->setCurrentIndex(-1);
    ui->criticalPointSelect->blockSignals(false);
    currentCrank_ = (double)value;
    ui->currentAngle->setText(QString::number(currentCrank_));
    drawValveDiagram();
}

void MainWindow::setCriticalPoint(int value)
{
    currentCrank_ = _engine->criticalPoints()[value];
    setAnimationSlider(currentCrank_);
    drawValveDiagram();
}

void MainWindow::squarePlot(QCustomPlot *plot, QSize s)
{
    if (s.height() > s.width())
        plot->xAxis->setScaleRatio(plot->yAxis,1.0);
    else
        plot->yAxis->setScaleRatio(plot->xAxis,1.0);
    plot->replot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

QCPDataContainer<QCPCurveData> MainWindow::drawSlide(double offset){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double portsY = params.valveTravel / 4;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[1], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[1], 2*portsY));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[1], 2*portsY));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[1], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], portsY));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[0], portsY));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[0], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[1], 0));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawCylinder1(){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double piston = params.stroke / 10;                       // value to use for the piston width/piston rod
    double valveHeight = params.valveTravel/4;
    double outsideEdge = piston + (params.stroke + piston)/2;
    double insideEdge = outsideEdge - piston;
    double bottomEdge = -(3*piston + params.bore);
    double topEdge = piston + 4*valveHeight;
    double axis = -2*piston - params.bore / 2;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, -outsideEdge, bottomEdge));
    data.add(QCPCurveData(pointIndex++, -outsideEdge, topEdge));
    data.add(QCPCurveData(pointIndex++, outsideEdge, topEdge));
    data.add(QCPCurveData(pointIndex++, outsideEdge, axis + piston / 2));
    data.add(QCPCurveData(pointIndex++, insideEdge, axis + piston / 2));
    data.add(QCPCurveData(pointIndex++, insideEdge, axis + params.bore / 2));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[1], 0));
    data.add(QCPCurveData(pointIndex++, insideEdge, 0));
    data.add(QCPCurveData(pointIndex++, insideEdge, topEdge - piston));
    data.add(QCPCurveData(pointIndex++, -insideEdge, topEdge - piston));
    data.add(QCPCurveData(pointIndex++, -insideEdge, 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[1], 0));
    data.add(QCPCurveData(pointIndex++, -insideEdge, axis + params.bore / 2));
    data.add(QCPCurveData(pointIndex++, -insideEdge, axis - params.bore / 2));
    data.add(QCPCurveData(pointIndex++, insideEdge, axis - params.bore / 2));
    data.add(QCPCurveData(pointIndex++, insideEdge, axis - piston/2));
    data.add(QCPCurveData(pointIndex++, outsideEdge, axis - piston/2));
    data.add(QCPCurveData(pointIndex++, outsideEdge, bottomEdge));
    data.add(QCPCurveData(pointIndex++, -outsideEdge, bottomEdge));


    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawCylinder2(){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double piston = params.stroke / 10;                       // value to use for the piston width/piston rod
    double axis = -2*piston - params.bore / 2;
    double outsideEdge = piston + (params.stroke + piston)/2;
    double insideEdge = outsideEdge - piston;
    double topPortWidth = params.valvePorts.topPort[0] - params.valvePorts.topPort[1];
    double botPortWidth = params.valvePorts.botPort[1] - params.valvePorts.botPort[0];

    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, -insideEdge + topPortWidth, axis + params.bore/2));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[0], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[1], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[1], -piston));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], -piston));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[0], 0));
    data.add(QCPCurveData(pointIndex++, insideEdge - botPortWidth , axis + params.bore/2));
    data.add(QCPCurveData(pointIndex++, -insideEdge + topPortWidth, axis + params.bore/2));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawPiston(double stroke){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double piston = params.stroke / 10;
    double axis = -2*piston - params.bore / 2;
    double leftEdge = -(params.stroke / 2) - piston/2;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge, axis - params.bore/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge, axis + params.bore/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + piston, axis + params.bore/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + piston, axis + piston/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + 4*piston + params.stroke, axis + piston/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + 4*piston + params.stroke, axis - piston/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + piston, axis - piston/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge + piston, axis - params.bore/2));
    data.add(QCPCurveData(pointIndex++, stroke + leftEdge, axis - params.bore/2));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawForwardShade(double stroke){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double piston = params.stroke / 10;
    double axis = -2*piston - params.bore / 2;
    double topPortWidth = params.valvePorts.topPort[0] - params.valvePorts.topPort[1];
    double leftEdge = -(params.stroke / 2) - piston/2;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, leftEdge, axis - params.bore/2));
    data.add(QCPCurveData(pointIndex++, leftEdge, axis + params.bore/2));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[1], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.topPort[0], 0));
    data.add(QCPCurveData(pointIndex++, leftEdge + topPortWidth, axis + params.bore/2));
    if (stroke + leftEdge > leftEdge){
        data.add(QCPCurveData(pointIndex++, stroke + leftEdge, axis + params.bore/2));
        data.add(QCPCurveData(pointIndex++, stroke + leftEdge, axis - params.bore/2));
    }else{
        data.add(QCPCurveData(pointIndex++, topPortWidth + leftEdge, axis + params.bore/2));
        data.add(QCPCurveData(pointIndex++, topPortWidth + leftEdge, axis - params.bore/2));
    }
    data.add(QCPCurveData(pointIndex++, leftEdge, axis - params.bore/2));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawReverseShade(double stroke){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double piston = params.stroke / 10;
    double outsideEdge = piston + (params.stroke + piston)/2;
    double insideEdge = outsideEdge - piston;
    double axis = -2*piston - params.bore / 2;
    double botPortWidth = params.valvePorts.botPort[1] - params.valvePorts.botPort[0];
    double rightEdge = -(params.stroke / 2) + piston/2;
    int pointIndex = 0;
    if (stroke + rightEdge < insideEdge){
        data.add(QCPCurveData(pointIndex++, stroke + rightEdge, axis - params.bore/2));
     } else {
        data.add(QCPCurveData(pointIndex++, insideEdge - botPortWidth, axis - params.bore/2));
     }
    data.add(QCPCurveData(pointIndex++, insideEdge, axis - params.bore/2));
    data.add(QCPCurveData(pointIndex++, insideEdge, axis + params.bore/2));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[1], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.botPort[0], 0));
    data.add(QCPCurveData(pointIndex++, insideEdge - botPortWidth, axis + params.bore/2));
    if (stroke + rightEdge < insideEdge){
        data.add(QCPCurveData(pointIndex++, stroke + rightEdge, axis + params.bore/2));
        data.add(QCPCurveData(pointIndex++, stroke + rightEdge, axis - params.bore/2));
     } else {
        data.add(QCPCurveData(pointIndex++, insideEdge - botPortWidth, axis + params.bore/2));
        data.add(QCPCurveData(pointIndex++, insideEdge - botPortWidth, axis - params.bore/2));
     }

    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawValveShade(double offset){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double portsY = params.valveTravel / 4;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[0], 0));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.topLand[0], portsY));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], portsY));
    data.add(QCPCurveData(pointIndex++, offset + params.valveSlide.botLand[0], 0));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawSteamChestShade(){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double piston = params.stroke / 10;                       // value to use for the piston width/piston rod
    double valveHeight = params.valveTravel/4;
    double outsideEdge = piston + (params.stroke + piston)/2;
    double insideEdge = outsideEdge - piston;
    double topEdge = piston + 4*valveHeight;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, -insideEdge, 0));
    data.add(QCPCurveData(pointIndex++, -insideEdge, topEdge - piston));
    data.add(QCPCurveData(pointIndex++, insideEdge, topEdge - piston));
    data.add(QCPCurveData(pointIndex++, insideEdge, 0));
    data.add(QCPCurveData(pointIndex++, -insideEdge, 0));
    return data;
}

QCPDataContainer<QCPCurveData> MainWindow::drawExahustShade(){
    QCPDataContainer<QCPCurveData> data;
    auto params = _engine->getEngineParams();
    double piston = params.stroke / 10;                       // value to use for the piston width/piston rod
    double valveHeight = params.valveTravel/4;
    double outsideEdge = piston + (params.stroke + piston)/2;
    double insideEdge = outsideEdge - piston;
    double topEdge = piston + 4*valveHeight;
    int pointIndex = 0;
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[1], 0));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[1], -piston));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], -piston));
    data.add(QCPCurveData(pointIndex++, params.valvePorts.exPort[0], 0));

    return data;
}
