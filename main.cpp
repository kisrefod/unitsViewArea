#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <vector>
#include <unordered_set>
#include <random>
#include <utility>

#include <omp.h>

#include "include/nlohmann/json.hpp"
#include "include/optional.hpp"
#include "SFML/Graphics.hpp"

#define SCREEN_SIZE 1000.
#define UNIT_SCALE 20.
#define POSITION_SCALE 40.

#include "objects.h"
#include "parsing.h"
#include "output.h"

void testN(unsigned int N) {
    // NOTE: it was decided not to put the test in a separate file or program, because it is the only one
    // You can start it in main after "omp_set_nested(1);" line

    struct PositionHash {
        inline size_t operator()(const Position& pos) const {
            return hash<double>{}(pos.x) ^ hash<double>{}(pos.y);
        }
    };

    Vision vision;
    vision.viewAngle = (180 * M_PI / 180) / 2;
    vision.sinViewAngle = sin(vision.viewAngle);
    vision.cosViewAngle = cos(vision.viewAngle);
    vision.distance = 2;

    vector<Unit> units(N);
    unordered_set<Position, PositionHash> positions;

    random_device rd;
    mt19937 mt(rd());
    uniform_real_distribution<double> dist(-10, 10);

    for(int i = 0; i < units.size(); i++) {
        units[i].id = i;
        units[i].name = to_string(i);

        double x, y;
        do {
            x = dist(mt);
            y = dist(mt);
        } while (positions.find({x, y}) != positions.end());

        units[i].position = { x, y };
        positions.insert({x, y});

        x = dist(mt);
        y = dist(mt);
        units[i].viewDirection = { x, y };
    }

    cout << "DRAW" << endl;
    drawUnits(units, vision);
    cout << "STARTED" << endl;
    string result = getResultString(units, vision);
    printResult(result);
}

int main() {
    omp_set_nested(1); // NOTE: Allow nested parallelism

    auto optionalSettings = readSettings();

    if (!optionalSettings)
        return 0;

    Json settings = optionalSettings.value();
    auto optionalVision = parseVision(settings);

    if (!optionalVision)
        return 0;

    Vision vision = optionalVision.value();

    auto optionalUnits = parseUnits(settings);
    if (!optionalUnits)
        return 0;

    vector<Unit> units = optionalUnits.value();

    string result = getResultString(units, vision);
    printResult(result);

    drawUnits(units, vision);

    return 0;
}
