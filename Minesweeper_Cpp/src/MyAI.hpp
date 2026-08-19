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
#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <set>
#include <algorithm>
#include <cstdint>

using namespace std;

class MyAI : public Agent
{
public:
    /* Provided code cannot be edited */
    MyAI ( int _rowDimension, int _colDimension, int _totalMines, int _agentX, int _agentY );

    Action getAction ( int number ) override;
    /* End of provided code*/

    // Just a simple way to keep track of each tile's info
    struct TileInfo {
        bool covered = true;
        bool flagged = false;
        bool alreadyAddedToQueue = false;
        int mineCount = -1; // the number revealed on the tile
    };

    // A group of tiles that all affect each other
    struct BoardComponent {
        vector<pair<int, int>> edgeTiles; // tiles we're trying to figure out
        vector<pair<int, int>> numbersNearEdge; // the revealed numbers helping us
        vector<uint64_t> validMineLayouts; // bitmask of where mines could be
        vector<vector<int>> tileToNumberIndex; // which numbers are next to which tile
    };

    vector<vector<TileInfo>> board;
    queue<Action> thingsToDo;
    set<pair<int, int>> revealedTiles; 
    queue<pair<int, int>> tilesToCheck;
    int flagsPlaced;

    // Helper stuff
    bool isInside(int x, int y);
    vector<pair<int, int>> getAdjacent(int x, int y);
    void findSafeMoves();
    pair<int, int> makeAGuess();

    // The heavy lifting logic
    void handleComponents();
    void groupTilesIntoComponents(vector<BoardComponent>& groups, vector<pair<int, int>>& lonelyTiles);
    void solveThisGroup(BoardComponent& group, int totalMinesLeft);
    void tryAllCombinations(BoardComponent& group, int tileIdx, uint64_t maskSoFar, vector<int>& labelsLeft, vector<int>& neighborsLeft, int minesUsedSoFar, int totalMinesLeft);
    
    // Math engine for probabilities
    struct Probabilities {
        map<pair<int, int>, double> mineProbForTile;
        double isolatedTileProb;
        bool isPossible;
        
        vector<vector<double>> distributionPerGroup;
        vector<double> combinedDP;
        double totalWaysToArrangeBoard;
    };
    Probabilities calculateProbs();
    double combinations(int n, int r); // nCr
    
    Probabilities lastCalculatedProbs;
    
    vector<BoardComponent> currentGroups;
    vector<pair<int, int>> currentLonelyTiles;
    
    // For tie-breaking when probabilities are the same
    double calculateBenefit(int tx, int ty, const Probabilities& p);
    map<int, double> getLikelyLabels(int tx, int ty, const Probabilities& p);
    int countFutureCertainMoves(int tx, int ty, int possibleLabel);

    bool showDebug = false;
    void drawBoard();
};

#endif //MINE_SWEEPER_CPP_SHELL_MYAI_HPP
