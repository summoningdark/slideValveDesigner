#include "slidevalveengine.h"

bool SVE::comparePointsLT(std::pair<double, int> point1, std::pair<double, int> point2){
 return point1.first < point2.first;
}

bool comparePointEQ(std::pair<double, int> point, double val){
    return point.first == val;
}

double SVE::deg2Rad(double deg)
{
    return deg * M_PI / 180.0;
}

double SVE:: rad2Deg(double rad)
{
    return rad * 180.0 * M_1_PI;
}

/*!
 * adds degree angles with wrapping so result is 0 to 360
 * \param deg1 first angle in degrees
 * \param deg2 second angle in degrees
 * \return restricted angle in degrees
 */
double SVE::addAngles(double deg1, double deg2)
{
    double rawSum = deg1 + deg2;
    double wrapped = fmod(rawSum, 360.0);
    if (wrapped < 0)
        return 360.0 + wrapped;
    else
        return wrapped;
}

/*!
 * calculate stroke position given grankshaft position in degrees.
 * stroke position is measured from 0 at Top Dead Center (TDC)
 * \param deg       crankshaft position in degrees measured from 0 at TDC
 * \param stroke    Total stroke
 * \param length    Connecting rod length
 * \return          double stroke position
 */
double SVE::crank2Stroke(double deg, double stroke, double length)
{
    // returns the stroke position offset from TDC for a given crank position
    // crank position angle is measured from 0 at TDC
    // from piston motion equations, x = distance from crankshaft to crosshead/piston
    double r = stroke / 2.0;
    double x = r * std::cos(SVE::deg2Rad(deg)) + std::sqrt(std::pow(length, 2) - std::pow(r*std::sin(SVE::deg2Rad(deg)), 2));
    // piston position(from TDC) = x(tdc) - x(angle)
    return length + r - x;
}

/*!
 * calculates the crankshaft position in degrees given a stroke offset. By default calculates the crankshaft position for the forward stroke (return value will be between 0 and 180)
 * stroke offsets are in linear units starting at 0 at TDC, and reaching a maximum of stroke at Bottom Dead Center (BDC)
 * \param pos       Stroke position
 * \param stroke    Total stroke
 * \param length    Connecting rod length
 * \param ret       if true, the crankshaft position is calculated assuming the return stroke (return value will be between 180 and 360)
 */
double SVE::stroke2Crank(double pos, double stroke, double length, bool ret)
{
    double r = stroke / 2.0;              // crank circle radius

    // trap for the extreams
    if (pos <= 0)
    {
        return 0.0;
    }
    else if (pos >= stroke)
    {
        return 180.0;
    }

    // use Law of Cosines to find the crank angle
    // l^2 = r^2+x^2 - 2rxCos(theta)
    double x = (length + r) - pos;                            // distance from crankshaft to write pin
    double arg = (x*x + r*r - length*length)/(2*r*x);
    double a = SVE::rad2Deg(std::acos(arg));

    //correct for return stroke
    if (ret)    // modify for return stroke
        return 360.0 - a;
    else
        return a;
}


SlideValveEngine::SlideValveEngine()
{
    // fill in some  known good default values
    _engineParams.bore = 7;
    _engineParams.stroke = 8.54;
    _engineParams.conRod = 22.375;
    _engineParams.valveTravel = 2.282;
    _engineParams.valveConRod = 22;
    _engineParams.eccentricAdvance = 120.0;
    _engineParams.valvePorts.topPort[0] = -1.07;
    _engineParams.valvePorts.topPort[1] = -1.725;
    _engineParams.valvePorts.botPort[0] = 1.07;
    _engineParams.valvePorts.botPort[1] = 1.725;
    _engineParams.valvePorts.exPort[0] = .55;
    _engineParams.valvePorts.exPort[1] = -.55;
    _engineParams.valveSlide.topLand[0] = -1.06;
    _engineParams.valveSlide.topLand[1] = -2.33;
    _engineParams.valveSlide.botLand[0] = .9;
    _engineParams.valveSlide.botLand[1] = 2.33;

    calcCriticalPoints(_engineParams);
}

