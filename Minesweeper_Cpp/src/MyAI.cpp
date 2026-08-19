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
    flagsPlaced = 0;
    showDebug = false;

    // Set up our own board tracking
    board.resize(colDimension);
    for (int i = 0; i < colDimension; ++i) {
        board[i].resize(rowDimension);
    }
};

Agent::Action MyAI::getAction( int number )
{
    // Update the board with what we just found out
    if (number != -1) {
        board[agentX][agentY].covered = false;
        board[agentX][agentY].mineCount = number;
        board[agentX][agentY].alreadyAddedToQueue = false;
        revealedTiles.insert({agentX, agentY});
        tilesToCheck.push({agentX, agentY});
        
        // Check neighbors too because opening this might have made them solvable
        vector<pair<int, int>> nbrs = getAdjacent(agentX, agentY);
        for (auto const& n : nbrs) {
            if (!board[n.first][n.second].covered) {
                tilesToCheck.push(n);
            }
        }
    } else {
        board[agentX][agentY].alreadyAddedToQueue = false;
    }

    Action result;
    // Try to find obvious moves first
    if (thingsToDo.empty()) {
        findSafeMoves();
    }

    // If no obvious moves, we gotta do the hard math
    if (thingsToDo.empty()) {
        handleComponents();
    }

    // Do whatever is in our action queue
    if (!thingsToDo.empty()) {
        Action nextAction = thingsToDo.front();
        thingsToDo.pop();
        agentX = nextAction.x;
        agentY = nextAction.y;
        result = nextAction;
    } else {
        // nothing is certain, time to guess
        pair<int,int> guess = makeAGuess();

        if (guess.first != -1) {
            agentX = guess.first;
            agentY = guess.second;
            result = {UNCOVER, guess.first, guess.second};
        } else {
            // Absolute last resort, just find any random covered tile
            bool foundOne = false;
            for (int x = 0; x < colDimension; ++x) {
                for (int y = 0; y < rowDimension; ++y) {
                    if (board[x][y].covered && !board[x][y].flagged && !board[x][y].alreadyAddedToQueue) {
                        agentX = x;
                        agentY = y;
                        result = {UNCOVER, x, y};
                        foundOne = true;
                        break;
                    }
                }
                if (foundOne) break;
            }

            if (!foundOne) result = {LEAVE, -1, -1};
        }
    }

    if (showDebug) drawBoard();
    return result;
}

bool MyAI::isInside(int x, int y) {
    return x >= 0 && x < colDimension && y >= 0 && y < rowDimension;
}

vector<pair<int, int>> MyAI::getAdjacent(int x, int y) {
    vector<pair<int, int>> adj;
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (isInside(nx, ny)) {
                adj.push_back({nx, ny});
            }
        }
    }
    return adj;
}

// Basic rules: if mines near = unflagged neighbors, they're all mines.
// If mines near = 0, all neighbors are safe.
void MyAI::findSafeMoves() {
    while (!tilesToCheck.empty()) {
        pair<int, int> curr = tilesToCheck.front();
        tilesToCheck.pop();
        
        int x = curr.first;
        int y = curr.second;
        
        if (board[x][y].covered) continue;
        
        vector<pair<int, int>> adj = getAdjacent(x, y);
        int flagsNear = 0;
        vector<pair<int, int>> hiddenNear;

        for (auto const& n : adj) {
            if (board[n.first][n.second].flagged) {
                flagsNear++;
            } else if (board[n.first][n.second].covered && !board[n.first][n.second].alreadyAddedToQueue) {
                hiddenNear.push_back(n);
            }
        }

        int hiddenCount = hiddenNear.size();
        int minesRemainingHere = board[x][y].mineCount - flagsNear;

        if (minesRemainingHere == hiddenCount && hiddenCount > 0) {
            for (auto const& n : hiddenNear) {
                board[n.first][n.second].flagged = true;
                board[n.first][n.second].alreadyAddedToQueue = true;
                thingsToDo.push({FLAG, n.first, n.second});
                flagsPlaced++;
                
                // Flagging a tile might help its neighbors
                vector<pair<int, int>> flaggedAdj = getAdjacent(n.first, n.second);
                for (auto const& fa : flaggedAdj) {
                    if (!board[fa.first][fa.second].covered) {
                        tilesToCheck.push(fa);
                    }
                }
            }
        } else if (minesRemainingHere == 0 && hiddenCount > 0) {
            for (auto const& n : hiddenNear) {
                board[n.first][n.second].alreadyAddedToQueue = true;
                thingsToDo.push({UNCOVER, n.first, n.second});
            }
        }
    }
}

