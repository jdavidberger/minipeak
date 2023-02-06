#pragma once

#include <iostream>
#include <chrono>

struct Timeit {
    decltype(std::chrono::_V2::system_clock::now()) start;
    std::string name;
    std::string hz_label;
    int cnt = 0;
    double scale = 1.;
    Timeit(const std::string& name, double scale = 1, const std::string& hz_label = "hz") : name(name), hz_label(hz_label), scale(scale) {
        start = std::chrono::_V2::system_clock::now();
    }

    bool under(float s) {
        cnt++;
        auto end = std::chrono::_V2::system_clock::now();
        auto secs = ((std::chrono::duration<float>) (end - start)).count();
        return secs < s;
    }

    ~Timeit() {
        auto end = std::chrono::_V2::system_clock::now();
        float hz = (float) (cnt) / ((std::chrono::duration<float>) (end - start)).count();
        printf("%-48s %12.3f %s %8.3fms\n", name.c_str(), hz * scale, hz_label.c_str(), 1000. / hz);
    }
};