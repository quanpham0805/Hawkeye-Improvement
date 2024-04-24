/*  Hawkeye with Belady's Algorithm Replacement Policy
    Code for Hawkeye configurations of 1 and 2 in Champsim */

#include <cmath>
#include <map>

#include "../inc/champsim_crc2.h"
#include "bit_manimulation.h"
#include "hawkeye_predictor.h"
#include "helper_function.h"
#include "hht.h"
#include "optgen.h"
#include "perceptron.h"

#define NUM_CORE 1
#define LLC_SETS NUM_CORE * 2048
#define LLC_WAYS 16

// check if the set is sampled
// check bits[0..5] == bits[5..11] in this case
// this is true because all 2048 sets can be represented by 11 bits
// so taking lower 6 bits, they can be used to represent 64 sets
// it's only counted exactly once because only 1 pair like above satisfies
// the condition
// Helper function to sample 64 sets for each core
#define SAMPLED_SET(set) \
  (bits(set, 0, 6) == bits(set, ((unsigned long long)log2(LLC_SETS) - 6), 6))

// 3-bit RRIP counter
#define MAXRRIP 7
uint32_t rrip[LLC_SETS][LLC_WAYS];

// Hawkeye predictors for demand and prefetch requests
// 2K entries, 5-bit counter per each entry
Hawkeye_Predictor* predictor_demand;
Hawkeye_Predictor* predictor_prefetch;

PerceptronPredictor* perceptron_predictor_demand;
PerceptronPredictor* perceptron_predictor_prefetch;

// 64 vectors, 128 entries each
OPTgen optgen_occup_vector[LLC_SETS];

// Prefectching
bool prefetching[LLC_SETS][LLC_WAYS];

// Sampler components tracking cache history
#define SAMPLER_ENTRIES 2800
#define SAMPLER_HIST 8
#define SAMPLER_SETS SAMPLER_ENTRIES / SAMPLER_HIST
// 2800 entries, 4-bytes per each entry
vector<map<uint64_t, HISTORY>> cache_history_sampler;
// PC signature for each line
uint64_t sample_signature[LLC_SETS][LLC_WAYS];

// History time
#define TIMER_SIZE 1024
uint64_t set_timer[LLC_SETS];  // 64 sets, where 1 timer is used for every set

// hit history table
HHT* hht;
uint8_t hit_count[LLC_SETS][LLC_WAYS];

// Initialize replacement state
void InitReplacementState() {
  cout << "Initialize Hawkeye replacement policy state" << endl;

  for (int i = 0; i < LLC_SETS; i++) {
    for (int j = 0; j < LLC_WAYS; j++) {
      rrip[i][j] = MAXRRIP;
      sample_signature[i][j] = 0;
      hit_count[i][j] = 0;
      prefetching[i][j] = false;
    }
    set_timer[i] = 0;
    optgen_occup_vector[i].init(LLC_WAYS - 2);
  }

  cache_history_sampler.resize(SAMPLER_SETS);
  for (int i = 0; i < SAMPLER_SETS; i++) {
    cache_history_sampler[i].clear();
  }

  predictor_prefetch = new Hawkeye_Predictor();
  predictor_demand = new Hawkeye_Predictor();
  perceptron_predictor_demand = new PerceptronPredictor();
  perceptron_predictor_prefetch = new PerceptronPredictor();
  hht = new HHT();

  cout << "Finished initializing Hawkeye replacement policy state" << endl;
}

static inline uint64_t get_hht_tag(uint64_t paddr) {
  return (paddr >> ((uint64_t)log2(LLC_SETS) + 6));
}

