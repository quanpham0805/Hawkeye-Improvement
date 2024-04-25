#ifndef _PERCEPTRON_H_
#define _PERCEPTRON_H_

#include <bitset>
#include <cstdint>

#define PERCEPTRON_TABLE_SIZE 2048  // Determined by threshold and history length
#define THRESHOLD 44    // Floor(1.93 * HIST_LEN + 14) as specified in paper
#define HIST_LEN 16      // Optimal value as determined by paper

using namespace std;

class PerceptronPredictor {
  int32_t perceptron_table[PERCEPTRON_TABLE_SIZE][HIST_LEN + 1];
  bitset<HIST_LEN> lhr[PERCEPTRON_TABLE_SIZE];

  uint64_t hash_pc(uint64_t pc) {
    return CRC(pc) % PERCEPTRON_TABLE_SIZE;
  }

  int get_weighted_sum(uint64_t hash) {
    int sum = perceptron_table[hash][0];
    for (size_t i = 1; i <= HIST_LEN; i++) {
      if (lhr[hash][i - 1]) {
        sum += perceptron_table[hash][i];
      } else {
        sum -= perceptron_table[hash][i];
      }
    }
    return sum;
  }

 public:
  PerceptronPredictor() {
    for (size_t i = 0; i < PERCEPTRON_TABLE_SIZE; i++) {
      lhr[i] = bitset<HIST_LEN>(0);
      for (size_t j = 0; j < HIST_LEN + 1; j++) {
        perceptron_table[i][j] = 0;
      }
    }
  }

  bool get_prediction(uint64_t pc) {
    uint64_t hash = hash_pc(pc);
    return get_weighted_sum(hash) >= 0;
  }

  void update(uint64_t PC, bool result, bool prediction) {
    uint64_t hash = hash_pc(PC);

    // Update the perceptron table entry only if the threshold has not been
    // reached or the predicted and true outcomes disagree. Update the bias
    // first, then the weights. Correct for overflow.
    if (result != prediction || abs(get) <= THRESHOLD) {
      // If the branch was taken, increment the bias value. Else, decrement it.

      if (result) {
        if (perceptron_table[hash][0] <= THRESHOLD) {
          perceptron_table[hash][0]++; 
        }
      } else {
        if (perceptron_table[hash][0] >= -THRESHOLD) {
          perceptron_table[hash][0]--;
        }
      }

      // Update the weights.

      for (int i = 1; i < HIST_LEN + 1; i++) {
        // increase weight if prediction matches history

        if (result == lhr[hash][i - 1]) {
          if (perceptron_table[hash][i] < THRESHOLD) {
            perceptron_table[hash][i]++;
          }
        } else {
          if (perceptron_table[hash][i] > -THRESHOLD) {
            perceptron_table[hash][i]--;
          }
        }
      }
    }
    // update the LHR by shifting left and setting new bit.
    lhr[hash] = (lhr[hash] << 1);
    lhr[hash].set(0, result);
  }
};

#endif  // _PERCEPTRON_H_
