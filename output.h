#pragma once

#include "parsing.h"
#include "objects.h"
#include "SFML/Graphics.hpp"
#include "include/kdtree.h"
#include <fstream>
#include <cmath>
#include <iostream>

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