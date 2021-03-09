#ifndef SLIDEVALVEENGINE_H
#define SLIDEVALVEENGINE_H

#include <tuple>
#include <cmath>

typedef struct
{
    double topPort[2];      // distance from valve neutral position to the lower edge [0] and upper edge[1] of the top steam port ( both positive )
    double botPort[2];      // distance from valve neutral position to the lower edge [0] and upper edge[1] of the top steam port ( both negative)
    double exPort[2];       // distance from valve neutral position to the lower edge [0] and upper edge[1] of the exahust port ( lower negative, upper positive)
} s_valvePorts;

typedef struct
{
    double topLand[2];      // distance from slide center to the top land edges [0]= inside edge, [1] = outside edge (both positive)
    double botLand[2];      // distance from slide center to the bottom land edges [0]= inside edge, [1] = outside edge (both negative)
} s_dValve;

typedef struct
{
    double bore;                // bore of the engine
    double stroke;              // stroke of the engine
    double conRod;              // length (pin to pin) of the piston connecting rod
    double valveTravel;         // total valve travel (twice the eccentric offset)
    double valveConRod;         // length (eccentric offset point to pin) of the valve connecting rod
    double eccentricAdvance;    // advance in radians of the eccentric from the crankshaft. (0 is TDC for crank, extream top position for valve)
    s_valvePorts valvePorts;    // valve ports
    s_dValve valveSlide;        // D-valve slider
} s_engineParams;

enum class ErrorEnum{
    none,
    error
};

enum class CycleEnum{
    intake, expansion, exahust, compression
};

namespace SVE {
    double rad2Deg(double rad);
    double deg2Rad(double deg);
}

/*!
 * Holds functional parameters for a sliding valve, double acting engine and provides methods for calculating cycle information.
 */
class SlideValveEngine
{
public:
    SlideValveEngine();
    SlideValveEngine(s_engineParams params);
    ~SlideValveEngine();

    ErrorEnum setEngineParams(s_engineParams newParams);    // Validates the new parameters, and only sets them if they are ok
    s_engineParams getEngineParams();
    ErrorEnum validateSettings(s_engineParams params);      // checks engine parameters for serious errors (like con rod shorter than stroke)
    double crankRad2Stroke(double rad);                         // returns the piston offset from TDC for the given crank angle in radians
    double crankDeg2Stroke(double deg);                         // returns the piston offset from TDC for the given crank angle in degrees
    double stroke2CrankRad(double pos);                         // returns the crank position in radians(forward stroke solution, 0..PI) for a given piston offset from TDC.
    double stroke2CrankRad(double pos, bool ret);
    double stroke2CrankDeg(double pos);
    double stroke2CrankDeg(double pos, bool ret);

    std::array<double, 4> criticalPointsRad(bool ret);
    std::array<double, 4> criticalPointsDeg(bool ret);
    double crankRadInlet(bool ret);                             // returns the crank position when the steam port opens in radians. if ret is true, returns the value for the return stroke
    double crankDegInlet(bool ret);
    double crankRadCutoff(bool ret);                            // returns the crank position when the steam port closes in radians. if ret is true, returns the value for the return stroke
    double crankDegCutoff(bool ret);
    double crankRadRelease(bool ret);                           // returns the crank position when the exahust port opens in radians. if ret is true, returns the value for the return stroke
    double crankDegRelease(bool ret);
    double crankRadCompression(bool ret);                       // returns the crank position when the exahust port closes in radians. if ret is true, returns the value for the return stroke
    double crankDegCompression(bool ret);

    // returns the cycle region corresponding to the crank position. if ret is true, calculates for return stroke.
    CycleEnum crankRad2Cycle(double rad, bool ret);
    CycleEnum crankDeg2Cycle(double deg, bool ret);

    double nextCycleRad(double rad, bool ret);                  // returns the first crank position > rad which is in the next cycle region
    double nextCycleDeg(double deg, bool ret);                  // returns the first crank position > deg which is in the next cycle region

    double inletVolume(bool ret);                               // returns the volume swept by the piston during inlet. if ret is true gives value for return stroke
    double expansionVolume(bool ret);
    double compressionVolume(bool ret);

private:
    s_engineParams _engineParams;    
};

#endif // SLIDEVALVEENGINE_H
