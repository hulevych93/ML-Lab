#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <cmath>
#include <chrono>

#include "game.h"
#include "agent.h"

class MinMaxAgent final : public Agent {
public:
    MinMaxAgent(const char player) : m_player(player) {}

    constexpr static int ALPHA = -999999;
    constexpr static int BETA = 999999;

    QAction chooseAction(const Board& game) const override {
        int bestScore = -999;
        QAction bestMove;

        const auto now = std::chrono::steady_clock::now();

        auto board = game.getBoard();

//        if(board[1][1] == Board::EMPTY_CELL) {
//            return {1, 1};
//        }

        for (int i = 0; i < Board::BOARD_SIZE; ++i) {
            for (int j = 0; j < Board::BOARD_SIZE; ++j) {
                if (board[i][j] == Board::EMPTY_CELL) {
                    board[i][j] = m_player;
                    int currentScore = minimax(board, 0, false);
                    board[i][j] = Board::EMPTY_CELL;
                    if (currentScore > bestScore) {
                        bestScore = currentScore;
                        bestMove = std::make_pair(i, j);
                    }
                }
            }
        }

        const auto point = std::chrono::steady_clock::now();
        const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(point - now);
        if(diff.count() > 10) {
            std::abort();
        }

        return bestMove;
    }

    // Function to check if a player has won the game
    static bool checkWin(const std::vector<std::vector<char>>& board, char player) {
        // Check rows and columns
        for (int i = 0; i < Board::BOARD_SIZE; ++i) {
            if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) ||
                (board[0][i] == player && board[1][i] == player && board[2][i] == player)) {
                return true;
            }
        }
        // Check diagonals
        if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) ||
            (board[0][2] == player && board[1][1] == player && board[2][0] == player)) {
            return true;
        }
        return false;
    }

    // Function to check if the board is full
    static bool boardFull(const std::vector<std::vector<char>>& board) {
        for (int i = 0; i < Board::BOARD_SIZE; ++i) {
            for (int j = 0; j < Board::BOARD_SIZE; ++j) {
                if (board[i][j] == Board::EMPTY_CELL) {
                    return false;
                }
            }
        }
        return true;
    }

    // Function to evaluate the board state
    int evaluate(const std::vector<std::vector<char>>& board) const {
        if (checkWin(board, m_player)) {
            return 1;
        } else if (checkWin(board, m_player == Board::FIRST_PLAYER ? Board::SECOND_PLAYER : m_player)) {
            return -1;
        } else {
            return 0;
        }
    }

    // Minimax algorithm with alpha-beta pruning
    int minimax(std::vector<std::vector<char>>& board, int depth, bool isMaximizing, int alpha = ALPHA, int beta = BETA) const {
        int score = evaluate(board);

        if (score != 0) {
            return score;
        }

        if (boardFull(board)) {
            return 0;
        }

        if (isMaximizing) {
            int maxScore = -999;
            for (int i = 0; i < Board::BOARD_SIZE; ++i) {
                for (int j = 0; j < Board::BOARD_SIZE; ++j) {
                    if (board[i][j] == Board::EMPTY_CELL) {
                        board[i][j] = m_player;
                        int currentScore = minimax(board, depth + 1, false, alpha, beta);
                        board[i][j] = Board::EMPTY_CELL;
                        maxScore = std::max(maxScore, currentScore);
                        alpha = std::max(alpha, currentScore);
                        if (beta <= alpha) {
                            break;
                        }
                    }
                }
            }
            return maxScore;
        } else {
            int minScore = 999;
            for (int i = 0; i < Board::BOARD_SIZE; ++i) {
                for (int j = 0; j < Board::BOARD_SIZE; ++j) {
                    if (board[i][j] == Board::EMPTY_CELL) {
                        board[i][j] = m_player == Board::FIRST_PLAYER ? Board::SECOND_PLAYER : m_player;
                        int currentScore = minimax(board, depth + 1, true, alpha, beta);
                        board[i][j] = Board::EMPTY_CELL;
                        minScore = std::min(minScore, currentScore);
                        beta = std::min(beta, currentScore);
                        if (beta <= alpha) {
                            break;
                        }
                    }
                }
            }
            return minScore;
        }
    }

private:
    const char m_player;
};
