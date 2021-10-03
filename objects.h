#pragma once

#include "include/kdtree.h"

using namespace std;

using Json = nlohmann::json;
using namespace experimental;

struct Vision {
    double viewAngle;
    double sinViewAngle;
    double cosViewAngle;
    bool   isAngleTooBig = false;

    double distance{};

    void parseViewAngle() {
        if (viewAngle > 180) {
            // NOTE: if the viewing angle is >180 degrees, then the algorithm considers the unobserved area
            viewAngle = 360 - viewAngle;
            isAngleTooBig = true;
        }

        // Pi / 180 convert angle to radians; / 2 - we need half of angle to look clockwise and counterclockwise from view
        viewAngle = (viewAngle * M_PI / 180) / 2;
        sinViewAngle = sin(viewAngle);
        cosViewAngle = cos(viewAngle);
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vision, viewAngle, distance) // NOTE: Used for Json-parser
};

struct Position {
    double x;
    double y;

    bool operator==(const Position& p) const {
        return x == p.x && y == p.y;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position, x, y) // NOTE: Used for Json-parser
};

class Unit {
private:
    Position getViewSectorStart(Vision vision) const {
        // NOTE: x and y are orthonormal vectors. To get the point, we need to perform the operation described below
        double startX = viewDirection.x * vision.cosViewAngle - viewDirection.y * vision.sinViewAngle;
        double startY = viewDirection.y * vision.cosViewAngle + viewDirection.x * vision.sinViewAngle;

        return { startX, startY };
    }

    Position getViewSectorEnd(Vision vision) const {
        // NOTE: x and y are orthonormal vectors. To get the point, we need to perform the operation described below
        double endX = viewDirection.x * vision.cosViewAngle + viewDirection.y * vision.sinViewAngle;
        double endY = viewDirection.y * vision.cosViewAngle - viewDirection.x * vision.sinViewAngle;

        return { endX, endY };
    }

    static bool isClockwise(Position p1, Position p2) {
        // NOTE: https://stackoverflow.com/questions/13652518/efficiently-find-points-inside-a-circle-sector
        return p1.y * p2.x - p1.x * p2.y > 0;
    }

    bool isInsideViewAngle(Position point, Position sectorStart, Position sectorEnd) const {
        Position relatedPoint = { point.x - position.x, point.y - position.y };

        // NOTE: at that moment we knew, that point inside view distance.
        //       We need to understand - if the point is inside the circle segment.
        return isClockwise(sectorStart, relatedPoint) && !isClockwise(sectorEnd, relatedPoint);
    }

public:
    unsigned id{};
    string name;
    Position position{};
    Position viewDirection{};

    Unit() = default;

    Unit(unsigned id, string name, double positionX, double positionY, double viewDirectionX, double viewDirectionY)
            : id(id),
              name(move(name)),
              position({ positionX, positionY }),
              viewDirection({ viewDirectionX, viewDirectionY }) {}

    Unit(unsigned id, string name, Position position, Position viewDirection)
            : id(id),
              name(move(name)),
              position(position),
              viewDirection(viewDirection) {}

    void drawUnit(sf::RenderWindow& window) const {
        sf::CircleShape unit(UNIT_SCALE, 3);
        unit.setFillColor(sf::Color::White);

        auto angle = viewDirection.y >= 0 ?
                atan(viewDirection.x / viewDirection.y) / M_PI * 180. :
                180. + atan(viewDirection.x / viewDirection.y) / M_PI * 180.;

        unit.scale(0.5, 1.);
        unit.setOrigin(UNIT_SCALE, UNIT_SCALE);
        unit.setRotation(static_cast<float>(angle));
        unit.setPosition(SCREEN_SIZE / 2 + position.x * POSITION_SCALE, SCREEN_SIZE / 2 - position.y * POSITION_SCALE);

        window.draw(unit);
    }

    void drawUnitRadius(sf::RenderWindow& window, Vision& vision) const {
        float radius = POSITION_SCALE * vision.distance ;

        sf::CircleShape ring(radius);
        ring.setFillColor(sf::Color(0, 0, 0, 0));
        ring.setOutlineColor(sf::Color::Red);
        ring.setOutlineThickness(2.);

        ring.setOrigin(radius, radius);
        ring.setPosition(position.x * POSITION_SCALE + SCREEN_SIZE / 2, -position.y * POSITION_SCALE + SCREEN_SIZE / 2);

        window.draw(ring);
    }

    unsigned int calcVisibleNeighborsNum(Vision vision, const vector<Unit>& allUnits, kdt::KDTree<Unit>& kdTree) {
        vector<int> neighbourIndexes = kdTree.radiusSearch(*this, vision.distance);

        Position sectorStart{}, sectorEnd{};
        if (vision.isAngleTooBig) {
            // NOTE: sector must be mirrored when angle too big
            sectorStart = getViewSectorEnd(vision);
            sectorEnd = getViewSectorStart(vision);
        }
        else {
            sectorStart = getViewSectorStart(vision);
            sectorEnd = getViewSectorEnd(vision);
        }

        unsigned int visibleUnitsNum = 0;

        #pragma omp parallel for reduction(+:visibleUnitsNum)
        for (int index = 0; index < neighbourIndexes.size(); index++) { //NOTE: This notation used for omp parallel - it doesn't works with another
            if (allUnits[neighbourIndexes[index]].id != this->id) {
                bool isNeighborInsideViewAngle = isInsideViewAngle(allUnits[neighbourIndexes[index]].position, sectorStart, sectorEnd);

                if (vision.isAngleTooBig && !isNeighborInsideViewAngle || // NOTE: the unobserved sector is being checked
                    !vision.isAngleTooBig && isNeighborInsideViewAngle)   // NOTE: the observed sector is being checked
                        visibleUnitsNum++;
            }
        }

        return visibleUnitsNum;
    }

    bool haveUniqPosition(vector<Unit>& allUnits) const {
        return !any_of(allUnits.begin(), allUnits.end(), [this](auto unit){
            return unit.id != id && unit.position.x == position.x && unit.position.y == position.y;
        });
    }

    static const int DIM = 2; // NOTE: Used for kdTree library - how many dimensions in our vectors

    double operator[] (size_t i) const {
        // NOTE: Used for kdTree library - indexing dimensions
        if (i == 0) return position.x;
        else if (i == 1) return position.y;
        else
            assert("array subscript out of range");
        return 0;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Unit, name, position, viewDirection) // NOTE: Used for Json-parser
};