// standard combinations function for math
double MyAI::combinations(int n, int r) {
    if (r < 0 || r > n) return 0.0;
    if (r == 0 || r == n) return 1.0;
    if (r > n / 2) r = n - r;
    double res = 1.0;
    for (int i = 1; i <= r; ++i) {
        res = res * (n - i + 1) / (double)i;
    }
    return res;
}

// mathe engine that calculates probabilities for the whole board
MyAI::Probabilities MyAI::calculateProbs() {
    Probabilities p;
    p.isPossible = false;
    p.isolatedTileProb = 1.0;
    p.totalWaysToArrangeBoard = 0;

    int minesLeft = totalMines - flagsPlaced;
    int U = currentLonelyTiles.size();
    if (minesLeft < 0) return p;

    // Build distributions for each group of tiles
    p.distributionPerGroup.resize(currentGroups.size());
    for (size_t i = 0; i < currentGroups.size(); ++i) {
        if (currentGroups[i].validMineLayouts.empty()) {
            // If we didn't solve it, just guess something roughly right
            p.distributionPerGroup[i].assign(currentGroups[i].edgeTiles.size() + 1, 0.0);
            int roughEst = currentGroups[i].edgeTiles.size() / 3;
            if (roughEst >= (int)p.distributionPerGroup[i].size()) roughEst = p.distributionPerGroup[i].size() - 1;
            p.distributionPerGroup[i][roughEst] = 1.0;
            continue;
        }
        
        int mostMines = 0;
        for (uint64_t mask : currentGroups[i].validMineLayouts) {
            int m = __builtin_popcountll(mask);
            if (m > mostMines) mostMines = m;
        }
        
        p.distributionPerGroup[i].assign(mostMines + 1, 0.0);
        for (uint64_t mask : currentGroups[i].validMineLayouts) {
            p.distributionPerGroup[i][__builtin_popcountll(mask)] += 1.0;
        }
    }

    // Combine distributions with dynamic programming approach
    auto combineWays = [&](const vector<vector<double>>& dists) {
        vector<double> dp = {1.0};
        for (const auto& d : dists) {
            vector<double> next(dp.size() + d.size() - 1, 0.0);
            for (size_t j = 0; j < dp.size(); ++j) {
                if (dp[j] == 0) continue;
                for (size_t k = 0; k < d.size(); ++k) {
                    if (d[k] == 0) continue;
                    if (j + k < next.size()) next[j + k] += dp[j] * d[k];
                }
            }
            dp = next;
            if (dp.size() > (size_t)minesLeft + 1) dp.resize(minesLeft + 1);
        }
        return dp;
    };

    p.combinedDP = combineWays(p.distributionPerGroup);
    for (size_t m = 0; m < p.combinedDP.size(); ++m) {
        if (m <= (size_t)minesLeft && minesLeft - m <= (size_t)U) {
            p.totalWaysToArrangeBoard += p.combinedDP[m] * combinations(U, minesLeft - m);
        }
    }

    if (p.totalWaysToArrangeBoard <= 0.0) return p;
    p.isPossible = true;

    // Calculate probability for each tile in the groups
    for (size_t i = 0; i < currentGroups.size(); ++i) {
        if (currentGroups[i].validMineLayouts.empty()) continue;

        vector<vector<double>> others = p.distributionPerGroup;
        others.erase(others.begin() + i);
        vector<double> combinedOthers = combineWays(others);

        for (size_t t = 0; t < currentGroups[i].edgeTiles.size(); ++t) {
            map<int, double> layoutsWithMineAtT;
            for (uint64_t mask : currentGroups[i].validMineLayouts) {
                if ((mask >> t) & 1) {
                    layoutsWithMineAtT[__builtin_popcountll(mask)] += 1.0;
                }
            }

            double waysToHaveMineAtT = 0;
            for (auto const& pair : layoutsWithMineAtT) {
                int minesInThisGroup = pair.first;
                for (size_t minesInOthers = 0; minesInOthers < combinedOthers.size(); ++minesInOthers) {
                    int totalOnBorder = minesInThisGroup + minesInOthers;
                    if (totalOnBorder <= minesLeft && minesLeft - totalOnBorder <= U) {
                        waysToHaveMineAtT += pair.second * combinedOthers[minesInOthers] * combinations(U, minesLeft - totalOnBorder);
                    }
                }
            }
            p.mineProbForTile[currentGroups[i].edgeTiles[t]] = waysToHaveMineAtT / p.totalWaysToArrangeBoard;
        }
    }

    // Probability for the isolated tiles that aren't touching any numbers
    if (U > 0) {
        double totalMinesInIsolated = 0;
        for (size_t m = 0; m < p.combinedDP.size(); ++m) {
            if (m <= (size_t)minesLeft && minesLeft - m <= (size_t)U) {
                totalMinesInIsolated += p.combinedDP[m] * combinations(U, minesLeft - m) * (minesLeft - m);
            }
        }
        p.isolatedTileProb = (totalMinesInIsolated / p.totalWaysToArrangeBoard) / (double)U;
    }

    return p;
}

