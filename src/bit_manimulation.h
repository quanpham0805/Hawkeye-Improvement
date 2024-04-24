#ifndef BIT_MANIPULATION
#define BIT_MANIPULATION



// Mathmatical functions needed for sampling set
// set l lower bits to 1
#define bitmask(l) \
  (((l) == 64) ? (unsigned long long)(-1LL) : ((1LL << (l)) - 1LL))

// extract l bits starting from i-th bit
#define bits(x, i, l) (((x) >> (i)) & bitmask(l))

#endif