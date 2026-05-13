// ======================================================================
// FILE:        MyAI.cpp
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

#include "MyAI.hpp"

MyAI::MyAI ( int _rowDimension, int _colDimension, int _totalMines, int _agentX, int _agentY ) : Agent()
{
    rowDimension = _rowDimension;
    colDimension = _colDimension;
    totalMines = _totalMines;
    agentX = _agentX;
    agentY = _agentY;

    internalBoard.resize(colDimension);
    for (int i = 0; i < colDimension; ++i) {
        internalBoard[i].resize(rowDimension);
    }
};

Agent::Action MyAI::getAction( int number )
{
    if (number != -1) {
        internalBoard[agentX][agentY].covered = false;
        internalBoard[agentX][agentY].label = number;
        internalBoard[agentX][agentY].inQueue = false;
        uncoveredTiles.insert({agentX, agentY});
    } else {
        internalBoard[agentX][agentY].inQueue = false;
    }

    if (actionQueue.empty()) {
        findCertainMoves();
    }

    if (!actionQueue.empty()) {
        Action nextAction = actionQueue.front();
        actionQueue.pop();
        agentX = nextAction.x;
        agentY = nextAction.y;
        return nextAction;
    }

    // Fallback: Pick the first available covered, unflagged tile
    for (int x = 0; x < colDimension; ++x) {
        for (int y = 0; y < rowDimension; ++y) {
            if (internalBoard[x][y].covered && !internalBoard[x][y].flagged && !internalBoard[x][y].inQueue) {
                agentX = x;
                agentY = y;
                return {UNCOVER, x, y};
            }
        }
    }

    return {LEAVE, -1, -1};
}

bool MyAI::isInBounds(int x, int y) {
    return x >= 0 && x < colDimension && y >= 0 && y < rowDimension;
}

vector<pair<int, int>> MyAI::getNeighbors(int x, int y) {
    vector<pair<int, int>> neighbors;
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (isInBounds(nx, ny)) {
                neighbors.push_back({nx, ny});
            }
        }
    }
    return neighbors;
}

void MyAI::findCertainMoves() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto const& pos : uncoveredTiles) {
            int x = pos.first;
            int y = pos.second;
            
            vector<pair<int, int>> neighbors = getNeighbors(x, y);
            int numMarked = 0;
            vector<pair<int, int>> unmarkedNeighbors;

            for (auto const& n : neighbors) {
                if (internalBoard[n.first][n.second].flagged) {
                    numMarked++;
                } else if (internalBoard[n.first][n.second].covered && !internalBoard[n.first][n.second].inQueue) {
                    unmarkedNeighbors.push_back(n);
                }
            }

            int numUnmarked = unmarkedNeighbors.size();
            int effectiveLabel = internalBoard[x][y].label - numMarked;

            if (effectiveLabel == numUnmarked && numUnmarked > 0) {
                // Rule 1: All unmarked are mines
                for (auto const& n : unmarkedNeighbors) {
                    internalBoard[n.first][n.second].flagged = true;
                    internalBoard[n.first][n.second].inQueue = true;
                    actionQueue.push({FLAG, n.first, n.second});
                }
                changed = true;
            } else if (effectiveLabel == 0 && numUnmarked > 0) {
                // Rule 2: All unmarked are safe
                for (auto const& n : unmarkedNeighbors) {
                    internalBoard[n.first][n.second].inQueue = true;
                    actionQueue.push({UNCOVER, n.first, n.second});
                }
                changed = true;
            }
        }
    }
}