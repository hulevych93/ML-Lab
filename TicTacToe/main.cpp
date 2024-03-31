#include <iostream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <optional>

const int BOARD_SIZE = 3;
const char EMPTY_CELL = '-';

const char SECOND_PLAYER = 'O';
const char FIRST_PLAYER = 'X';

const int NUM_EPISODES = 50000;
const double LEARNING_RATE = 0.1;
const double DISCOUNT_FACTOR = 0.9;

using QValue = double;
using QAction = std::pair<int, int>;

struct pairhash {
public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const
    {
        return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
    }
};

std::ostream& operator<<(std::ostream& ss, const QAction& action) {
    ss << "(" << action.first << ", " << action.second << ")";
    return ss;
}

using QValues = std::unordered_map<QAction, QValue, pairhash>;

using QValuesList = std::vector<QAction>;

using GameState = std::string;
using QTable = std::unordered_map<GameState, QValues>;

class Game final {
public:
    Game() : m_board(BOARD_SIZE, std::vector<char>(BOARD_SIZE, EMPTY_CELL)) {
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

    std::string getBoard() const {
        std::string boardString;
        for (int i = 0; i < m_size; ++i) {
            for (int j = 0; j < m_size; ++j) {
                boardString += m_board[i][j];
            }
        }
        return boardString;
    }

    void move(const std::pair<int, int>& action, const char player) {
        m_board[action.first][action.second] = player;
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

    bool checkDraw() const {
        for (const auto& row : m_board) {
            for (char cell : row) {
                if (cell == EMPTY_CELL) return false;
            }
        }
        return true;
    }

    double getAggressiveReward(const char player) const {
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

    double getDefensiveReward(const char player) const {
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

    std::vector<std::pair<int, int>> getAvailableActions() const {
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

private:
    std::vector<std::vector<char>> m_board;
    const int m_size = BOARD_SIZE;
};

std::ostream& printBoardFromString(std::ostream& ss, const std::string& boardString) {
    for (int i = 0; i < 9; ++i) {
        if (i % 3 == 0 && i != 0) {
            ss << std::endl;
            ss << "- - - - -" << std::endl;
        }
        if (i % 3 != 0) {
            ss << " | ";
        }
        ss << boardString[i];
    }
    ss << std::endl;
    return ss;
}

QValues getAvailable(const std::vector<std::pair<int, int>>& available, const QValues& values) {
    QValues availableValues;
    for(const auto& action: available) {
        const auto qValueIter = values.find(action);
        if(qValueIter != values.cend()) {
            availableValues.emplace(qValueIter->first, qValueIter->second);
        }
    }
    return availableValues;
}

QValuesList getAllBest(const QValues& values) {
    QValuesList list;
    const auto elemIter = std::max_element(values.cbegin(), values.cend(), [](const QValues::value_type& left,
                                                                                const QValues::value_type& right) {
        return left.second < right.second;
    });
    for(const auto value: values) {
        if(value.second == elemIter->second) {
            list.emplace_back(value.first);
        }
    }
    return list;
}

std::optional<std::pair<int, int>> findBest(const Game& game, const QTable& qtable)
{
    const auto state = game.getBoard();
    const auto qValuesIter = qtable.find(state);
    if(qValuesIter != qtable.cend()) {
        const auto& qValues = getAvailable(game.getAvailableActions(), qValuesIter->second);
        const auto best = getAllBest(qValues);
        return best[rand() % best.size()];
    }
    return std::nullopt;
}

std::pair<int, int> getRandomAction(const Game& game) {
    const auto actions = game.getAvailableActions();
    return actions[rand() % actions.size()];
}

std::pair<int, int> chooseAction(const Game& game, const QTable& qtable, double exploitationRate) {
    std::pair<int, int> action;
    if (rand() / static_cast<double>(RAND_MAX) < exploitationRate) {
        action = getRandomAction(game);
    } else {
        if(const auto actionOpt = findBest(game, qtable)) {
            action = *actionOpt;
        } else {
            action = getRandomAction(game);
        }
    }
    return action;
}

void updateQValues(QTable& qTable,
                   const std::string& state,
                   const std::string& nextState,
                   const std::pair<int, int>& action) {
    auto& qValues = qTable[state];
    auto& qValue = qValues[action];

    double maxQValue = 0;
    const auto qNextValuesIter = qTable.find(nextState);
    if(qNextValuesIter != qTable.cend()) {
      const auto& qNextValues = qNextValuesIter->second;
      for (const auto& qNextValue : qNextValues) {
          maxQValue = std::max(maxQValue, qNextValue.second);
      }
    }

    qValue += LEARNING_RATE * (DISCOUNT_FACTOR * maxQValue - qValue);
}

void updateQValues(QTable& qTable,
                   const std::string& state,
                   const std::pair<int, int>& action,
                   const double reward) {
    auto& qValues = qTable[state];
    auto& qValue = qValues[action];

    qValue += LEARNING_RATE * reward;
}

void ticTacToeLearning(QTable& firstPlayer, QTable& secondPlayer, const int episodes) {
    for (int i = 0; i < episodes; ++i) {
        if(i % 10000 == 0) {
            std::cout << i << std::endl;
        }

        Game game;

        auto stateFirstPlayer = game.getBoard();
        auto stateSecondPlayer = stateFirstPlayer;

        QAction firstPlayerAction, secondPlayerAction;

        auto nextState = game.getBoard();
        double exploitationRate = double(i) / episodes;

        while (true) {
            stateFirstPlayer = nextState;
            firstPlayerAction = chooseAction(game, firstPlayer, exploitationRate);

            game.move(firstPlayerAction, FIRST_PLAYER);
            nextState = game.getBoard();

            auto isDraw = game.checkDraw();
            const auto isWinFP = game.checkWin(FIRST_PLAYER);
            if(isDraw || isWinFP) {
                updateQValues(firstPlayer, stateFirstPlayer, firstPlayerAction,
                              game.getAggressiveReward(FIRST_PLAYER));
                break;
            } else {
                updateQValues(secondPlayer, stateSecondPlayer, nextState, secondPlayerAction);
            }

            stateSecondPlayer = nextState;
            secondPlayerAction = chooseAction(game, secondPlayer, exploitationRate);

            game.move(secondPlayerAction, SECOND_PLAYER);
            nextState = game.getBoard();

            isDraw = game.checkDraw();
            const auto isWinSP = game.checkWin(SECOND_PLAYER);
            if(isDraw || isWinSP) {
                updateQValues(secondPlayer, stateSecondPlayer, secondPlayerAction, game.getDefensiveReward(SECOND_PLAYER));
                break;
            } else {
                updateQValues(firstPlayer, stateFirstPlayer, nextState, firstPlayerAction);
            }
        }
    }
}

int main() {
    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    QTable firstPlayer;
    QTable secondPlayer;
    ticTacToeLearning(firstPlayer, secondPlayer, NUM_EPISODES);

    // Print the first player Q-table
    std::cout << "Q-table: " << firstPlayer.size() << std::endl;
    for (const auto& entry : firstPlayer) {
        std::cout << entry.first << std::endl;
        printBoardFromString(std::cout, entry.first);
        for(const auto action : entry.second) {
            std::cout << action.first << " - " << action.second
            << std::endl;
        }
    }

    return 0;
}
