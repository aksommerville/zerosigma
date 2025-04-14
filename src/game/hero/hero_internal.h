#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "game/zs.h"

#define HERO_WALK_SPEED 8.0 /* m/s */
#define HERO_JUMP_POWER_DEFAULT 23.0 /* m/s */
#define HERO_JUMP_POWER_DECELERATION 48.0 /* m/s**2 */
#define HERO_COYOTE_TIME 0.040 /* s */
#define HERO_WALLJUMPX_INITIAL 18.0 /* m/s */
#define HERO_WALLJUMPY_INITIAL 24.0 /* m/s */
#define HERO_WALLJUMPX_DECELERATION 100.0 /* m/s**2 */
#define HERO_WALLJUMPY_DECELERATION 100.0 /* m/s**2 */
#define HERO_WALLJUMP_SUSPENDX 0.500 /* s */

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
  int input; // Full input state.
  int indx,indy,injump,initem; // Digested input state.
  int walkdx; // -1,0,1
  double jump_power;
  int walljump; // Nonzero during operation
  double walljump_xpower;
  double walljump_ypower;
  double suspendx;
};

#define SPRITE ((struct sprite_hero*)sprite)

void hero_fall_begin(struct sprite *sprite);
void hero_fall_end(struct sprite *sprite,double duration);

#endif
