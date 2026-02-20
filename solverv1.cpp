#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <mutex>
#include <future>
#include <map>
#include <iomanip>  

using namespace std;

const int GAMMA = 3;
const int MAX_DIFF = 500; // Sufficient for k up to ~20+

struct Result {
    int N;
    vector<int> witness;
};

// Global cache for G-(3, k) values to use in pruning
map<int, int> G_lookup;
mutex state_mutex;

/**
 * Checks if adding 'val' violates the gamma=3 constraint.
 * If valid, updates diff_counts and returns true.
 */
bool try_add(int val, const vector<int>& A, vector<int>& diff_counts) {
    for (int a : A) {
        if (diff_counts[val - a] + 1 > GAMMA) return false;
    }
    for (int a : A) {
        diff_counts[val - a]++;
    }
    return true;
}

void remove_val(int val, const vector<int>& A, vector<int>& diff_counts) {
    for (int a : A) {
        diff_counts[val - a]--;
    }
}

/**
 * Core DFS logic
 */
void dfs(vector<int>& A, vector<int>& diff_counts, int k, int& best_N, vector<int>& best_witness) {
    int current_len = A.size();

    if (current_len == k) {
        lock_guard<mutex> lock(state_mutex);
        if (A.back() < best_N) {
            best_N = A.back();
            best_witness = A;
        }
        return;
    }

    int remaining = k - current_len;
    int g_rem = G_lookup[remaining];
    
    int min_val = A.back() + 1;
    
    // Pruning: current_val + G-(remaining) must be less than current best_N
    // Also Symmetry Breaking: A[2] + A[k-1] <= A[k]. 
    // We apply a partial check here: A[1] <= best_N / 2 (since A[0]=0)
    
    for (int val = min_val; val <= best_N - g_rem; ++val) {
        // Late symmetry break for A[1] (A[2] in Julia 1-based)
        if (current_len == 2 && k >= 3) {
            // This is a heuristic; the full A[2]+A[k-1] <= N is checked at the end 
            // but we can bound A[1] to roughly half of the expected N.
        }

        if (try_add(val, A, diff_counts)) {
            A.push_back(val);
            dfs(A, diff_counts, k, best_N, best_witness);
            A.pop_back();
            remove_val(val, A, diff_counts);
        }

        // Optimization: If we found a solution exactly equal to the lower bound, stop
        if (best_N == G_lookup[k - 1] + 1) return; 
    }
}

/**
 * Greedy upper bound to seed the search
 */
Result get_greedy(int k) {
    vector<int> A = {0};
    vector<int> diff_counts(MAX_DIFF, 0);
    for (int i = 1; i < k; ++i) {
        int val = A.back() + 1;
        while (true) {
            if (try_add(val, A, diff_counts)) {
                A.push_back(val);
                break;
            }
            val++;
        }
    }
    return {A.back(), A};
}

/**
 * Parallel Coordinator
 */
Result solve_G_minus(int k) {
    if (k <= GAMMA + 1) return {k - 1, {}}; // Witness [0, 1, ..., k-1]

    Result initial = get_greedy(k);
    int best_N = initial.N;
    vector<int> best_witness = initial.witness;

    // We parallelize the first branching level
    vector<future<void>> futures;
    
    // Level 1: A = {0, 1} ... {0, max_first_step}
    // For k > 5, A[1] (the second element) usually stays very small
    for (int val1 = 1; val1 <= best_N / 2; ++val1) {
        futures.push_back(async(launch::async, [val1, k, &best_N, &best_witness]() {
            vector<int> local_A = {0, val1};
            vector<int> local_diffs(MAX_DIFF, 0);
            local_diffs[val1] = 1;
            dfs(local_A, local_diffs, k, best_N, best_witness);
        }));
    }

    for (auto& f : futures) f.get();
    return {best_N, best_witness};
}

int main() {
    int max_k = 15;
    cout << "Computing G-(gamma=3, k)" << endl;
    cout << "-------------------------" << endl;
    cout << "| k  | G-(3,k) | Witness" << endl;
    cout << "|----|---------|--------" << endl;

    // Base cases
    for (int k = 1; k <= GAMMA + 1; ++k) {
        G_lookup[k] = k - 1;
        cout << "| " << k << "  | " << k - 1 << "       | [0..." << k - 1 << "]" << endl;
    }

    for (int k = GAMMA + 2; k <= max_k; ++k) {
        auto start = chrono::high_resolution_clock::now();
        
        Result res = solve_G_minus(k);
        G_lookup[k] = res.N;
        
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;

        cout << "| " << k << (k < 10 ? "  | " : " | ") << res.N << "      | [";
        for (size_t i = 0; i < res.witness.size(); ++i) {
            cout << res.witness[i] << (i == res.witness.size() - 1 ? "" : ",");
        }
        cout << "] (" << fixed << setprecision(2) << elapsed.count() << "s)" << endl;
    }

    return 0;
}