#include "game.h"
#include "qvalues_agent.h"
#include "minmax_agent.h"

#include <fstream>
#include <chrono>

const int NUM_EPISODES = 30000;
const double LEARNING_RATE = 0.01;
const double DISCOUNT_FACTOR = 0.8;

void ticTacToeLearningOfFirstPlayer(QValuesAgent& firstPlayer, Agent& secondPlayer, const int episodes)
{
    for (int i = 0; i < episodes; ++i) {

        if(i % 10 == 0) {
            std::cout << i << std::endl;
        }

        const auto expRate = double(episodes - i) / episodes;

        Board game;
        auto nextState = game.toString();

        while (true) {
            const auto stateBeforeAction = nextState;
            const auto action = firstPlayer.chooseAction(game, expRate);

            game.move(action, Board::FIRST_PLAYER);
            nextState = game.toString();

            if(game.isOver()) {
                firstPlayer.updateQValues(stateBeforeAction, "", action,
                              game.getAggressiveReward(Board::FIRST_PLAYER), LEARNING_RATE, DISCOUNT_FACTOR);
                break;
            }

            game.move(secondPlayer.chooseAction(game), Board::SECOND_PLAYER);
            nextState = game.toString();

            if(!game.isOver()) {
                firstPlayer.updateQValues(stateBeforeAction, nextState, action, 0.0f, LEARNING_RATE, DISCOUNT_FACTOR);
            } else {
                firstPlayer.updateQValues(stateBeforeAction, "", action, -1.0f, LEARNING_RATE, DISCOUNT_FACTOR);
                break;
            }
        }
    }
}

void ticTacToeLearningOfSecondPlayer(QValuesAgent& secondPlayer, Agent& firstPlayer, const int episodes)
{
    auto now = std::chrono::steady_clock::now();
    for (int i = 0; i < episodes; ++i) {

        if(i % 10 == 0) {
            const auto point = std::chrono::steady_clock::now();
            const auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(point - now);
            std::cout << i << ": " << diff.count() << " mills" << std::endl;
            now = point;
        }

        const auto expRate = double(episodes - i) / episodes;

        Board game;
        auto nextState = game.toString();
        auto stateBeforeAction = nextState;

        QAction action;

        while (true) {
            game.move(firstPlayer.chooseAction(game), Board::FIRST_PLAYER);
            nextState = game.toString();

            if(game.checkWin(Board::FIRST_PLAYER)) {
                secondPlayer.updateQValues(stateBeforeAction, "", action, -1.0f, LEARNING_RATE, DISCOUNT_FACTOR); //
                break;
            } else if(game.checkDraw()) {
                secondPlayer.updateQValues(stateBeforeAction, "", action,
                                           game.getDefensiveReward(Board::SECOND_PLAYER), LEARNING_RATE, DISCOUNT_FACTOR); //
                break;
            } else {
                secondPlayer.updateQValues(stateBeforeAction, nextState, action, 0.0f, LEARNING_RATE, DISCOUNT_FACTOR);
            }

            stateBeforeAction = nextState;
            action = secondPlayer.chooseAction(game, expRate);
            game.move(action, Board::SECOND_PLAYER);
            nextState = game.toString();

            if(game.isOver()) {
                secondPlayer.updateQValues(stateBeforeAction, "", action,
                                          game.getDefensiveReward(Board::SECOND_PLAYER), LEARNING_RATE, DISCOUNT_FACTOR);
                break;
            }
        }
    }
}

void humanMove(Board& game, const char player) {
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

const char getOponent(const char player) {
    return (player == Board::FIRST_PLAYER) ? Board::SECOND_PLAYER : Board::FIRST_PLAYER;
}

void playTicTacToe(const char humanPlayer, const Agent& aiAgent) {
    Board game;

    std::cout << "You are " << humanPlayer << ". ";
    std::cout << "The board is empty. Let's start!" << std::endl;
    game.print();

    char currentPlayer = Board::FIRST_PLAYER;
    while (true) {
        if (currentPlayer == humanPlayer) {
            humanMove(game, currentPlayer);
        } else {
            const auto& action = aiAgent.chooseAction(game);
            game.move(action, currentPlayer);
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

        currentPlayer = getOponent(currentPlayer); // Switch players
    }
}

void testTicTacToeAgent(const char targetPlayer, const Agent& aiAgent, const Agent& opponent) {
    int wins = 0;
    int draws = 0;
    static const int GamesCount = 10000U;

    for(int i = 0; i < GamesCount; ++i) {
        Board game;
        char currentPlayer = Board::FIRST_PLAYER;
        while (!game.isOver()) {
            if (currentPlayer == targetPlayer) {
                const auto& action = aiAgent.chooseAction(game);
                game.move(action, currentPlayer);
            } else {
                const auto& action = opponent.chooseAction(game);
                game.move(action, currentPlayer);
            }

            if (game.checkWin(targetPlayer)) {
                ++wins;
                break;
            } else if (game.checkDraw()) {
                ++draws;
                break;
            }

            currentPlayer = getOponent(currentPlayer); // Switch players
        }
    }

    std::cout << "WinRate: " << double(wins) / GamesCount << std::endl;
    std::cout << "WinRate(+Draws): " << double(wins + draws) / GamesCount << std::endl;
}

int main() {
    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::cout << "Let's play Tic Tac Toe!" << std::endl;
    std::cout << "Choose your player: X or O: ";

    char humanPlayer = 'O';
   // std::cin >> humanPlayer;
    if (humanPlayer != 'X' && humanPlayer != 'O') {
        std::cout << "Invalid choice! Please choose 'X' or 'O'." << std::endl;
        return -1;
    }

    QValuesAgent aiAgent;
    Agent* opponent;

    if(0) {
        opponent = new MinMaxAgent{humanPlayer};
    } else {
        opponent = new RandomAgent{};
    }

    if(humanPlayer == Board::FIRST_PLAYER) {
        ticTacToeLearningOfSecondPlayer(aiAgent, *opponent, NUM_EPISODES);

        std::ofstream debug("second_player_qtree.txt");
        aiAgent.print(debug);
    } else {
        ticTacToeLearningOfFirstPlayer(aiAgent, *opponent, NUM_EPISODES);

        std::ofstream debug("first_player_qtree.txt");
        aiAgent.print(debug);
    }

    testTicTacToeAgent(getOponent(humanPlayer), aiAgent, *opponent);

    delete opponent;

    return 0;
}