// Find replacement victim
// Return value should be 0 ~ 15 or 16 (bypass)
uint32_t GetVictimInSet(uint32_t cpu, uint32_t set, const BLOCK* current_set,
                        uint64_t PC, uint64_t paddr, uint32_t type) {
  // Find the line with RRPV of 7 in that set
  for (uint32_t i = 0; i < LLC_WAYS; i++) {
    if (rrip[set][i] == MAXRRIP) {
      return i;
    }
  }

  for (uint32_t i = 0; i < LLC_WAYS; i++) {
    if (!current_set[i].valid) {
      return i;
    }
  }

  // If not found -> no cache-averse line
  // use EHC to break ties

  // victim1 uses only RRIP info
  // victim2 uses HHT if possible
  int32_t victim1 = -1, victim2 = -1;
  uint32_t max_rrpv = 0;

  // If no RRPV of 7, then we find next highest RRPV value (oldest
  // cache-friendly line)

  for (uint32_t i = 0; i < LLC_WAYS; i++) {
    if (rrip[set][i] >= max_rrpv) {
      victim1 = i;
      max_rrpv = rrip[set][i];
    }
  }

#ifdef HHT_ENABLE
  uint8_t min_val = MAX_HIT_COUNT + MAXRRIP + 2;
  for (uint32_t i = 0; i < LLC_WAYS; i++) {
    uint8_t expected_hit_count =
        hht->get_expected_hit_count(current_set[i].tag);
    // evict max rrip -> evict min max_rrip - current_rrip
    // combine with hit_count -> minimize both
    uint8_t value =
        abs(expected_hit_count - hit_count[set][i]) + (max_rrpv - rrip[set][i]);
    if (value < min_val) {
      min_val = value;
      victim2 = i;
    }
  }

  // edge case where incoming has lower value -> bypass
  uint8_t incoming_paddr_value =
      hht->get_expected_hit_count(get_hht_tag(paddr));
  if (incoming_paddr_value < min_val) {
    if (type != WRITEBACK) {
      return LLC_WAYS;
    }
  }
#endif

  int32_t victim;
  if (victim2 != -1) {
    victim = victim2;
  } else {
    victim = victim1;
  }

#ifdef HHT_ENABLE
  hht->add_to_hht(current_set[victim].tag, hit_count[set][victim]);
#endif

  // Asserting that LRU victim is not -1
  // Predictor will be trained negaively on evictions
  if (SAMPLED_SET(set)) {
    if (prefetching[set][victim]) {
      predictor_prefetch->decrease(sample_signature[set][victim]);
      perceptron_predictor_prefetch->update(sample_signature[set][victim],
                                            false, true);
    } else {
      predictor_demand->decrease(sample_signature[set][victim]);
      perceptron_predictor_demand->update(sample_signature[set][victim], false,
                                          true);
    }
  }

  return victim;
}

// Helper function for "UpdateReplacementState" to update cache history
void update_cache_history(unsigned int sample_set, unsigned int currentVal) {
  for (map<uint64_t, HISTORY>::iterator it =
           cache_history_sampler[sample_set].begin();
       it != cache_history_sampler[sample_set].end(); it++) {
    if ((it->second).lru < currentVal) {
      (it->second).lru++;
    }
  }
}

