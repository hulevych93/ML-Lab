#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <cstdlib>
#include <limits>
#include <sstream>

using QValue = double;
using QAction = std::pair<int, int>;
using QActionList = std::vector<QAction>;

class Board final {
public:
    constexpr static const int BOARD_SIZE = 3;
    constexpr static const char EMPTY_CELL = '-';
    constexpr static const char SECOND_PLAYER = 'O';
    constexpr static const char FIRST_PLAYER = 'X';

    Board() : m_board(BOARD_SIZE, std::vector<char>(BOARD_SIZE, EMPTY_CELL)) {
    }

    std::string toString() const {
        std::string boardString;
        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                boardString += m_board[i][j];
            }
        }
        return boardString;
    }

    std::string print() const {
        std::stringstream ss;
        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                ss << m_board[i][j];
                if (j < m_size - 1) {
                    ss << " | ";
                }
            }
            ss << std::endl;
            if (i < m_size - 1) {
                for (int j = 0; j < m_size * 4 - 1; ++j) {
                    ss << "-";
                }
                ss << std::endl;
            }
        }
        return ss.str();
    }

    void move(const QAction& action, const char player) {
        m_board[action.first][action.second] = player;
    }

    bool checkAction(const QAction& action) const {
        const auto row = action.first;
        const auto col = action.second;
        return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && m_board[row][col] == EMPTY_CELL;
    }

    bool checkWin(const char player) const {
        // Check rows and columns
        for (int i = 0; i < m_size; ++i) {
            bool rowWin = true;
            bool colWin = true;
            for (int j = 0; j < m_size; ++j) {
                if (m_board[i][j] != player) rowWin = false; // Check row
                if (m_board[j][i] != player) colWin = false; // Check column
            }
            if (rowWin || colWin) return true;
        }
        // Check diagonals
        bool diag1Win = true;
        bool diag2Win = true;
        for (int i = 0; i < m_size; ++i) {
            if (m_board[i][i] != player) diag1Win = false; // Check diagonal 1
            if (m_board[i][m_size - 1 - i] != player) diag2Win = false; // Check diagonal 2
        }
        return diag1Win || diag2Win;
    }

    bool isOver() const {
        return checkDraw() || checkWin(FIRST_PLAYER) || checkWin(SECOND_PLAYER);
    }

    bool checkDraw() const {
        for (const auto& row : m_board) {
            for (char cell : row) {
                if (cell == EMPTY_CELL) return false;
            }
        }
        return true;
    }

    QValue getAggressiveReward(const char player) const {
        auto reward = 0.0f;
        if (checkWin(player)) {
            reward = 1.0f;
        } else if (checkDraw()) {
            reward = 0.5f;
        } else {
            reward = 0.0f;
        }
        return reward;
    }

    QValue getDefensiveReward(const char player) const {
        auto reward = 0.0f;
        if (checkDraw()) {
            reward = 1.0f;
        } else if (checkWin(player)) {
            reward = 0.5f;
        } else {
            reward = 0.0f;
        }
        return reward;
    }

    QActionList getAvailableActions() const {
        std::vector<std::pair<int, int>> actions;
        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                if (m_board[i][j] == EMPTY_CELL) {
                    actions.push_back(std::make_pair(i, j));
                }
            }
        }
        return actions;
    }

    QAction getRandomAction() const {
        const auto actions = getAvailableActions();
        return actions[rand() % actions.size()];
    }

    std::vector<std::vector<char>> getBoard() const {
        return m_board;
    }

private:
    std::vector<std::vector<char>> m_board;
    const int m_size = BOARD_SIZE;
};