// This splits the board into groups of connected tiles and solves them
void MyAI::handleComponents() {
    currentGroups.clear();
    currentLonelyTiles.clear();
    groupTilesIntoComponents(currentGroups, currentLonelyTiles);
    
    int totalMinesLeft = totalMines - flagsPlaced;

    for (auto& g : currentGroups) {
        solveThisGroup(g, totalMinesLeft);
    }

    lastCalculatedProbs = calculateProbs();
    if (!lastCalculatedProbs.isPossible) return;

    // Check if any tiles are 100% or 0% mine
    for (auto const& entry : lastCalculatedProbs.mineProbForTile) {
        int x = entry.first.first;
        int y = entry.first.second;
        double prob = entry.second;

        if (prob >= 1.0 - 1e-9) {
            board[x][y].flagged = true;
            board[x][y].alreadyAddedToQueue = true;
            thingsToDo.push({FLAG, x, y});
            flagsPlaced++;
        } else if (prob <= 1e-9) {
            board[x][y].alreadyAddedToQueue = true;
            thingsToDo.push({UNCOVER, x, y});
        }
    }
}

// Smart guessing using probabilities and looking into the future!
pair<int,int> MyAI::makeAGuess() {
    if (!lastCalculatedProbs.isPossible) {
        lastCalculatedProbs = calculateProbs();
    }
    
    Probabilities& p = lastCalculatedProbs;
    if (!p.isPossible) {
        // Just pick the first thing we can find if math failed
        for (int x = 0; x < colDimension; ++x) {
            for (int y = 0; y < rowDimension; ++y) {
                if (board[x][y].covered && !board[x][y].flagged && !board[x][y].alreadyAddedToQueue) {
                    return {x, y};
                }
            }
        }
        return {-1, -1};
    }

    vector<pair<int, int>> candidates;
    double bestProb = 2.0;

    // Find tiles with the lowest mine probability
    for (auto const& entry : p.mineProbForTile) {
        double prob = entry.second;
        if (prob < bestProb - 1e-7) {
            bestProb = prob;
            candidates.clear();
            candidates.push_back(entry.first);
        } else if (abs(prob - bestProb) <= 1e-7) {
            candidates.push_back(entry.first);
        }
    }

    if (!currentLonelyTiles.empty()) {
        double prob = p.isolatedTileProb;
        if (prob < bestProb - 1e-7) {
            bestProb = prob;
            candidates = currentLonelyTiles;
        } else if (abs(prob - bestProb) <= 1e-7) {
            for (auto const& t : currentLonelyTiles) candidates.push_back(t);
        }
    }

    // Try to solve large groups that we gave up on earlier
    for (auto const& g : currentGroups) {
        if (g.validMineLayouts.empty()) {
            for (auto const& t : g.edgeTiles) {
                double guessProb = 0.33; // rough guess
                if (guessProb < bestProb - 1e-7) {
                    bestProb = guessProb;
                    candidates.clear();
                    candidates.push_back(t);
                } else if (abs(guessProb - bestProb) <= 1e-7) {
                    candidates.push_back(t);
                }
            }
        }
    }

    if (candidates.empty()) return {-1, -1};
    if (candidates.size() == 1) return candidates[0];

    // Tie-breaker: which tile gives us more info if we pick it
    pair<int, int> finalPick = candidates[0];
    double maxBenefit = -1.0;
    
    for (auto const& t : candidates) {
        double benefit = calculateBenefit(t.first, t.second, p);
        // Small bias towards corners
        bool isCorner = (t.first == 0 || t.first == colDimension - 1) && 
                         (t.second == 0 || t.second == rowDimension - 1);
        if (isCorner) benefit += 0.0001;

        if (benefit > maxBenefit) {
            maxBenefit = benefit;
            finalPick = t;
        }
    }
    return finalPick;
}

