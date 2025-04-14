#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "game/zs.h"

#define HERO_WALK_SPEED 8.0 /* m/s */
#define HERO_JUMP_POWER_DEFAULT 20.0 /* m/s */
#define HERO_JUMP_POWER_DECELERATION 38.0 /* m/s**2 */
#define HERO_COYOTE_TIME 0.040 /* s */

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
  int input; // Full input state.
  int indx,indy,injump,initem; // Digested input state.
  int walkdx; // -1,0,1
  double jump_power;
};

#define SPRITE ((struct sprite_hero*)sprite)

void hero_fall_begin(struct sprite *sprite);
void hero_fall_end(struct sprite *sprite,double duration);

#endif
