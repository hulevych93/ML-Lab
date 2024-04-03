#pragma once

#include <utility>

using QValue = double;
using QAction = std::pair<int, int>;

class Board;

class Agent {
public:
    virtual ~Agent() = default;

    virtual QAction chooseAction(const Board& game) const = 0;
};