SlideValveEngine::SlideValveEngine(s_engineParams params)
{
    if (validateSettings(params) == ErrorEnum::none)    // validate calls calcCriticalPoints
    {
        _engineParams = params;        
    }
    else
    {
        // fill in some known good default values
        _engineParams.bore = 7;
        _engineParams.stroke = 8.54;
        _engineParams.conRod = 22.375;
        _engineParams.valveTravel = 2.282;
        _engineParams.valveConRod = 22;
        _engineParams.eccentricAdvance = 120.0;
        _engineParams.valvePorts.topPort[0] = -1.07;
        _engineParams.valvePorts.topPort[1] = -1.725;
        _engineParams.valvePorts.botPort[0] = 1.07;
        _engineParams.valvePorts.botPort[1] = 1.725;
        _engineParams.valvePorts.exPort[0] = .55;
        _engineParams.valvePorts.exPort[1] = -.55;
        _engineParams.valveSlide.topLand[0] = -1.06;
        _engineParams.valveSlide.topLand[1] = -2.33;
        _engineParams.valveSlide.botLand[0] = .9;
        _engineParams.valveSlide.botLand[1] = 2.33;
        calcCriticalPoints(_engineParams);
    }

}

ErrorEnum SlideValveEngine::validateSettings(s_engineParams params)
{
    // check the basic stuff
    if (params.conRod <= params.stroke)
        return ErrorEnum::error;
    if (params.valveConRod <= params.valveTravel)
        return ErrorEnum::error;

    //valve porting checks
    if (params.valvePorts.topPort[0] <= params.valvePorts.topPort[1])    // top port values reversed
        return ErrorEnum::error;
    if (params.valvePorts.exPort[0] <= params.valvePorts.exPort[1])      // exahust port values reversed
        return ErrorEnum::error;
    if (params.valvePorts.botPort[1] <= params.valvePorts.botPort[0])    // bot port values reversed
        return ErrorEnum::error;
    if (params.valvePorts.topPort[0] >= params.valvePorts.exPort[1])    // top bridge is <=0
        return ErrorEnum::error;
    if (params.valvePorts.botPort[0] <= params.valvePorts.exPort[0])    // bot bridge is <=0
        return ErrorEnum::error;

    // TODO: check for exahust restriction if exahust port is too narrow

    // check that critical points calculate ok
    ErrorEnum err = calcCriticalPoints(params);     // if this returns no error, critical points are updated
    if (err != ErrorEnum::none)
        return err;

    return ErrorEnum::none;
}

s_engineParams SlideValveEngine::getEngineParams()
{
    return _engineParams;
}

ErrorEnum SlideValveEngine::setEngineParams(s_engineParams newParams)
{
    ErrorEnum ret = validateSettings(newParams);    // validate calls calcCriticalPoints
    if (ret == ErrorEnum::none){
        _engineParams = newParams;
    }
    return ret;
}

