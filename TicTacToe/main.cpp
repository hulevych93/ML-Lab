#include <iostream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <cstdlib>
#include <limits>
#include <sstream>

const int BOARD_SIZE = 3;
const char EMPTY_CELL = '-';

const char SECOND_PLAYER = 'O';
const char FIRST_PLAYER = 'X';

const int NUM_EPISODES = 1000000;
const double LEARNING_RATE = 0.01;
const double DISCOUNT_FACTOR = 0.9;

using QValue = double;
using QAction = std::pair<int, int>;
using QActionList = std::vector<QAction>;

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

class Game final {
public:
    Game() : m_board(BOARD_SIZE, std::vector<char>(BOARD_SIZE, EMPTY_CELL)) {
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

private:
    std::vector<std::vector<char>> m_board;
    const int m_size = BOARD_SIZE;
};

class Agent final {
    using QValues = std::unordered_map<QAction, QValue, pairhash>;
    using GameState = std::string;
    using QTable = std::unordered_map<GameState, QValues>;

    static std::ostream& printBoardFromString(std::ostream& ss, const std::string& boardString) {
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

    static QActionList getBestFromAvailable(const QActionList& available, const QValues& values) {
        QValues availableValues;
        for(const auto& action: available) {
            const auto qValueIter = values.find(action);
            if(qValueIter != values.cend()) {
                availableValues.emplace(qValueIter->first, qValueIter->second);
            }
        }

        QActionList bestValues;
        const auto elemIter = std::max_element(availableValues.cbegin(), availableValues.cend(), [](const QValues::value_type& left,
                                                                                  const QValues::value_type& right) {
            return left.second < right.second;
        });
        for(const auto value: availableValues) {
            if(value.second == elemIter->second) {
                bestValues.emplace_back(value.first);
            }
        }

        if(bestValues.empty()) {
            return available;
        }

        return bestValues;
    }

    QAction findBestOrRandomAvailableAction(const Game& game) const
    {
        const auto state = game.toString();
        const auto qValuesIter = m_qtable.find(state);
        const auto& availableActions = game.getAvailableActions();
        if(qValuesIter != m_qtable.cend()) {
            const auto& qValues = getBestFromAvailable(availableActions, qValuesIter->second);
            return qValues[rand() % qValues.size()];
        } else {
            return availableActions[rand() % availableActions.size()];
        }
    }

public:
    void printAlternatives(const Game& game) const
    {
        const auto state = game.toString();
        const auto qValuesIter = m_qtable.find(state);
        if(qValuesIter != m_qtable.cend()) {
            for(const auto action : qValuesIter->second) {
                std::cout << action.first << " - " << action.second
                          << std::endl;
            }
        }
    }

    std::pair<int, int> chooseAction(const Game& game, const double exploration) {
        std::pair<int, int> action;
        if (rand() / static_cast<double>(RAND_MAX) < exploration) {
            action = game.getRandomAction();
        } else {
            action = findBestOrRandomAvailableAction(game);
        }
        return action;
    }

    std::pair<int, int> chooseAction(const Game& game) const {
        return findBestOrRandomAvailableAction(game);
    }

    void updateQValues(const std::string& state,
                       const std::string& nextState,
                       const std::pair<int, int>& action,
                       const double reward) {
        auto& qValues = m_qtable[state];
        auto& qValue = qValues[action];

        double maxQValue = 0;
        const auto qNextValuesIter = m_qtable.find(nextState);
        if(qNextValuesIter != m_qtable.cend()) {
            const auto& qNextValues = qNextValuesIter->second;
            for (const auto& qNextValue : qNextValues) {
                maxQValue = std::max(maxQValue, qNextValue.second);
            }
        }

        qValue += LEARNING_RATE * (reward + DISCOUNT_FACTOR * maxQValue - qValue);
    }

    void print() const {
        std::cout << "Q-table: " << m_qtable.size() << std::endl;
        for (const auto& entry : m_qtable) {
            std::cout << entry.first << std::endl;
            printBoardFromString(std::cout, entry.first);
            for(const auto action : entry.second) {
                std::cout << action.first << " - " << action.second
                          << std::endl;
            }
        }
    }

private:
    QTable m_qtable;
};

void ticTacToeLearning(Agent& firstPlayer, Agent& secondPlayer, const int episodes) {
    for (int i = 0; i < episodes; ++i) {

        if(i % 10000 == 0) {
            std::cout << i << std::endl;
        }

        const auto explorationRate = double(episodes - i) / episodes;

        Game game;

        auto stateFirstPlayer = game.toString();
        auto stateSecondPlayer = stateFirstPlayer;

        QAction firstPlayerAction, secondPlayerAction;

        auto nextState = game.toString();

        while (true) {
            stateFirstPlayer = nextState;
            firstPlayerAction = firstPlayer.chooseAction(game, explorationRate);

            game.move(firstPlayerAction, FIRST_PLAYER);
            nextState = game.toString();

            if(game.isOver()) {
                firstPlayer.updateQValues(stateFirstPlayer, stateFirstPlayer, firstPlayerAction,
                              game.getAggressiveReward(FIRST_PLAYER));
                break;
            } else {
                secondPlayer.updateQValues(stateSecondPlayer, nextState, secondPlayerAction, 0.0f);
            }

            stateSecondPlayer = nextState;
            secondPlayerAction = secondPlayer.chooseAction(game, explorationRate);

            game.move(secondPlayerAction, SECOND_PLAYER);
            nextState = game.toString();

            if(game.isOver()) {
                secondPlayer.updateQValues(stateSecondPlayer, stateSecondPlayer, secondPlayerAction, game.getDefensiveReward(SECOND_PLAYER));
                break;
            } else {
                firstPlayer.updateQValues(stateFirstPlayer, nextState, firstPlayerAction, 0.0f);
            }
        }
    }
}

void humanMove(Game& game, const char player) {
    int row, col;
    std::cout << "Enter row (0-2) and column (0-2) to make your move: ";
    std::cin >> row >> col;
    if (game.checkAction(QAction{row, col})) {
        game.move(QAction{row, col}, player);
    } else {
        std::cout << "Invalid move! Please try again." << std::endl;
        humanMove(game, player);
    }
}

void aiMove(Game& game, const Agent& agent, const char player) {
    agent.printAlternatives(game);
    const auto& action = agent.chooseAction(game);
    game.move(action, player);
}

void playTicTacToe(const Agent& XAgent, const Agent& OAgent) {
    Game game;

    std::cout << "Let's play Tic Tac Toe!" << std::endl;
    std::cout << "Choose your player: X or O: ";

    char humanPlayer;
    std::cin >> humanPlayer;
    if (humanPlayer != 'X' && humanPlayer != 'O') {
        std::cout << "Invalid choice! Please choose 'X' or 'O'." << std::endl;
        return;
    }

    std::cout << "You are " << humanPlayer << ". ";
    std::cout << "The board is empty. Let's start!" << std::endl;
    game.print();

    char currentPlayer = FIRST_PLAYER;
    while (true) {
        if (currentPlayer == humanPlayer) {
            humanMove(game, currentPlayer);
        } else {
            aiMove(game, currentPlayer == FIRST_PLAYER ? XAgent : OAgent, currentPlayer);
        }

        std::cout << std::endl << "Current board state:" << std::endl;
        std::cout << game.print();

        if (game.checkWin(currentPlayer)) {
            std::cout << currentPlayer << " wins!" << std::endl;
            break;
        } else if (game.checkDraw()) {
            std::cout << "It's a draw!" << std::endl;
            break;
        }

        currentPlayer = (currentPlayer == FIRST_PLAYER) ? SECOND_PLAYER : FIRST_PLAYER; // Switch players
    }
}

int main() {
    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    Agent firstPlayer;
    Agent secondPlayer;
    ticTacToeLearning(firstPlayer, secondPlayer, NUM_EPISODES);

    firstPlayer.print();


    playTicTacToe(firstPlayer, secondPlayer);
    return 0;
}