// This splits the frontier into separate sections so we can solve them easier
void MyAI::groupTilesIntoComponents(vector<BoardComponent>& groups, vector<pair<int, int>>& lonelyTiles) {
    vector<pair<int, int>> frontier;
    map<pair<int, int>, vector<pair<int, int>>> numToAdjFrontier;
    map<pair<int, int>, vector<pair<int, int>>> frontierToAdjNum;
    int coveredTotal = 0;

    for (int x = 0; x < colDimension; ++x) {
        for (int y = 0; y < rowDimension; ++y) {
            if (board[x][y].covered && !board[x][y].flagged && !board[x][y].alreadyAddedToQueue) {
                coveredTotal++;
                vector<pair<int, int>> neighbors = getAdjacent(x, y);
                bool isTouchingNumber = false;
                for (auto const& n : neighbors) {
                    if (!board[n.first][n.second].covered && board[n.first][n.second].mineCount > 0) {
                        isTouchingNumber = true;
                        frontierToAdjNum[{x, y}].push_back(n);
                        numToAdjFrontier[n].push_back({x, y});
                    }
                }
                if (isTouchingNumber) frontier.push_back({x, y});
                else lonelyTiles.push_back({x, y});
            }
        }
    }

    // If there aren't many tiles left, just treat them all as one big group since it should add more accuracy
    if (coveredTotal > 0 && coveredTotal <= 32) {
        BoardComponent finalGroup;
        set<pair<int, int>> relevantNums;
        for (auto const& t : frontier) {
            finalGroup.edgeTiles.push_back(t);
            for (auto const& n : frontierToAdjNum[t]) relevantNums.insert(n);
        }
        for (auto const& t : lonelyTiles) finalGroup.edgeTiles.push_back(t);
        lonelyTiles.clear();
        finalGroup.numbersNearEdge.assign(relevantNums.begin(), relevantNums.end());
        groups.push_back(finalGroup);
        return;
    }

    // BFS to find connected components
    set<pair<int, int>> visited;
    for (auto const& t : frontier) {
        if (visited.find(t) != visited.end()) continue;

        BoardComponent g;
        queue<pair<int, int>> q;
        q.push(t);
        visited.insert(t);
        set<pair<int, int>> groupNums;

        while (!q.empty()) {
            pair<int, int> curr = q.front();
            q.pop();
            g.edgeTiles.push_back(curr);

            for (auto const& n : frontierToAdjNum[curr]) {
                groupNums.insert(n);
                for (auto const& adjFrontierTile : numToAdjFrontier[n]) {
                    if (visited.find(adjFrontierTile) == visited.end()) {
                        visited.insert(adjFrontierTile);
                        q.push(adjFrontierTile);
                    }
                }
            }
        }
        g.numbersNearEdge.assign(groupNums.begin(), groupNums.end());
        groups.push_back(g);
    }
}

