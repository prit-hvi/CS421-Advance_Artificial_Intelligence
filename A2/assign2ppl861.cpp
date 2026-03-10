#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <queue>
#include <set>
#include <iomanip>

enum Strategy {
    BT = 1,
    FC = 2,
    MAC = 3
};

struct Constraint {
    int xi, xj; 
    std::vector<std::vector<bool>> incompatible; 

    Constraint(int _xi = -1, int _xj = -1, int d = 0)
        : xi(_xi), xj(_xj), incompatible(d, std::vector<bool>(d, false)) {}
};

struct CSP {
    int n; // number of variables
    int d; // domain size
    int m; // number of constraints
    int t; // incompatible tuples per constraint
    double p, alpha, r;

    std::vector<Constraint> constraints;
    std::vector<std::vector<int>> consIndex; 
    std::vector<std::vector<int>> neighbors; 

    std::mt19937 rng;

    CSP(int n_, double p_, double alpha_, double r_)
        : n(n_), p(p_), alpha(alpha_), r(r_), consIndex(n_, std::vector<int>(n_, -1)),
          neighbors(n_) {
        std::random_device rd;
        rng.seed(rd());
        buildRBInstance();
    }

    void buildRBInstance() {
        d = (int) std::round(std::pow((double)n, alpha));
        if (d < 1) d = 1;
        m = (int) std::round(r * n * std::log((double)n));
        if (m < 1) m = 1;
        t = (int) std::round(p * d * d);
        if (t < 0) t = 0;
        if (t > d * d) t = d * d;

        std::uniform_int_distribution<int> varDist(0, n - 1);
        std::set<std::pair<int,int>> chosenPairs;

        while ((int)chosenPairs.size() < m) {
            int i = varDist(rng);
            int j = varDist(rng);
            if (i == j) continue;
            if (i > j) std::swap(i, j);
            auto pr = std::make_pair(i, j);
            if (chosenPairs.insert(pr).second) {
                // New constraint
                constraints.emplace_back(i, j, d);
                int idx = (int)constraints.size() - 1;
                consIndex[i][j] = consIndex[j][i] = idx;
                neighbors[i].push_back(j);
                neighbors[j].push_back(i);
            }
        }

        std::uniform_int_distribution<int> valDist(0, d - 1);
        for (Constraint &c : constraints) {
            std::set<std::pair<int,int>> bad;
            while ((int)bad.size() < t) {
                int a = valDist(rng);
                int b = valDist(rng);
                bad.insert(std::make_pair(a, b));
            }
            for (auto &pr : bad) {
                c.incompatible[pr.first][pr.second] = true;
            }
        }
    }

    bool hasConstraint(int i, int j) const {
        return consIndex[i][j] != -1;
    }

    const Constraint& getConstraint(int i, int j) const {
        return constraints[consIndex[i][j]];
    }

    bool isIncompatible(int i, int val_i, int j, int val_j) const {
        int idx = consIndex[i][j];
        if (idx == -1) return false; // no constraint -> always compatible
        const Constraint &c = constraints[idx];
        if (i == c.xi) {
            return c.incompatible[val_i][val_j];
        } else {
            return c.incompatible[val_j][val_i];
        }
    }
};
// Just a helper function to print
void printCSP(const CSP &csp) {
    std::cout << "===== Generated RB CSP Instance =====\n";
    std::cout << "Number of variables (n): " << csp.n << "\n";
    std::cout << "Constraint tightness (p): " << csp.p << "\n";
    std::cout << "Constant alpha: " << csp.alpha << "\n";
    std::cout << "Constant r: " << csp.r << "\n";

    double pt = 1.0 - std::exp(-csp.alpha / csp.r);
    std::cout << "Phase transition p_t = 1 - e^{-alpha/r} = " << pt << "\n";

    std::cout << "Domain size (n^alpha): " << csp.d << "\n";
    std::cout << "Number of constraints (r * n ln n): " << csp.m << "\n";
    std::cout << "Number of incompatible tuples per constraint (p * d^2): " << csp.t << "\n";

    std::cout << "Variables: {";
    for (int i = 0; i < csp.n; ++i) {
        std::cout << "X" << i;
        if (i + 1 < csp.n) std::cout << ", ";
    }
    std::cout << "}\n";

    std::cout << "Domain: {";
    for (int v = 0; v < csp.d; ++v) {
        std::cout << v;
        if (v + 1 < csp.d) std::cout << ", ";
    }
    std::cout << "}\n";

    std::cout << "Constraints (incompatible tuples):\n";
    for (const Constraint &c : csp.constraints) {
        std::cout << "(X" << c.xi << ", X" << c.xj << "): ";
        bool first = true;
        for (int a = 0; a < csp.d; ++a) {
            for (int b = 0; b < csp.d; ++b) {
                if (c.incompatible[a][b]) {
                    if (!first) std::cout << " ";
                    std::cout << "(" << a << "," << b << ")";
                    first = false;
                }
            }
        }
        std::cout << "\n";
    }
    std::cout << "=====================================\n\n";
}

