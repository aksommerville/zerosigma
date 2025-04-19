/* sprite_type_treadle.c
 * Sets a flag when Dot stands on it, and clears when she steps away.
 * Also triggerable by goats.
 * arg: u8 flagid, u24 reserved
 */
 
#include "game/zs.h"

struct sprite_treadle {
  struct sprite hdr;
  int flagid;
  int state;
  uint8_t tileid0;
};

#define SPRITE ((struct sprite_treadle*)sprite)

static int _treadle_init(struct sprite *sprite) {
  sprite->terminal_velocity=0.0;
  sprite->layer=99;
  SPRITE->tileid0=sprite->tileid;
  // Don't read the flag. Our state always begins zero, and we can actually set it on startup.
  SPRITE->flagid=sprite->arg>>24;
  session_set_flag(SPRITE->flagid,0);
  return 0;
}

static void _treadle_update(struct sprite *sprite,double elapsed) {
  int nstate=0;
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *other=*p;
    if (
      (other->type!=&sprite_type_hero)&&
      (other->type!=&sprite_type_goat)
    ) continue;
    double dx=other->x-sprite->x;
    if ((dx<-0.75)||(dx>0.75)) continue;
    double dy=other->y-sprite->y;
    if ((dy<-0.100)||(dy>0.125)) continue;
    nstate=1;
    break;
  }
  if (SPRITE->state==nstate) return;
  session_set_flag(SPRITE->flagid,nstate);
  SPRITE->state=nstate;
  sprite->tileid=SPRITE->tileid0+nstate;
  if (nstate) egg_play_sound(RID_sound_treadle_on);
  else egg_play_sound(RID_sound_treadle_off);
}

const struct sprite_type sprite_type_treadle={
  .name="treadle",
  .objlen=sizeof(struct sprite_treadle),
  .init=_treadle_init,
  .update=_treadle_update,
};