// Setting up the recursive backtracking for a group
void MyAI::solveThisGroup(BoardComponent& group, int totalMinesLeft) {
    if (group.edgeTiles.empty()) return;
    if (group.edgeTiles.size() > 40) return; // Too big to solve exactly, skip for now

    group.tileToNumberIndex.resize(group.edgeTiles.size());
    for (int i = 0; i < (int)group.edgeTiles.size(); ++i) {
        int tx = group.edgeTiles[i].first;
        int ty = group.edgeTiles[i].second;
        for (int j = 0; j < (int)group.numbersNearEdge.size(); ++j) {
            int nx = group.numbersNearEdge[j].first;
            int ny = group.numbersNearEdge[j].second;
            if (abs(tx - nx) <= 1 && abs(ty - ny) <= 1) {
                group.tileToNumberIndex[i].push_back(j);
            }
        }
    }

    vector<int> labelsLeft(group.numbersNearEdge.size());
    vector<int> neighborsLeft(group.numbersNearEdge.size(), 0);
    for (int i = 0; i < (int)group.numbersNearEdge.size(); ++i) {
        int x = group.numbersNearEdge[i].first;
        int y = group.numbersNearEdge[i].second;
        int flags = 0;
        int coveredInThisGroup = 0;
        vector<pair<int, int>> adj = getAdjacent(x, y);
        for (auto const& n : adj) {
            if (board[n.first][n.second].flagged) flags++;
            else if (board[n.first][n.second].covered) {
                for (auto const& gt : group.edgeTiles) {
                    if (gt == n) { coveredInThisGroup++; break; }
                }
            }
        }
        labelsLeft[i] = board[x][y].mineCount - flags;
        neighborsLeft[i] = coveredInThisGroup;
    }

    tryAllCombinations(group, 0, 0, labelsLeft, neighborsLeft, 0, totalMinesLeft);
}

// Standard backtracking to find all valid ways mines could be placed in a group
void MyAI::tryAllCombinations(BoardComponent& group, int tileIdx, uint64_t maskSoFar, vector<int>& labelsLeft, vector<int>& neighborsLeft, int minesUsedSoFar, int totalMinesLeft) {
    if (tileIdx == (int)group.edgeTiles.size()) {
        for (int val : labelsLeft) if (val != 0) return;
        group.validMineLayouts.push_back(maskSoFar);
        return;
    }

    int remainingInGroup = (int)group.edgeTiles.size() - tileIdx;
    int lonelyCount = (int)currentLonelyTiles.size();

    // 1: This tile is not a mine
    bool canBeSafe = true;
    for (int idx : group.tileToNumberIndex[tileIdx]) {
        if (neighborsLeft[idx] - 1 < labelsLeft[idx]) { canBeSafe = false; break; }
    }
    // Make sure we don't end up needing more mines than we have left
    if (canBeSafe && (totalMinesLeft - minesUsedSoFar > (remainingInGroup - 1) + lonelyCount)) {
        canBeSafe = false;
    }

    if (canBeSafe) {
        for (int idx : group.tileToNumberIndex[tileIdx]) neighborsLeft[idx]--;
        tryAllCombinations(group, tileIdx + 1, maskSoFar, labelsLeft, neighborsLeft, minesUsedSoFar, totalMinesLeft);
        for (int idx : group.tileToNumberIndex[tileIdx]) neighborsLeft[idx]++;
    }

    // 2: This tile is a mine
    bool canBeMine = (minesUsedSoFar < totalMinesLeft);
    if (canBeMine) {
        for (int idx : group.tileToNumberIndex[tileIdx]) {
            if (labelsLeft[idx] <= 0) { canBeMine = false; break; }
        }
    }

    if (canBeMine) {
        for (int idx : group.tileToNumberIndex[tileIdx]) {
            labelsLeft[idx]--;
            neighborsLeft[idx]--;
        }
        tryAllCombinations(group, tileIdx + 1, maskSoFar | (1ULL << tileIdx), labelsLeft, neighborsLeft, minesUsedSoFar + 1, totalMinesLeft);
        for (int idx : group.tileToNumberIndex[tileIdx]) {
            labelsLeft[idx]++;
            neighborsLeft[idx]++;
        }
    }
}