// Check consistency of assigning var -> val with respect to already assigned variables
bool isConsistentAssignment(const CSP &csp,
                            const std::vector<int> &assignment,
                            int var, int val) {
    for (int i = 0; i < csp.n; ++i) {
        if (assignment[i] != -1) {
            if (csp.isIncompatible(i, assignment[i], var, val)) {
                return false;
            }
        }
    }
    return true;
}

// Find next unassigned variable (simple left-to-right)
int selectUnassignedVariable(const std::vector<int> &assignment) {
    for (int i = 0; i < (int)assignment.size(); ++i) {
        if (assignment[i] == -1) return i;
    }
    return -1;
}

// AC-3 helpers

bool hasSupport(const CSP &csp,
                const std::vector<std::vector<bool>> &domains,
                int xi, int val_i, int xj) {
    // if no constraint, always supported
    if (!csp.hasConstraint(xi, xj)) return true;

    for (int val_j = 0; val_j < csp.d; ++val_j) {
        if (!domains[xj][val_j]) continue;
        if (!csp.isIncompatible(xi, val_i, xj, val_j)) {
            return true;
        }
    }
    return false;
}

// revise(D_i, D_j) for arc (i,j); returns true if domain[i] was reduced
bool revise(const CSP &csp,
            std::vector<std::vector<bool>> &domains,
            int i, int j,
            std::vector<std::pair<int,int>> &pruned) {
    if (!csp.hasConstraint(i, j)) return false;

    bool revised = false;
    for (int val_i = 0; val_i < csp.d; ++val_i) {
        if (!domains[i][val_i]) continue;
        if (!hasSupport(csp, domains, i, val_i, j)) {
            domains[i][val_i] = false;
            pruned.emplace_back(i, val_i);
            revised = true;
        }
    }
    return revised;
}

// AC-3 algorithm on current domains.
bool ac3(const CSP &csp,
         std::vector<std::vector<bool>> &domains,
         std::vector<std::pair<int,int>> &pruned) {
    std::queue<std::pair<int,int>> q;
    for (const Constraint &c : csp.constraints) {
        q.emplace(c.xi, c.xj);
        q.emplace(c.xj, c.xi);
    }

    while (!q.empty()) {
        auto arc = q.front(); q.pop();
        int i = arc.first;
        int j = arc.second;
        if (revise(csp, domains, i, j, pruned)) {
            bool empty = true;
            for (int v = 0; v < csp.d; ++v) {
                if (domains[i][v]) { empty = false; break; }
            }
            if (empty) return false;
            for (int k : csp.neighbors[i]) {
                if (k == j) continue;
                q.emplace(k, i);
            }
        }
    }
    return true;
}

// Forward checking: after assigning xi, prune inconsistent values from neighbor xj
bool forwardCheck(const CSP &csp,
                  int xi, int xj,
                  const std::vector<int> &assignment,
                  std::vector<std::vector<bool>> &domains,
                  std::vector<std::pair<int,int>> &pruned) {
    if (!csp.hasConstraint(xi, xj)) return true;

    int val_i = assignment[xi];
    for (int val_j = 0; val_j < csp.d; ++val_j) {
        if (!domains[xj][val_j]) continue;
        if (csp.isIncompatible(xi, val_i, xj, val_j)) {
            domains[xj][val_j] = false;
            pruned.emplace_back(xj, val_j);
        }
    }

    // Check if domain of xj became empty
    for (int v = 0; v < csp.d; ++v) {
        if (domains[xj][v]) return true;
    }
    return false;
}