// Called on every cache hit and cache fill
void UpdateReplacementState(uint32_t cpu, uint32_t set, uint32_t way,
                            uint64_t paddr, uint64_t PC, uint64_t victim_addr,
                            uint32_t type, uint8_t hit) {
  paddr = (paddr >> 6) << 6;

  // Ignore all types that are writebacks
  if (type == WRITEBACK) {
    return;
  }

  if (type == PREFETCH) {
    if (!hit) {
      prefetching[set][way] = true;
    }
  } else {
    prefetching[set][way] = false;
  }

  // Only if we are using sampling sets for OPTgen
  if (SAMPLED_SET(set)) {
    uint64_t currentVal = set_timer[set] % OPTGEN_SIZE;
    uint64_t sample_tag = CRC(paddr >> 12) % 256;
    uint32_t sample_set = (paddr >> 6) % SAMPLER_SETS;

    // If line has been used before, ignoring prefetching (demand access
    // operation)
    if ((type != PREFETCH) &&
        (cache_history_sampler[sample_set].find(sample_tag) !=
         cache_history_sampler[sample_set].end())) {
      unsigned int current_time = set_timer[set];
      if (current_time <
          cache_history_sampler[sample_set][sample_tag].previousVal) {
        current_time += TIMER_SIZE;
      }
      uint64_t previousVal =
          cache_history_sampler[sample_set][sample_tag].previousVal %
          OPTGEN_SIZE;
      bool isWrap =
          (current_time -
           cache_history_sampler[sample_set][sample_tag].previousVal) >
          OPTGEN_SIZE;

      // Train predictor positvely for last PC value that was prefetched
      if (!isWrap &&
          optgen_occup_vector[set].is_cache(currentVal, previousVal)) {
        if (cache_history_sampler[sample_set][sample_tag].prefetching) {
          predictor_prefetch->increase(
              cache_history_sampler[sample_set][sample_tag].PCval);
          perceptron_predictor_prefetch->update(
              cache_history_sampler[sample_set][sample_tag].PCval, true, true);
        } else {
          predictor_demand->increase(
              cache_history_sampler[sample_set][sample_tag].PCval);
          perceptron_predictor_demand->update(
              cache_history_sampler[sample_set][sample_tag].PCval, true, true);
        }
      }
      // Train predictor negatively since OPT did not cache this line
      else {
        if (cache_history_sampler[sample_set][sample_tag].prefetching) {
          predictor_prefetch->decrease(
              cache_history_sampler[sample_set][sample_tag].PCval);
          perceptron_predictor_prefetch->update(
              cache_history_sampler[sample_set][sample_tag].PCval, false, true);
        } else {
          predictor_demand->decrease(
              cache_history_sampler[sample_set][sample_tag].PCval);
          perceptron_predictor_demand->update(
              cache_history_sampler[sample_set][sample_tag].PCval, false, true);
        }
      }

      optgen_occup_vector[set].set_access(currentVal);
      // Update cache history
      update_cache_history(sample_set,
                           cache_history_sampler[sample_set][sample_tag].lru);

      // Mark prefetching as false since demand access
      cache_history_sampler[sample_set][sample_tag].prefetching = false;
    }
    // If line has not been used before, mark as prefetch or demand
    else if (cache_history_sampler[sample_set].find(sample_tag) ==
             cache_history_sampler[sample_set].end()) {
      // If sampling, find victim from cache
      if (cache_history_sampler[sample_set].size() == SAMPLER_HIST) {
        // Replace the element in the cache history
        uint64_t addr_val = 0;
        for (map<uint64_t, HISTORY>::iterator it =
                 cache_history_sampler[sample_set].begin();
             it != cache_history_sampler[sample_set].end(); it++) {
          if ((it->second).lru == (SAMPLER_HIST - 1)) {
            addr_val = it->first;
            break;
          }
        }
        cache_history_sampler[sample_set].erase(addr_val);
      }

      // Create new entry
      cache_history_sampler[sample_set][sample_tag].init();
      // If preftech, mark it as a prefetching or if not, just set the demand
      // access
      if (type == PREFETCH) {
        cache_history_sampler[sample_set][sample_tag].set_prefetch();
        optgen_occup_vector[set].set_prefetch(currentVal);
      } else {
        optgen_occup_vector[set].set_access(currentVal);
      }

      // Update cache history
      update_cache_history(sample_set, SAMPLER_HIST - 1);
    }
    // If line is neither of the two above options, then it is a prefetch line
    else {
      uint64_t previousVal =
          cache_history_sampler[sample_set][sample_tag].previousVal %
          OPTGEN_SIZE;
      if (set_timer[set] -
              cache_history_sampler[sample_set][sample_tag].previousVal <
          5 * NUM_CORE) {
        if (optgen_occup_vector[set].is_cache(currentVal, previousVal)) {
          if (cache_history_sampler[sample_set][sample_tag].prefetching) {
            predictor_prefetch->increase(
                cache_history_sampler[sample_set][sample_tag].PCval);
            perceptron_predictor_prefetch->update(
                cache_history_sampler[sample_set][sample_tag].PCval, true,
                true);
          } else {
            predictor_demand->increase(
                cache_history_sampler[sample_set][sample_tag].PCval);
            perceptron_predictor_demand->update(
                cache_history_sampler[sample_set][sample_tag].PCval, true,
                true);
          }
        }
      }
      cache_history_sampler[sample_set][sample_tag].set_prefetch();
      optgen_occup_vector[set].set_prefetch(currentVal);
      // Update cache history
      update_cache_history(sample_set,
                           cache_history_sampler[sample_set][sample_tag].lru);
    }
    // Update the sample with time and PC
    cache_history_sampler[sample_set][sample_tag].update(set_timer[set], PC);
    cache_history_sampler[sample_set][sample_tag].lru = 0;
    set_timer[set] = (set_timer[set] + 1) % TIMER_SIZE;
  }

  // set up HHT entry

  if (hit) {
    if (hit_count[set][way] < MAX_HIT_COUNT - 1) {
      hit_count[set][way]++;
    } else if (hit_count[set][way] == MAX_HIT_COUNT - 1) {
      hit_count[set][way] = MAX_HIT_COUNT;
      hht->add_to_hht(get_hht_tag(paddr), hit_count[set][way]);
    }
  } else {
    hit_count[set][way] = 0;
  }

// Retrieve Hawkeye's prediction for line
#ifdef PERCEPTRON
  bool prediction = perceptron_predictor_demand->get_prediction(PC);
  if (type == PREFETCH) {
    prediction = perceptron_predictor_prefetch->get_prediction(PC);
  }
#else
  bool prediction = predictor_demand->get_prediction(PC);
  if (type == PREFETCH) {
    prediction = predictor_prefetch->get_prediction(PC);
  }
#endif

  sample_signature[set][way] = PC;
  // Fix RRIP counters with correct RRPVs and age accordingly
  if (!prediction) {
    rrip[set][way] = MAXRRIP;
  } else {
    rrip[set][way] = 0;
    if (!hit) {
      // Verifying RRPV of lines has not saturated
      bool isMaxVal = false;
      for (uint32_t i = 0; i < LLC_WAYS; i++) {
        if (rrip[set][i] == MAXRRIP - 1) {
          isMaxVal = true;
        }
      }

      // Aging cache-friendly lines that have not saturated
      for (uint32_t i = 0; i < LLC_WAYS; i++) {
        if (!isMaxVal && rrip[set][i] < MAXRRIP - 1) {
          rrip[set][i]++;
        }
      }
    }
    rrip[set][way] = 0;
  }
}

// Use this function to print out your own stats on every heartbeat
void PrintStats_Heartbeat() {
  int hits = 0;
  int access = 0;
  for (int i = 0; i < LLC_SETS; i++) {
    hits += optgen_occup_vector[i].get_optgen_hits();
    access += optgen_occup_vector[i].access;
  }

  cout << "OPTGen Hits: " << hits << endl;
  cout << "OPTGen Access: " << access << endl;
  cout << "OPTGEN Hit Rate: " << 100 * ((double)hits / (double)access) << endl;
}

// Use this function to print out your own stats at the end of simulation
void PrintStats() {
  int hits = 0;
  int access = 0;
  for (int i = 0; i < LLC_SETS; i++) {
    hits += optgen_occup_vector[i].get_optgen_hits();
    access += optgen_occup_vector[i].access;
  }

  cout << "Final OPTGen Hits: " << hits << endl;
  cout << "Final OPTGen Access: " << access << endl;
  cout << "Final OPTGEN Hit Rate: " << 100 * ((double)hits / (double)access)
       << endl;
  cout << "Final cache history size: " << cache_history_sampler.size() << endl;
}