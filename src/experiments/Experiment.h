#pragma once

#include <string>

class Experiment {
public:
    virtual ~Experiment() = default;

    virtual std::string name() const = 0;
    virtual void run() = 0;
};
