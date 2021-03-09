#include "slidevalveengine.h"

double SVE::deg2Rad(double deg)
{
    return deg * M_PI / 180.0;
}

double SVE:: rad2Deg(double rad)
{
    return rad * 180.0 * M_1_PI;
}


SlideValveEngine::SlideValveEngine()
{
    // fill in some  known good default values
    _engineParams.bore = 7;
    _engineParams.stroke = 8.54;
    _engineParams.conRod = 22.375;
    _engineParams.valveTravel = 2.282;
    _engineParams.valveConRod = 22;
    _engineParams.eccentricAdvance = 1.57;
    _engineParams.valvePorts.topPort[0] = 1.07;
    _engineParams.valvePorts.topPort[1] = 1.725;
    _engineParams.valvePorts.botPort[0] = -1.07;
    _engineParams.valvePorts.botPort[1] = -1.725;
    _engineParams.valvePorts.exPort[0] = -.55;
    _engineParams.valvePorts.exPort[1] = .55;
    _engineParams.valveSlide.topLand[0] = 1.06;
    _engineParams.valveSlide.topLand[1] = 2.33;
    _engineParams.valveSlide.botLand[0] = -.9;
    _engineParams.valveSlide.botLand[1] = -2.33;

}

SlideValveEngine::SlideValveEngine(s_engineParams params)
{
    if (validateSettings(params) == ErrorEnum::none)
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
        _engineParams.eccentricAdvance = 1.57;
        _engineParams.valvePorts.topPort[0] = 1.07;
        _engineParams.valvePorts.topPort[1] = 1.725;
        _engineParams.valvePorts.botPort[0] = -1.07;
        _engineParams.valvePorts.botPort[1] = -1.725;
        _engineParams.valvePorts.exPort[0] = -.55;
        _engineParams.valvePorts.exPort[1] = .55;
        _engineParams.valveSlide.topLand[0] = 1.06;
        _engineParams.valveSlide.topLand[1] = 2.33;
        _engineParams.valveSlide.botLand[0] = -.9;
        _engineParams.valveSlide.botLand[1] = -2.33;
    }
}

ErrorEnum SlideValveEngine::validateSettings(s_engineParams params)
{
    // check the basic stuff
    if (params.conRod <= params.stroke)
        return ErrorEnum::error;
    if (params.valveConRod <= params.valveTravel)
        return ErrorEnum::error;

    // todo: more advanced checks

    return ErrorEnum::none;
}

s_engineParams SlideValveEngine::getEngineParams()
{
    return _engineParams;
}

ErrorEnum SlideValveEngine::setEngineParams(s_engineParams newParams)
{
    ErrorEnum ret = validateSettings(newParams);
    if (ret == ErrorEnum::none)
        _engineParams = newParams;
    return ret;
}

double SlideValveEngine::crankRad2Stroke(double rad)
{
    // returns the stroke position offset from TDC for a given crank position
    // crank position angle is measured from 0 at TDC
    // from piston motion equations, x = distance from crankshaft to crosshead/piston
    double r = _engineParams.stroke / 2.0;
    double x = r * std::cos(rad) + std::sqrt(std::pow(_engineParams.conRod, 2) - std::pow(r*std::sin(rad), 2));
    // piston position(from TDC) = x(tdc) - x(angle)
    return _engineParams.conRod + r - x;
}

double SlideValveEngine::stroke2CrankRad(double pos)
{
    // Calculates the crankshaft position in radians (0 is TDC) given the piston position
    // always returns the crank angle in forward stroke

    double r = _engineParams.stroke / 2.0;              // crank circle radius

    // trap for the extreams
    if (pos <= 0)
    {
        return 0.0;
    }
    else if (pos >= _engineParams.stroke)
    {
        return M_PI;
    }

    // use Law of Cosines to find the Y coord of the crank pin
    double l = _engineParams.conRod;                // makes the following equations shorter and easier to read
    double b = (l + r) - pos;                       // base of triangle formed by wrist pin, crank pin, crank shaft
    double d = (l*l + b*b - r*r) / (2*b);           // d is distance from wrist pin to the projection of the crank pin onto the cyttlinder axis
    double h = std::sqrt(l*l - d*d);                // h is the height of the triangle formed by the wrist pin, crank pin and crank shaft

    // find crank angle in radians, 0 is TDC
    return M_PI - std::atan2(h, d);
}

double SlideValveEngine::stroke2CrankRad(double pos, bool ret)
{

    if (ret)    // modify for return stroke
        return 2 * M_PI - stroke2CrankRad(pos);
    else    // return the unmodified value if at BDC or on forward stroke
        return stroke2CrankRad(pos);
}

/*!
 * Given a crank position (rad), calculates the smallest crank position > rad such that
 * crankRad2Cycle(new) follows crankRad2Cycle(rad).
 * \param rad crank position in radians
 * \param ret if true calculates for return stroke
 * \return next crank position in radians
 */

