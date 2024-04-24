#ifndef _HHT_H_
#define _HHT_H_

#include <cstdint>
#include <deque>
#include <map>
#include <queue>

// The HHT is updated either when Current Hit Counter saturates or when a block
// gets evicted from the cache. Whenever one of these events occurs, we look up
// the HHT with the tag of the cache block. If the lookup results in a hit, we
// push the Current Hit Counter to the front of Hit Count Queue. Otherwise, we
// allocate a new entry in the HHT set its Valid bit, update its Tag, and
// finally clear the Hit Count Queue and push the Current Hit Counter to the
// front of Hit Count Queue.

#define MAX_HIT_COUNT 7
#define MAX_QUEUE_SIZE 4
#define TABLE_SIZE 128
#define TABLE_WAY 16
using namespace std;

class HHT {
 private:
  struct HHTEntry {
    uint64_t tag;
    queue<uint8_t> hit_count;
  };

  int hash_tag_index(uint64_t tag) {
    uint64_t hashed_tag = tag % TABLE_SIZE;
    return hashed_tag;
  }
  uint64_t hash_tag_tag(uint64_t tag) {
    uint64_t hashed_tag = tag / TABLE_SIZE;
    hashed_tag &= 0xfffff;
    return hashed_tag;
  }
  // map tag index with tag tag
  deque<HHTEntry*> hht[TABLE_SIZE + 1];

  uint8_t queue_avg(const queue<uint8_t>& hhtqueue) {
    uint8_t sum = 0;
    queue<uint8_t> temp = hhtqueue;
    while (!temp.empty()) {
      sum += temp.front();
      temp.pop();
    }
    // average among MAX_QUEUE_SIZE
    return sum / MAX_QUEUE_SIZE;
  }

  void add_to_queue(queue<uint8_t>& hhtqueue, uint8_t hit_count) {
    hhtqueue.push(hit_count);
    while (hhtqueue.size() > MAX_QUEUE_SIZE) hhtqueue.pop();
  }

 public:
  void add_to_hht(uint64_t tag, uint8_t hit_count) {
#ifdef HHT_ENABLE
    uint64_t tag_index = hash_tag_index(tag);
    uint64_t tag_tag = hash_tag_tag(tag);
    int index = -1;

    for (int i = 0; i < (int)hht[tag_index].size(); ++i) {
      if (hht[tag_index][i]->tag == tag_tag) {
        auto* entry = hht[tag_index][i];
        hht[tag_index].erase(hht[tag_index].begin() + i);
        hht[tag_index].push_back(entry);
        index = hht[tag_index].size() - 1;
        break;
      }
    }

    if (index != -1) {
      add_to_queue(hht[tag_index][index]->hit_count, hit_count);
    } else {
      HHTEntry* entry = new HHTEntry();
      entry->tag = tag_tag;
      add_to_queue(entry->hit_count, hit_count);
      hht[tag_index].push_back(entry);
      while (hht[tag_index].size() > TABLE_WAY) {
        delete hht[tag_index].front();
        hht[tag_index].pop_front();
      }
    }
#endif
  }

  uint8_t get_expected_hit_count(uint64_t tag) {
#ifdef HHT_ENABLE
    uint64_t tag_index = hash_tag_index(tag);
    uint64_t tag_tag = hash_tag_tag(tag);
    int index = -1;
    for (int i = 0; i < (int)hht[tag_index].size(); ++i) {
      if (hht[tag_index][i]->tag == tag_tag) {
        index = i;
        break;
      }
    }

    if (index != -1) {
      return queue_avg(hht[tag_index][index]->hit_count);
    } else {
      return 1;
    }
#else
    return 1;
#endif
  }
};

#endif  // _HHT_H_
