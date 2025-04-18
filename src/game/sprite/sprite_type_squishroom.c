/* sprite_type_squishroom.c
 * Occupies 2x2 cells naturally, or 2x1 if you fastfall onto it.
 * arg=flagid (native order)
 */
 
#include "game/zs.h"

struct sprite_squishroom {
  struct sprite hdr;
  int compressed;
  int flagid;
};

#define SPRITE ((struct sprite_squishroom*)sprite)

static int _squishroom_init(struct sprite *sprite) {
  sprite->terminal_velocity=0.0;
  // No need to listen for the flag. External parties won't change it.
  SPRITE->flagid=sprite->arg;
  if (session_get_flag(SPRITE->flagid)) {
    SPRITE->compressed=1;
  }
  // Our position is centered in the SW tile.
  sprite->phl=-0.5;
  sprite->pht=SPRITE->compressed?-0.5:-1.5;
  sprite->phr=1.5;
  sprite->phb=0.5;
  return 0;
}

static void _squishroom_render(struct sprite *sprite,int x,int y) {
  if (SPRITE->compressed) {
    int dstx=x-(NS_sys_tilesize>>1);
    int dsty=y-(NS_sys_tilesize>>1);
    int srcx=NS_sys_tilesize*5;
    int srcy=NS_sys_tilesize*5;
    graf_draw_decal(&g.graf,g.texid_sprites,dstx,dsty,srcx,srcy,NS_sys_tilesize<<1,NS_sys_tilesize,0);
  } else {
    int dstx=x-(NS_sys_tilesize>>1);
    int dsty=y-NS_sys_tilesize-(NS_sys_tilesize>>1);
    int srcx=NS_sys_tilesize*3;
    int srcy=NS_sys_tilesize*4;
    graf_draw_decal(&g.graf,g.texid_sprites,dstx,dsty,srcx,srcy,NS_sys_tilesize<<1,NS_sys_tilesize<<1,0);
  }
}

const struct sprite_type sprite_type_squishroom={
  .name="squishroom",
  .objlen=sizeof(struct sprite_squishroom),
  .init=_squishroom_init,
  .render=_squishroom_render,
};

void squishroom_compress(struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_squishroom)) return;
  if (SPRITE->compressed) return;
  egg_play_sound(RID_sound_squishroom);
  SPRITE->compressed=1;
  sprite->pht=-0.5;
  session_set_flag(SPRITE->flagid,1);
}
