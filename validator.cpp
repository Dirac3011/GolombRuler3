#include <iostream>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

/**
 * Validates if a set of numbers is a valid B_g set.
 * @param gamma The maximum allowed occurrences of any difference.
 * @param s The set of integers to check.
 */
void validate_gamma_set(int gamma, const vector<int>& s) {
    map<int, int> diff_counts;
    int n = s.size();
    bool is_valid = true;

    cout << "Checking set for n=" << n << ", gamma=" << gamma << "..." << endl;

    // Calculate all pairwise differences (a_j - a_i where j > i)
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            int diff = abs(s[j] - s[i]);
            diff_counts[diff]++;
        }
    }

    // Check for violations
    for (auto const& [diff, count] : diff_counts) {
        if (count > gamma) {
            cout << "  [!] VIOLATION: Difference " << diff << " appears " << count << " times!" << endl;
            is_valid = false;
        }
    }

    if (is_valid) {
        cout << "  [+] SUCCESS: Set is valid. No difference appears more than " << gamma << " times." << endl;
        cout << "  Max element (alpha): " << s.back() << endl;
    } else {
        cout << "  [-] FAILED: Set exceeds the gamma limit." << endl;
    }
}

int main() {
    // Your n=15 case
    vector<int> my_set = {0, 4, 5, 6, 8, 18, 21, 28, 30, 38, 45, 49, 56, 61, 67, 70, 75, 76};
    int gamma_limit = 3;

    validate_gamma_set(gamma_limit, my_set);

    return 0;
}