#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "game/zs.h"

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_hero*)sprite)

#endif