ErrorEnum SlideValveEngine::calcCriticalPoints(s_engineParams params){
    double offsetFIO;                 // linear position offset from valve neutral for forward intake/cutoff
    double offsetFRC;                 // linear position offset from valve neutral for forward release/compression
    double offsetRIO;                 // linear position offset from valve neutral for reverse intake/cutoff
    double offsetRRC;                 // linear position offset from valve neutral for reverse release/compression
    double ecc[8];

    // calculate offsets
    offsetFIO = params.valvePorts.topPort[1] - params.valveSlide.topLand[1];
    offsetFRC = params.valvePorts.topPort[0] - params.valveSlide.topLand[0];
    offsetRIO = params.valvePorts.botPort[1] - params.valveSlide.botLand[1];
    offsetRRC = params.valvePorts.botPort[0] - params.valveSlide.botLand[0];


    // calculate points
    ecc[0] = SVE::addAngles(SVE::stroke2Crank(params.valveTravel/2 + offsetFIO, params.valveTravel, params.valveConRod, false), -params.eccentricAdvance);
    ecc[1] = SVE::addAngles(SVE::stroke2Crank(params.valveTravel/2 + offsetFIO, params.valveTravel, params.valveConRod, true), -params.eccentricAdvance);
    ecc[2] = SVE::addAngles(SVE::stroke2Crank(params.valveTravel/2 + offsetFRC, params.valveTravel, params.valveConRod, true), -params.eccentricAdvance);
    ecc[3] = SVE::addAngles(SVE::stroke2Crank(params.valveTravel/2 + offsetFRC, params.valveTravel, params.valveConRod, false), -params.eccentricAdvance);
    ecc[4] = SVE::addAngles(SVE::stroke2Crank(params.valveTravel/2 + offsetRIO, params.valveTravel, params.valveConRod, true), -params.eccentricAdvance);
    ecc[5] = SVE::addAngles(SVE::stroke2Crank(params.valveTravel/2 + offsetRIO, params.valveTravel, params.valveConRod, false), -params.eccentricAdvance);
    ecc[6] = SVE::addAngles(SVE::stroke2Crank(params.valveTravel/2 + offsetRRC, params.valveTravel, params.valveConRod, false), -params.eccentricAdvance);
    ecc[7] = SVE::addAngles(SVE::stroke2Crank(params.valveTravel/2 + offsetRRC, params.valveTravel, params.valveConRod, true), -params.eccentricAdvance);

    _criticalPoints[0] = ecc[0];
    _criticalPoints[1] = ecc[1];
    _criticalPoints[2] = ecc[2];
    _criticalPoints[3] = ecc[3];

    _criticalPoints[4] = ecc[4];
    _criticalPoints[5] = ecc[5];
    _criticalPoints[6] = ecc[6];
    _criticalPoints[7] = ecc[7];


    // for testing
/*
    _forwardCriticalPoints[0] = 358;
    _forwardCriticalPoints[1] = 120;
    _forwardCriticalPoints[2] = 150;
    _forwardCriticalPoints[3] = 340;

    _returnCriticalPoints[0] = 178;
    _returnCriticalPoints[1] = 300;
    _returnCriticalPoints[2] = 330;
    _returnCriticalPoints[3] = 160;
*/
    return ErrorEnum::none;

}


double SlideValveEngine::stroke2Crank(double pos, bool ret)
{
    return SVE::stroke2Crank(pos, _engineParams.stroke, _engineParams.conRod, ret);
}

double SlideValveEngine::crank2Stroke(double deg)
{
    return SVE::crank2Stroke(deg, _engineParams.stroke, _engineParams.conRod);
}

double SlideValveEngine::valvePos2Crank(double pos, bool ret)
{
    // valve position is measured relative to neutral, so convert to position from TDC
    double posFromTDC = pos + (_engineParams.valveTravel/2.0);
    //now use the standard piston motion call
    double eccentricAngle = SVE::stroke2Crank(posFromTDC, _engineParams.valveTravel, _engineParams.valveConRod, ret);
    // apply offset to get crankshaft angle
    return SVE::addAngles(eccentricAngle, -_engineParams.eccentricAdvance);
}

double SlideValveEngine::crank2ValvePos(double deg)
{
    // convert crankshaft angle to eccentric angle
    double eccAngle = SVE::addAngles(deg, _engineParams.eccentricAdvance);
    // get valve offset from TDC
    double posFromTDC = SVE::crank2Stroke(eccAngle, _engineParams.valveTravel, _engineParams.valveConRod);
    // convert to valve position from neutral
    return posFromTDC - (_engineParams.valveTravel/2.0);
}

CycleEnum SlideValveEngine::crank2TopCycle(double deg){
    return crank2Cycle(deg, false);
}

CycleEnum SlideValveEngine::crank2BotCycle(double deg)
{
    return crank2Cycle(deg, true);
}

