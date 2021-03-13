#ifndef SLIDEVALVEENGINE_H
#define SLIDEVALVEENGINE_H

#include <tuple>
#include <cmath>
#include <algorithm>


// valve measurements are from neutral position. neutral is positive direction form valve TDC
typedef struct
{
    double topPort[2];      // distance from valve neutral position to the lower edge [0] and upper edge[1] of the top steam port ( both negative, closer to valve TDC than neutral )
    double botPort[2];      // distance from valve neutral position to the lower edge [0] and upper edge[1] of the top steam port ( both positive, farther from valve TDC than neutral)
    double exPort[2];       // distance from valve neutral position to the lower edge [0] and upper edge[1] of the exahust port ( lower positive, upper negative)
} s_valvePorts;

typedef struct
{
    double topLand[2];      // distance from slide center to the top land edges [0]= inside edge, [1] = outside edge (both negative)
    double botLand[2];      // distance from slide center to the bottom land edges [0]= inside edge, [1] = outside edge (both positive)
} s_dValve;

typedef struct
{
    double bore;                // bore of the engine
    double stroke;              // stroke of the engine
    double conRod;              // length (pin to pin) of the piston connecting rod
    double valveTravel;         // total valve travel (twice the eccentric offset)
    double valveConRod;         // length (eccentric offset point to pin) of the valve connecting rod
    double eccentricAdvance;    // advance in degrees of the eccentric from the crankshaft. (0 is TDC for crank, extream top position for valve)
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
    double crank2Stroke(double deg, double stroke, double length);
    double stroke2Crank(double pos, double stroke, double length, bool ret);
    double addAngles(double deg1, double deg2);
    bool comparePointsLT(std::pair<double, int> point1, std::pair<double, int> point2);
    bool comparePointEQ(std::pair<double, int> point, double val);
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

    ErrorEnum setEngineParams(s_engineParams newParams);        // Validates the new parameters, and only sets them if they are ok
    s_engineParams getEngineParams();    

    std::array<double, 4> topCriticalPoints();
    std::array<double, 4> botCriticalPoints();
    std::array<double, 8> criticalPoints();

    double crankInlet(bool ret);
    double crankCutoff(bool ret);
    double crankRelease(bool ret);
    double crankCompression(bool ret);
    double stroke2Crank(double pos, bool ret);
    double crank2Stroke(double deg);
    double valvePos2Crank(double pos, bool ret);
    double crank2ValvePos(double deg);

    // returns the cycle region corresponding to the crank position. if ret is true, calculates for return stroke.
    CycleEnum crank2TopCycle(double deg);
    CycleEnum crank2BotCycle(double deg);

    // these functions return the next crank position after deg which hits a critical point
    int nextTopCriticalPoint(double deg);       // returns the index of the next top critical point after deg
    int nextBotCriticalPoint(double deg);       // returns the index of the next bottom critical point after deg

    // returns the volume swept by the piston during inlet. if ret is true gives value for return stroke
    double inletVolume(bool ret);

    double expansionVolume(bool ret);

    double compressionVolume(bool ret);

private:
    s_engineParams _engineParams;    
    // the critical points only change when engine parameters change. no need to calculate them every time
    ErrorEnum calcCriticalPoints(s_engineParams params);    // uses passed in engine parameters, if no error is encountered, updates the internal critical point values
    ErrorEnum validateSettings(s_engineParams params);      // checks engine parameters for serious errors (like con rod shorter than stroke)
    CycleEnum crank2Cycle(double deg, bool ret);
    int nextPoint(double deg, std::array<double, 4> points);    // returns the index of the next point (with wrap). index is into <points>
    double _criticalPoints[8];
    double _forwardValveNeutral;                            // angular position of the eccentric when the valve is in the neutral position (1/2 its total travel)
    double _returnValveNeutral;                            // angular position of the eccentric when the valve is in the neutral position (1/2 its total travel)
};

#endif // SLIDEVALVEENGINE_H
