#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "game/zs.h"

#define HERO_WALK_SPEED 6.0 /* m/s */

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
  int input; // Full input state.
  int indx,indy,injump,initem; // Digested input state.
  int walkdx; // -1,0,1
};

#define SPRITE ((struct sprite_hero*)sprite)

void hero_fall_begin(struct sprite *sprite);
void hero_fall_end(struct sprite *sprite);

#endif
