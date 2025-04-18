/* sprite_type_snore.c
 * Do a little animation of a "Z", float upward, self-terminate.
 */
 
#include "game/zs.h"

struct sprite_snore {
  struct sprite hdr;
  double animclock;
  int frame;
};

#define SPRITE ((struct sprite_snore*)sprite)

static int _snore_init(struct sprite *sprite) {
  sprite->terminal_velocity=0.0;
  sprite->tileid=0x4b;
  return 0;
}

static void _snore_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.666;
    switch (SPRITE->frame++) {
      case 0: break; // Do nothing on zero; the clock starts at zero so that fires right away.
      case 1: sprite->tileid++; break;
      case 2: sprite->tileid++; break;
      case 3: sprite->tileid--; break;
      case 4: sprite->tileid--; break;
      default: sprite->defunct=1;
    }
  }
  sprite->y-=0.333*elapsed;
}

const struct sprite_type sprite_type_snore={
  .name="snore",
  .objlen=sizeof(struct sprite_snore),
  .init=_snore_init,
  .update=_snore_update,
};

void snore_drop_all() {
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    if (sprite->type!=&sprite_type_snore) continue;
    sprite->defunct=1;
  }
}