// finds out the probability of seeing each possible number (0-8) if we uncover a tile
map<int, double> MyAI::getLikelyLabels(int tx, int ty, const Probabilities& p) {
    map<int, double> labelLikelihoods;
    int groupIdx = -1;
    int tileIdx = -1;
    
    for (size_t i = 0; i < currentGroups.size(); ++i) {
        for (size_t j = 0; j < currentGroups[i].edgeTiles.size(); ++j) {
            if (currentGroups[i].edgeTiles[j] == make_pair(tx, ty)) {
                groupIdx = i;
                tileIdx = j;
                break;
            }
        }
        if (groupIdx != -1) break;
    }

    int minesLeft = totalMines - flagsPlaced;
    int U = currentLonelyTiles.size();

    if (groupIdx != -1) {
        BoardComponent& g = currentGroups[groupIdx];
        
        // Use dynamic programming to see how many global ways each seperate layout can be finished
        auto combineWays = [&](const vector<vector<double>>& dists) {
            vector<double> dp = {1.0};
            for (const auto& d : dists) {
                vector<double> next(dp.size() + d.size() - 1, 0.0);
                for (size_t j = 0; j < dp.size(); ++j) {
                    if (dp[j] == 0) continue;
                    for (size_t k = 0; k < d.size(); ++k) {
                        if (d[k] == 0) continue;
                        if (j + k < next.size()) next[j + k] += dp[j] * d[k];
                    }
                }
                dp = next;
                if (dp.size() > (size_t)minesLeft + 1) dp.resize(minesLeft + 1);
            }
            return dp;
        };

        vector<vector<double>> others = p.distributionPerGroup;
        others.erase(others.begin() + groupIdx);
        vector<double> combinedOthers = combineWays(others);

        double totalSafeWeight = 0;
        for (uint64_t mask : g.validMineLayouts) {
            if (((mask >> tileIdx) & 1) == 0) { // If tile is safe in this layout
                int minesInThis = __builtin_popcountll(mask);
                double ways = 0;
                for (size_t minesInOthers = 0; minesInOthers < combinedOthers.size(); ++minesInOthers) {
                    int totalOnBorder = minesInThis + minesInOthers;
                    if (totalOnBorder <= minesLeft && minesLeft - totalOnBorder <= U) {
                        ways += combinedOthers[minesInOthers] * combinations(U, minesLeft - totalOnBorder);
                    }
                }

                if (ways > 0) {
                    int val = 0;
                    vector<pair<int,int>> adj = getAdjacent(tx, ty);
                    for (auto const& n : adj) {
                        if (board[n.first][n.second].flagged) val++;
                        else {
                            for (size_t j = 0; j < g.edgeTiles.size(); ++j) {
                                if (g.edgeTiles[j] == n) { if ((mask >> j) & 1) val++; break; }
                            }
                        }
                    }
                    labelLikelihoods[val] += ways;
                    totalSafeWeight += ways;
                }
            }
        }
        if (totalSafeWeight > 0) {
            for (auto& entry : labelLikelihoods) entry.second /= totalSafeWeight;
        } else {
            labelLikelihoods[0] = 1.0;
        }
    } else {
        // For isolated tiles, use the alternative distribution
        vector<pair<int,int>> adj = getAdjacent(tx, ty);
        int baseMines = 0;
        int isoAdjCount = 0;
        for (auto const& n : adj) {
            if (board[n.first][n.second].flagged) baseMines++;
            else if (board[n.first][n.second].covered) isoAdjCount++;
        }

        double safeWeight = 0;
        if (U >= 1) {
            for (int k = 0; k <= isoAdjCount; ++k) {
                double weight = 0;
                for (size_t m = 0; m < p.combinedDP.size(); ++m) {
                    int m_iso = minesLeft - (int)m;
                    if (m_iso >= 0 && m_iso <= U - 1) { // -1 because current tile is safe
                        weight += p.combinedDP[m] * combinations(isoAdjCount, k) * combinations(U - 1 - isoAdjCount, m_iso - k);
                    }
                }
                if (weight > 0) {
                    labelLikelihoods[baseMines + k] = weight;
                    safeWeight += weight;
                }
            }
        }

        if (safeWeight > 0) {
            for (auto& entry : labelLikelihoods) entry.second /= safeWeight;
        } else {
            labelLikelihoods[baseMines] = 1.0;
        }
    }
    return labelLikelihoods;
}