CycleEnum SlideValveEngine::crank2Cycle(double deg, bool ret)
{
    CycleEnum cycle = CycleEnum::intake;    

    // calculate wrapped angle
    double wrapped = SVE::addAngles(deg, 0);

    // compare with critical points to find region
    std::array<double, 4> critP;
    if (ret)
        critP = botCriticalPoints();
    else
        critP = topCriticalPoints();

    // find (crit - wrapped) for all crit (crit >=0 by definition)
    std::array<double, 4> diffs;
    for (int i=0; i<4; i++)
        diffs[i] = critP[i] - wrapped;

    // if any exact match choose the match
    int startPoint = 0;
    if (diffs[0] == 0){
        startPoint = 0;
    }else if (diffs[1] == 0){
        startPoint = 1;
    }else if (diffs[2] == 0){
        startPoint = 2;
    }else if (diffs[3] == 0){
        startPoint = 3;
    }else if (diffs[0] > 0 && diffs[1] > 0 && diffs[2] > 0 && diffs[3] > 0){ // if all diff + choose largest
        double maxDiff = 0;
        for (int i=0; i<4; i++){
            if (diffs[i] > maxDiff){
                maxDiff = diffs[i];
                startPoint = i;
            }
        }
    }else{ // if any - choose largest that is < 0
        double maxDiff = -1000;
        for (int i=0; i<4; i++){
            if (diffs[i] < 0 && diffs[i] > maxDiff){
                maxDiff = diffs[i];
                startPoint = i;
            }
        }
    }

    switch (startPoint){
    case 1:
        cycle = CycleEnum::expansion;
        break;
    case 2:
        cycle = CycleEnum::exahust;
        break;
    case 3:
        cycle = CycleEnum::compression;
        break;
    default:
        cycle = CycleEnum::intake;
        break;
    }

    return cycle;
}

int SlideValveEngine::nextTopCriticalPoint(double deg)
{
    return nextPoint(deg, topCriticalPoints());
}

int SlideValveEngine::nextBotCriticalPoint(double deg)
{
    return nextPoint(deg, botCriticalPoints());
}

int SlideValveEngine::nextPoint(double deg, std::array<double, 4> points){
    std::array<std::pair<double, int>, 4> newPoints;                        // array of pairs to keep track of original index
    newPoints[0] = std::make_pair(points[0], 0);
    newPoints[1] = std::make_pair(points[1], 1);
    newPoints[2] = std::make_pair(points[2], 2);
    newPoints[3] = std::make_pair(points[3], 3);

    std::sort(newPoints.begin(), newPoints.end(), SVE::comparePointsLT);                      // sort into acending order
    double wrapped = SVE::addAngles(deg, 0);                                //get wrapped version of deg
    for (int i=0; i<4; i++)                                                 // search the sorted points for the first larger than wrapped
        if (newPoints[i].first > wrapped){                                  // found the next point
            return newPoints[i].second;
        }
    // didn't find a next, must wrap around to the beginning
    return newPoints[0].second;
}

double SlideValveEngine::crankInlet(bool ret)                              // returns the crank position when the steam port opens in radians. if ret is true, returns the value for the return stroke
{
    if (ret)
        return _criticalPoints[0];
    else
        return _criticalPoints[0];
}

double SlideValveEngine::crankCutoff(bool ret)                             // returns the crank position when the steam port closes in radians. if ret is true, returns the value for the return stroke
{
    if (ret)
        return _criticalPoints[1];
    else
        return _criticalPoints[1];
}

double SlideValveEngine::crankRelease(bool ret)                            // returns the crank position when the exahust port opens in radians. if ret is true, returns the value for the return stroke
{
    if (ret)
        return _criticalPoints[2];
    else
        return _criticalPoints[2];
}

double SlideValveEngine::crankCompression(bool ret)                        // returns the crank position when the exahust port closes in radians. if ret is true, returns the value for the return stroke
{
    if (ret)
        return _criticalPoints[3];
    else
        return _criticalPoints[3];
}

std::array<double, 4> SlideValveEngine::topCriticalPoints()
{
    std::array<double, 4> foo;
    foo[0] = _criticalPoints[0];
    foo[1] = _criticalPoints[1];
    foo[2] = _criticalPoints[2];
    foo[3] = _criticalPoints[3];
    return foo;
}

std::array<double, 4> SlideValveEngine::botCriticalPoints()
{
    std::array<double, 4> foo;
    foo[0] = _criticalPoints[4];
    foo[1] = _criticalPoints[5];
    foo[2] = _criticalPoints[6];
    foo[3] = _criticalPoints[7];
    return foo;
}

std::array<double, 8> SlideValveEngine::criticalPoints()
{
    std::array<double, 8> foo;
    foo[0] = _criticalPoints[0];
    foo[1] = _criticalPoints[1];
    foo[2] = _criticalPoints[2];
    foo[3] = _criticalPoints[3];
    foo[4] = _criticalPoints[4];
    foo[5] = _criticalPoints[5];
    foo[6] = _criticalPoints[6];
    foo[7] = _criticalPoints[7];
    return foo;
}
