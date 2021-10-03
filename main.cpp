#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include <omp.h>
#include <unordered_set>
#include <random>
#include <utility>

#include "include/nlohmann/json.hpp"
#include "include/kdtree.h"
#include "include/optional.hpp"
#include "SFML/Graphics.hpp"

#define SCREEN_SIZE 1000.
#define UNIT_SCALE 20.
#define POSITION_SCALE 40.

#include "objects.h"

void errorMsg(const string& msg) {
    cout << "Ошибка: " <<  msg << endl;
    ofstream fout ("error.txt");
    fout << "Ошибка: " <<  msg << endl;
    system("pause");
}

optional<Json> readSettings() { // NOTE: optional appears in C++17. It was possible to do with NULL-exception
    ifstream fin("setting.json");

    if(!fin.is_open()) {
        errorMsg("Не найден файл настроек");
        return {};
    }

    Json settings;
    try {
        fin >> settings;
    } catch (exception& e) {
        errorMsg("Файл настроек содержит ошибку");
        return {};
    }

    return settings;
}

bool isVisionCorrect(Vision vision) {
    if (vision.viewAngle > 360) {
        errorMsg("Угол обзора больше 360 градусов");
        return false;
    } else if (vision.viewAngle < 0) {
        errorMsg("Угол обзора меньше 0 градусов");
        return false;
    }

    if(vision.distance < 0) {
        errorMsg("Расстояние зрения < 0");
        return false;
    }

    return true;
}

optional<Vision> parseVision(const Json& settings) { // NOTE: optional appears in C++17. It was possible to do with NULL-exception
    try {
        Vision vision = settings["Vision"].get<Vision>();

        if(!isVisionCorrect(vision))
            return {};

        vision.parseViewAngle();

        return vision;
    } catch (exception& e) {
        errorMsg("Поле Vision задано неправильно");
        return {};
    }
}

optional<vector<Unit>> parseUnits(const Json& settings) { // NOTE: optional appears in C++17. It was possible to do with NULL-exception
    try {
        vector<Unit> units = settings["Units"].get<vector<Unit>>();

        for (int i = 0; i < units.size(); i++) {
            units[i].id = i;
            if (!units[i].haveUniqPosition(units)) {
                errorMsg("У двух юнитов одинаковая позиция");
                return {};
            }

            if(units[i].viewDirection.x == 0 && units[i].viewDirection.y == 0) {
                errorMsg("Задан нулевой viewVector");
                return {};
            }
        }

        return units;
    } catch (exception& e) {
        errorMsg("Поле Vision задано неправильно");
        return {};
    }
}

string getResultString(vector<Unit>& units, Vision& vision) {
    // NOTE: KD-Tree is used to search for units in a radius with complexity O(log n)
    kdt::KDTree<Unit> kdTree (units);
    string resultStr;

    #pragma omp parallel for ordered
    for(int unitIndex = 0; unitIndex < units.size(); unitIndex++) {
        unsigned int visibleNeighborsNum = units[unitIndex].calcVisibleNeighborsNum(vision, units, kdTree);

        #pragma omp ordered
        resultStr += units[unitIndex].name + ": видит " + to_string(visibleNeighborsNum) + "\n";
    }

    return resultStr;
}

void printResult(const string& result) {
    ofstream fout("result.txt");
    fout << result;
}

void drawUnits(vector<Unit>& units, Vision& vision) {
    // NOTE: SFML graphics library is used here
    sf::RenderWindow window(sf::VideoMode(SCREEN_SIZE, SCREEN_SIZE), "Unigine Task");

    while (window.isOpen())
    {
        sf::Event event{};
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();

        for(const Unit& unit : units) {
            unit.drawUnit(window);
            unit.drawUnitRadius(window, vision);
        }

        window.display();
    }
}

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
