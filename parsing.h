#pragma once

#include "objects.h"
#include "include/optional.hpp"
#include "include/nlohmann/json.hpp"
#include <fstream>
#include <vector>
#include <cmath>
#include <iostream>

void errorMsg(const string& msg) {
    cout << "Ошибка: " <<  msg << endl;
    ofstream file_out ("error.txt");
    file_out << "Ошибка: " << msg << endl;
    system("pause");
}

optional<Json> readSettings() { // NOTE: optional appears in C++17. It was possible to do with NULL-exception
    ifstream fin("setting.json");

    if(!fin.is_open()) {
        // NOTE: with some compilation settings, the executable file is placed in a new folder.
        // You also need to reach out to the settings from it
        fin.open("./../setting.json");
        if(!fin.is_open()) {
            errorMsg("Не найден файл настроек");
            return {};
        }
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