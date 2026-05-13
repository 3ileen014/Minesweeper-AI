// ======================================================================
// FILE:        MyAI.hpp
//
// AUTHOR:      Eileen Kang, Nicolas Fuller
//
// DESCRIPTION: This file contains your agent class, which you will
//              implement. You are responsible for implementing the
//              'getAction' function and any helper methods you feel you
//              need.
//
// NOTES:       - If you are having trouble understanding how the shell
//                works, look at the other parts of the code, as well as
//                the documentation.
//
//              - You are only allowed to make changes to this portion of
//                the code. Any changes to other portions of the code will
//                be lost when the tournament runs your code.
// ======================================================================

#ifndef MINE_SWEEPER_CPP_SHELL_MYAI_HPP
#define MINE_SWEEPER_CPP_SHELL_MYAI_HPP

#include "Agent.hpp"
#include <iostream> // temporary use
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <algorithm>

using namespace std;

class MyAI : public Agent
{
public:
    /* Provided code cannot be edited */
    MyAI ( int _rowDimension, int _colDimension, int _totalMines, int _agentX, int _agentY );

    Action getAction ( int number ) override;
    /* End of provided code*/

    struct Tile {
        bool covered = true;
        bool flagged = false;
        bool inQueue = false;
        int label = -1;
    };


    vector<vector<Tile>> internalBoard;
    queue<Action> actionQueue;
    set<pair<int, int>> uncoveredTiles; // Set of uncovered tiles to check rules on

    bool isInBounds(int x, int y);
    vector<pair<int, int>> getNeighbors(int x, int y);
    void findCertainMoves();
};

#endif //MINE_SWEEPER_CPP_SHELL_MYAI_HPP
