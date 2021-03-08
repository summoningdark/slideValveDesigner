#include "slidevalveengine.h"

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

std::tuple<CycleEnum, double> SlideValveEngine::crankRad2Cycle(double rad, bool ret)
{
    CycleEnum cycle = CycleEnum::intake;
    double offsetToNext = 0;

    // calculate arg mod 2*Pi
    // compare with critical points to find region
    // calculate offset to next point

    return std::make_tuple(cycle, rad+ offsetToNext);
}


// ********************* degree versions of the functions *****************************
double SlideValveEngine::crankDeg2Stroke(double deg)
{
    double rad = deg * 180.0 * M_1_PI;
    return crankRad2Stroke(rad);
}

double SlideValveEngine::stroke2CrankDeg(double pos)
{
    double rad = stroke2CrankRad(pos);
    return rad * 180.0 * M_1_PI;
}

double SlideValveEngine::stroke2CrankDeg(double pos, bool ret)
{
    double rad = stroke2CrankRad(pos, ret);
    return rad * 180.0 * M_1_PI;
}

std::tuple<CycleEnum, double> SlideValveEngine::crankDeg2Cycle(double deg, bool ret)
{
    double rad = deg * M_PI / 180.0;
    return crankRad2Cycle(rad, ret);
}