// Main search (BT / FC / MAC)
bool backtrackSearch(const CSP &csp,
                     std::vector<int> &assignment,
                     std::vector<std::vector<bool>> &domains,
                     Strategy strat,
                     std::vector<std::pair<int,int>> &pruned) {
    int var = selectUnassignedVariable(assignment);
    if (var == -1) {
        // all assigned
        return true;
    }

    for (int val = 0; val < csp.d; ++val) {
        if (!domains[var][val]) continue;

        if (!isConsistentAssignment(csp, assignment, var, val)) continue;

        assignment[var] = val;
        int prevPrunedSize = (int)pruned.size();

        if (strat == FC || strat == MAC) {
            for (int v = 0; v < csp.d; ++v) {
                if (v == val) continue;
                if (domains[var][v]) {
                    domains[var][v] = false;
                    pruned.emplace_back(var, v);
                }
            }
        }

        bool ok = true;

        if (strat == FC || strat == MAC) {
            for (int nb : csp.neighbors[var]) {
                if (assignment[nb] == -1) {
                    if (!forwardCheck(csp, var, nb, assignment, domains, pruned)) {
                        ok = false;
                        break;
                    }
                }
            }
            if (ok && strat == MAC) {
                if (!ac3(csp, domains, pruned)) {
                    ok = false;
                }
            }
        }

        if (ok) {
            if (backtrackSearch(csp, assignment, domains, strat, pruned)) {
                return true;
            }
        }

        for (int i = (int)pruned.size() - 1; i >= prevPrunedSize; --i) {
            int v = pruned[i].first;
            int val_v = pruned[i].second;
            domains[v][val_v] = true;
        }
        pruned.resize(prevPrunedSize);

        assignment[var] = -1;
    }

    return false;
}

int main() {
    int n;
    double p, alpha, r;
    std::cout << "Enter n, p, alpha, r (space-separated): ";
    if (!(std::cin >> n >> p >> alpha >> r)) {
        std::cerr << "Invalid input.\n";
        return 1;
    }

    std::cout << "Choose strategy: 1 = BT, 2 = FC, 3 = MAC (FLA): ";
    int stratInt;
    std::cin >> stratInt;
    Strategy strat;
    if (stratInt == 1) strat = BT;
    else if (stratInt == 2) strat = FC;
    else strat = MAC;

    std::cout << "Run AC before search? (0 = no, 1 = yes): ";
    int runAC;
    std::cin >> runAC;
    bool preAC = (runAC != 0);

    CSP csp(n, p, alpha, r);
    printCSP(csp);

    std::vector<std::vector<bool>> domains(n, std::vector<bool>(csp.d, true));
    std::vector<int> assignment(n, -1);
    std::vector<std::pair<int,int>> pruned;

    if (preAC) {
        std::cout << "Running global AC-3 before search...\n";
        auto ac_start = std::chrono::steady_clock::now();
        if (!ac3(csp, domains, pruned)) {
            auto ac_end = std::chrono::steady_clock::now();
            auto ac_ms = std::chrono::duration_cast<std::chrono::milliseconds>(ac_end - ac_start).count();
            std::cout << "AC-3 detected inconsistency. Instance unsatisfiable. (AC time: "
                      << ac_ms << " ms)\n";
            return 0;
        }
        auto ac_end = std::chrono::steady_clock::now();
        auto ac_ms = std::chrono::duration_cast<std::chrono::milliseconds>(ac_end - ac_start).count();
        std::cout << "AC-3 finished. Time: " << ac_ms << " ms\n";
        pruned.clear();
    }

    auto start = std::chrono::steady_clock::now();
    bool solved = backtrackSearch(csp, assignment, domains, strat, pruned);
    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (!solved) {
        std::cout << "No solution found (instance may be unsatisfiable).\n";
    } else {
        std::cout << "Solution found:\n";
        for (int i = 0; i < n; ++i) {
            std::cout << "X" << i << " = " << assignment[i] << "\n";
        }
    }
    std::cout << "Search time: " << ms << " ms\n";

    return 0;
}
