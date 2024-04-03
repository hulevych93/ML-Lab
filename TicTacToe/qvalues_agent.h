#include <iostream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <cstdlib>
#include <limits>
#include <sstream>

#include "game.h"
#include "agent.h"

struct pairhash {
public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const
    {
        return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
    }
};

inline std::ostream& operator<<(std::ostream& ss, const QAction& action) {
    ss << "(" << action.first << ", " << action.second << ")";
    return ss;
}

class QValuesAgent final : public Agent {
    using QValues = std::unordered_map<QAction, QValue, pairhash>;
    using BoardState = std::string;
    using QTable = std::unordered_map<BoardState, QValues>;

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

    QAction findBestOrRandomAvailableAction(const Board& game) const
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
    void printAlternatives(const Board& game) const
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

    std::pair<int, int> chooseAction(const Board& game, const double exploration) {
        std::pair<int, int> action;
        if (rand() / static_cast<double>(RAND_MAX) < exploration) {
            action = game.getRandomAction();
        } else {
            action = findBestOrRandomAvailableAction(game);
        }
        return action;
    }

    std::pair<int, int> chooseAction(const Board& game) const override {
        return findBestOrRandomAvailableAction(game);
    }

    void updateQValues(const std::string& state,
                       const std::string& nextState,
                       const std::pair<int, int>& action,
                       const double reward,
                       const double learningRate,
                       const double discount) {
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

        qValue += learningRate * reward;

        if(maxQValue != 0) {
            qValue += learningRate* (discount * maxQValue - qValue);
        }
    }

    void print(std::ostream& ss) const {
        ss << "Q-table: " << m_qtable.size() << std::endl;
        for (const auto& entry : m_qtable) {
            ss << entry.first << std::endl;
            printBoardFromString(ss, entry.first);
            for(const auto action : entry.second) {
                ss << action.first << " - " << action.second
                          << std::endl;
            }
        }
    }

private:
    QTable m_qtable;
};

class RandomAgent final : public Agent {
public:
    QAction chooseAction(const Board& game) const override {
        return game.getRandomAction();
    }
};