double SlideValveEngine::nextCycleRad(double rad, bool ret)
{
    // get cycle region for the argument
    CycleEnum oldCycle = crankRad2Cycle(rad, ret);
    CycleEnum newCycle = CycleEnum::intake;

    // get the next cycle information
    std::array<double, 4> critP = criticalPointsRad(ret);
    double nextPoint = 0;
    switch(oldCycle){
    case CycleEnum::intake:
        newCycle = CycleEnum::expansion;
        nextPoint = critP[1];
        break;
    case CycleEnum::expansion:
        newCycle = CycleEnum::exahust;
        nextPoint = critP[2];
        break;
    case CycleEnum::exahust:
        newCycle = CycleEnum::compression;
        nextPoint = critP[3];
        break;
    case CycleEnum::compression:
        newCycle = CycleEnum::intake;
        nextPoint = critP[0];
        break;
    }

    // estimate the next position
    double wrappedRad = fmod(rad, 2 * M_PI);
        if (wrappedRad < 0)
            wrappedRad += 2 * M_PI;

    double nextPos;
    if (wrappedRad < nextPoint)
        nextPos = rad + (nextPoint - wrappedRad);
    else
        nextPos = rad + (2*M_PI - wrappedRad + nextPoint);

    // verify that the new position will evaluate to the next cycle
    CycleEnum testCycle = crankRad2Cycle(nextPos, ret);
    if (testCycle == newCycle)
        return nextPos;
    else if (testCycle == oldCycle)
        while (crankRad2Cycle(nextPos, ret) != newCycle)
            nextPos += .001;
    else
        throw;
    return nextPos;
}

CycleEnum SlideValveEngine::crankRad2Cycle(double rad, bool ret)
{
    CycleEnum cycle = CycleEnum::intake;    

    // calculate arg mod 2*Pi
    double wrappedRad = fmod(rad, 2 * M_PI);
        if (wrappedRad < 0)
            wrappedRad += 2 * M_PI;

    // compare with critical points to find region
    std::array<double, 4> critP = criticalPointsRad(ret);
    // find (crit - wrappedRad) for all crit (crit >=0 by definition)
    std::array<double, 4> diffs;
    for (int i=0; i<4; i++)
        diffs[i] = critP[i] - wrappedRad;

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
    }else if (diffs[1] > 0 && diffs[1] > 0 && diffs[2] > 0 && diffs[3] > 0){ // if all diff + choose largest
        double maxDiff = 0;
        for (int i=0; i<4; i++){
            if (diffs[i] > maxDiff){
                maxDiff = diffs[i];
                startPoint = i;
            }
        }
    }else{ // if any - choose largest that is < 0
        double maxDiff = -100;
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

double SlideValveEngine::crankRadInlet(bool ret)                              // returns the crank position when the steam port opens in radians. if ret is true, returns the value for the return stroke
{
    if (ret)
        return M_PI - .1;
    else
        return 2 * M_PI - .1;
}

double SlideValveEngine::crankRadCutoff(bool ret)                             // returns the crank position when the steam port closes in radians. if ret is true, returns the value for the return stroke
{
    if (ret)
        return 2 * M_PI - .7;
    else
        return  M_PI - .7;
}

double SlideValveEngine::crankRadRelease(bool ret)                            // returns the crank position when the exahust port opens in radians. if ret is true, returns the value for the return stroke
{
    if (ret)
        return 2 * M_PI - .3;
    else
        return M_PI - .3;
}

double SlideValveEngine::crankRadCompression(bool ret)                        // returns the crank position when the exahust port closes in radians. if ret is true, returns the value for the return stroke
{
    if (ret)
        return M_PI - .4;
    else
        return 2 * M_PI - .4;
}

std::array<double, 4> SlideValveEngine::criticalPointsRad(bool ret)
{
    std::array<double, 4> foo;
    foo[0] = crankRadInlet(ret);
    foo[1] = crankRadCutoff(ret);
    foo[2] = crankRadRelease(ret);
    foo[3] = crankRadCompression(ret);
    return foo;
}

// ********************* degree versions of the functions *****************************
double SlideValveEngine::crankDeg2Stroke(double deg)
{    
    return crankRad2Stroke(SVE::deg2Rad(deg));
}

double SlideValveEngine::stroke2CrankDeg(double pos)
{    
    return SVE::rad2Deg(stroke2CrankRad(pos));
}

double SlideValveEngine::stroke2CrankDeg(double pos, bool ret)
{
    return SVE::rad2Deg(stroke2CrankRad(pos, ret));
}

double SlideValveEngine::crankDegInlet(bool ret)
{    
    return SVE::rad2Deg(crankRadInlet(ret));
}

double SlideValveEngine::crankDegCutoff(bool ret)
{
    return SVE::rad2Deg(crankRadCutoff(ret));
}

double SlideValveEngine::crankDegRelease(bool ret)
{
    return SVE::rad2Deg(crankRadRelease(ret));
}

double SlideValveEngine::crankDegCompression(bool ret)
{
    return SVE::rad2Deg(crankRadCompression(ret));
}

CycleEnum SlideValveEngine::crankDeg2Cycle(double deg, bool ret)
{
    return crankRad2Cycle(SVE::deg2Rad(deg), ret);
}

std::array<double, 4> SlideValveEngine::criticalPointsDeg(bool ret)
{
    std::array<double, 4> foo = criticalPointsRad(ret);
    std::array<double, 4> bar;
    for (int i=0; i<4; i++)
        bar[i] = SVE::rad2Deg(foo[i]);
    return bar;
}