// Simulates what would happen if we found a certain number
int MyAI::countFutureCertainMoves(int tx, int ty, int possibleLabel) {
    auto oldBoard = board;
    auto oldRevealed = revealedTiles;
    auto oldToCheck = tilesToCheck;
    
    queue<Action> oldQueue;
    while (!thingsToDo.empty()) {
        oldQueue.push(thingsToDo.front());
        thingsToDo.pop();
    }
    
    int oldFlags = flagsPlaced;

    board[tx][ty].covered = false;
    board[tx][ty].mineCount = possibleLabel;
    board[tx][ty].alreadyAddedToQueue = false;
    revealedTiles.insert({tx, ty});
    
    while(!tilesToCheck.empty()) tilesToCheck.pop();
    tilesToCheck.push({tx, ty});

    findSafeMoves();

    int found = thingsToDo.size();

    // Revert everything back to how it was
    board = oldBoard;
    revealedTiles = oldRevealed;
    tilesToCheck = oldToCheck;
    while (!thingsToDo.empty()) thingsToDo.pop();
    while (!oldQueue.empty()) {
        thingsToDo.push(oldQueue.front());
        oldQueue.pop();
    }
    flagsPlaced = oldFlags;

    return found;
}

// Calculates the expected number of guaranteed moves if we uncover this tile
double MyAI::calculateBenefit(int tx, int ty, const Probabilities& p) {
    map<int, double> likelihoods = getLikelyLabels(tx, ty, p);
    double expected = 0.0;
    for (auto const& entry : likelihoods) {
        int label = entry.first;
        double prob = entry.second;
        if (prob > 0) {
            int moves = countFutureCertainMoves(tx, ty, label);
            expected += prob * moves;
        }
    }
    return expected;
}

// debug feature
void MyAI::drawBoard() {
    cout << "   ";
    for (int x = 0; x < colDimension; ++x) {
        if (x < 10) cout << " " << x << " ";
        else cout << " " << x;
    }
    cout << endl;

    for (int y = rowDimension - 1; y >= 0; --y) {
        if (y < 10) cout << " " << y << " ";
        else cout << y << " ";

        for (int x = 0; x < colDimension; ++x) {
            if (board[x][y].flagged) cout << " F ";
            else if (board[x][y].covered) cout << " . ";
            else if (board[x][y].mineCount == 0) cout << "   ";
            else cout << " " << board[x][y].mineCount << " ";
        }
        cout << endl;
    }
    cout << "Mines left: " << totalMines - flagsPlaced << endl;
}
