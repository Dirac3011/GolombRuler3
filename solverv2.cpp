#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>

using namespace std;

// Using __uint128_t allows differences up to 127.
// G-(3, 16) is well within this range.
typedef __uint128_t bitmask;

// Optimized Pruning Table for gamma=3
// These are the known/calculated lower bounds for G-(3, k)
const int g_min_span[] = {
    0,  // k=0
    0,  // k=1 (A={0})
    1,  // k=2 (A={0,1})
    2,  // k=3
    3,  // k=4
    5,  // k=5
    7,  // k=6
    9,  // k=7
    12,  // k=8
    15, // k=9
    19, // k=10
    24, // k=11
    29, // k=12
    35, // k=13
    41, // k=14
    49, // k=15
    58  // k=16
};

bool backtrack(int* A, int size, bitmask m1, bitmask m2, bitmask m3, int k_target, int alpha) {
    int last_val = A[size - 1];
    int remaining = k_target - size;

    // Pruning: Can we fit the remaining elements within the alpha bound?
    if (last_val + g_min_span[remaining + 1] > alpha) {
        return false;
    }

    // Base case: Try to close the set with exactly 'alpha'
    if (size == k_target - 1) {
        for (int i = 0; i < size; ++i) {
            int diff = alpha - A[i];
            // Conflict if the difference is already seen 3 times
            if ((m3 >> diff) & 1) return false;
        }
        A[size] = alpha;
        return true;
    }

    int max_cand = alpha - g_min_span[remaining];
    
    // Symmetry breaking: A[1] should be in the lower half of the total span
    // to avoid searching reflected sets.
    if (size == 1) {
        max_cand = min(max_cand, alpha / 2);
    }

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
            // Update bitmasks vertically:
            // Thrice = old_thrice OR (old_twice AND new_diffs)
            // Twice  = old_twice  OR (old_once  AND new_diffs)
            // Once   = old_once   OR new_diffs
            bitmask next_m3 = m3 | (m2 & new_diffs);
            bitmask next_m2 = m2 | (m1 & new_diffs);
            bitmask next_m1 = m1 | new_diffs;

            A[size] = cand;
            if (backtrack(A, size + 1, next_m1, next_m2, next_m3, k_target, alpha)) {
                return true;
            }
        }
    }
    return false;
}

void solve(int k) {
    // Start alpha at the theoretical lower bound
    int alpha = g_min_span[k];
    int A[32]; 

    cout << "Computing G-(3, " << k << ") starting at alpha=" << alpha << "..." << endl;

    while (true) {
        auto start = chrono::high_resolution_clock::now();
        
        A[0] = 0; // Standardize A[1]=0 (0-indexed A[0])
        bitmask m1 = 0, m2 = 0, m3 = 0;

        if (backtrack(A, 1, m1, m2, m3, k, alpha)) {
            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double> elapsed = end - start;
            
            cout << "FOUND! G-(3, " << k << ") = " << alpha;
            cout << " | Set: [";
            for (int i = 0; i < k; ++i) cout << A[i] << (i == k - 1 ? "" : ",");
            cout << "] (" << elapsed.count() << "s)" << endl;
            break;
        } else {
            alpha++;
        }
    }
}

int main() {
    solve(16);
    return 0;
}