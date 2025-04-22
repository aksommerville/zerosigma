/* hiscore.h
 * Calculates, persists, and loads the set of high scores.
 * We're a global singleton (g.hiscore).
 * Score encode as whitespace-delimited unsigned decimal integers, corresponding to the HISCORE_* symbols below.
 */
 
#ifndef HISCORE_H
#define HISCORE_H

#define HISCORE_TIME         0 /* ms. Best time to finish regardless of score. */
#define HISCORE_VALID_TIME   1 /* ms. Best time with 5 valid bouquets. Zero if never achieved. */
#define HISCORE_PERFECT_TIME 2 /* ms. Best time with a 500 score. Zero if never achieved. */
#define HISCORE_SCORE        3 /* 0..500. Best score. */
#define HISCORE_HERMIT_SCORE 4 /* 0..57. Best score without leaving home. 57 is usually but not always possible. */
#define HISCORE_MISER_SCORE  5 /* 0..236. Best score without using teleport, fastfall, or walljump. 126 if you don't get any perfect; I think 236 is usually impossible. */
#define HISCORE_COUNT        6
/* Note that it is not possible to win a perfect score with HERMIT or MISER.
 * HERMIT has an upper limit of 57, the count of flowers in map:1.
 * Sometimes it's possible to deliver a perfect bouquet on the first day without leaving home, but you won't have enough flowers to finish the session then.
 * Is it actually possible to use all 57 flowers?
 *   eg 9 9 12 15 12
 * ...first try: yes. It's beyond my skill to prove or disprove that it's always possible.
 * I did it by counting the flowers and working out a plan for the whole session in advance.
 * There's probably an incremental strategy for it too.
 *  - Day 1, level it: Pick a bouquet such that every color ends up <=9 and >=5, and at least two colors end up equal. (can i prove there is such a bouquet? ...no there's provably not; what if one color is empty?)
 *  - Day 2, deplete the two equal colors.
 *  - Days 3,4,5: Single-color bouquets.
 * Hermit 57 is not necessarily possible: Consider the edge case where every home flower is the same color. You can't play more than 45 of them.
 * ...oooh consider it again, jackass: There's only 45 of each color in the world.
 * ...well but still: If there's 45 of one color, then assume an ideal distribution, 3 each of the remaining colors, clear the others on the first day and you're left with 42 flowers and 4 days to play them -- impossible.
 * ...an even idealler case for that would be 45 of one and 12 of one other. Play 12+12 the first day, then you have 4 days to clear 33 flowers -- possible.
 * So the point remains: Impossible Hermit-57 distributions are possible but unlikely.
 */
/* And what is the upper bound for MISER?
 *  1 57
 *  2 19
 *  3  0
 *  4  0 (can get in but can't get out)
 *  5 19
 *  6 12
 *  7 19
 * That's 126 flowers. Must allow for the possibility of 2 perfect bouquets, so +110 = 236 is the hard upper bound.
 */

struct hiscore {
  int v[HISCORE_COUNT];
  uint32_t validscores; // (1<<HISCORE_*), which ones are not defaults.
  uint32_t newscores; // (1<<HISCORE_*), which ones were set at the last commit (including nonzero ties).
};

/* Wipes any existing state and reloads from Egg's store, or sets defaults.
 */
void hiscore_load();

/* Reads from (g.session), recomposes high scores as needed, and writes to Egg's store if needed.
 * zs_layer_dayend should call this as it wraps up the last day.
 * Rewrites both (g.hiscore) and (g.prevscore).
 */
void hiscore_commit();

/* Timed scores can't be perfect (they safely return zero every time here).
 * The ones measured in points all have a hard upper limit.
 * For SCORE, it's eminently reachable.
 * HERMIT and MISER are both difficult, you have to use every available flower and the draw doesn't guarantee that that's even possible.
 * For MISER, the real limit is 236 but we call it perfect at 126 -- you don't have to play two perfect bouquets to get the prize.
 */
int hiscore_is_perfect(int id,int v);

#endif
