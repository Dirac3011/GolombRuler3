#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <thread>
#include <atomic>
#include <mutex>

using namespace std;

typedef __uint128_t bitmask;

const int g_min_span[] = {
    0, 0, 1, 2, 3, 5, 7, 9, 12, 15, 19, 24, 29, 35, 41, 49, 58, 67, 76, 85
};

// Shared state for threads
atomic<bool> found_global(false);
mutex cout_mutex;
int final_set[64];

bool backtrack(int* A, int size, bitmask m1, bitmask m2, bitmask m3, int k_target, int alpha) {
    // Check if another thread already found the solution
    if (found_global) return false;

    int last_val = A[size - 1];
    int remaining = k_target - size;

    if (last_val + g_min_span[remaining + 1] > alpha) return false;

    if (size == k_target - 1) {
        for (int i = 0; i < size; ++i) {
            int diff = alpha - A[i];
            if ((m3 >> diff) & 1) return false;
        }
        A[size] = alpha;
        return true;
    }

    int max_cand = alpha - g_min_span[remaining];
    
    for (int cand = last_val + 1; cand <= max_cand; ++cand) {
        bitmask new_diffs = 0;
        bool conflict = false;

        for (int i = 0; i < size; ++i) {
            int d = cand - A[i];
            if ((m3 >> d) & 1) {
                conflict = true;
                break;
            }
            new_diffs |= ((bitmask)1 << d);
        }

        if (!conflict) {
            bitmask next_m3 = m3 | (m2 & new_diffs);
            bitmask next_m2 = m2 | (m1 & new_diffs);
            bitmask next_m1 = m1 | new_diffs;

            A[size] = cand;
            if (backtrack(A, size + 1, next_m1, next_m2, next_m3, k_target, alpha)) return true;
        }
    }
    return false;
}

// Entry point for each thread
void worker(int a1_val, int k_target, int alpha) {
    int local_set[64];
    local_set[0] = 0;
    local_set[1] = a1_val;

    bitmask m1 = ((bitmask)1 << a1_val);
    bitmask m2 = 0, m3 = 0;

    if (backtrack(local_set, 2, m1, m2, m3, k_target, alpha)) {
        if (!found_global.exchange(true)) { // Ensure only one thread reports success
            lock_guard<mutex> lock(cout_mutex);
            for (int i = 0; i < k_target; ++i) final_set[i] = local_set[i];
        }
    }
}

void solve_parallel(int k) {
    int alpha = g_min_span[k];
    
    while (true) {
        auto start = chrono::high_resolution_clock::now();
        found_global = false;
        vector<thread> threads;

        // Symmetry breaking: A[1] only needs to go up to alpha/2
        int max_a1 = alpha / 2;

        for (int a1 = 1; a1 <= max_a1; ++a1) {
            threads.emplace_back(worker, a1, k, alpha);
        }

        for (auto& t : threads) t.join();

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = end - start;

        if (found_global) {
            cout << "\nFOUND! G-(3, " << k << ") = " << alpha;
            cout << " | Set: {";
            for (int i = 0; i < k; ++i) cout << final_set[i] << (i == k - 1 ? "" : ", ");
            cout << "} (" << elapsed.count() << "s)" << endl;
            break;
        } else {
            cout << "alpha = " << alpha << " failed (" << elapsed.count() << "s)" << endl;
            alpha++;
        }
    }
}

int main() {
    int start = 17;
    int end = 19;
    for (int k = start; k <= end; ++k) {
        cout << "Starting Multi-threaded Search for k=" << k << "..." << endl;
        solve_parallel(k);
    }
    
    return 0;
}