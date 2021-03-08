#include "bilgramdialog.h"
#include "ui_bilgramdialog.h"

BilgramDialog::BilgramDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BilgramDialog)
{
    ui->setupUi(this);
}

BilgramDialog::~BilgramDialog()
{
    delete ui;
}

void BilgramDialog::drawCrankDiagram()
{
//    // draw the crank circles
//    double crankRadius = ui->stroke->value() / 2.0;
//    double crankCenter = ui->conRod->value() + crankRadius;

//    // for normal screen resolution, 500 points should be sufficient
//    const int circlePoints = 500;
//    QVector<double> crankCircleX(circlePoints), crankCircleY(circlePoints);
//    for (int i=0; i< circlePoints; i++)
//    {
//        double theta = i/(double)(circlePoints-1)*M_PI;
//        crankCircleX[i] = crankCenter + crankRadius*qCos(theta);
//        crankCircleY[i] = crankRadius*qSin(theta);
//    }

//    ui->crankCutoffPlot->clearPlottables();
//    ui->crankCutoffPlot->clearItems();
//    ui->crankCutoffPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignRight);

//    QCPCurve *cuttoffCrankCircle = new QCPCurve(ui->crankCutoffPlot->xAxis, ui->crankCutoffPlot->yAxis);
//    cuttoffCrankCircle->setData(crankCircleX, crankCircleY);

//    // solve for connecting rod position(s)
//    QVector<double> cutoffRodX(3), cutoffRodY(3);
//    cutoffRodX[0] = ui->stroke->value() * ui->cutoff->value();
//    cutoffRodY[0] = 0;
//    cutoffRodX[2] = ui->conRod->value() + ui->stroke->value() / 2;
//    cutoffRodY[2] = 0;

//    std::tie(_cutoffCrankAngle,
//             cutoffRodX[1],
//             cutoffRodY[1] ) = stroke2Crank(ui->stroke->value(),
//                                            ui->cutoff->value(),
//                                            ui->conRod->value());

//    QCPCurve *cuttoffRodCurve = new QCPCurve(ui->crankCutoffPlot->xAxis, ui->crankCutoffPlot->yAxis);
//    cuttoffRodCurve->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 10));
//    cuttoffRodCurve->setData(cutoffRodX, cutoffRodY);

//    // do annotations
//    QCPItemText *cutoffAngleText = new QCPItemText(ui->crankCutoffPlot);
//    cutoffAngleText->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
//    cutoffAngleText->position->setType(QCPItemPosition::ptAxisRectRatio);
//    cutoffAngleText->position->setCoords(0.5, 0); // place position at center/top of axis rect
//    cutoffAngleText->setText("Crank Angle at Cutoff: " + QString::number(_cutoffCrankAngle));
//    cutoffAngleText->setFont(QFont(font().family(), 16)); // make font a bit larger

//    ui->crankCutoffPlot->xAxis->setRange(-crankRadius/2, crankCenter + 1.5 * crankRadius);
//    squarePlotY(ui->crankCutoffPlot);
}

void BilgramDialog::drawBilgramDiagram()
{

}

void BilgramDialog::squarePlotY(QCustomPlot *plot)
{
    plot->yAxis->setScaleRatio(plot->xAxis,1.0);
    plot->replot();
}
