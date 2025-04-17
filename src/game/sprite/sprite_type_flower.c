/* sprite_type_flower.c
 * arg=flowerid (native byte order)
 */

#include "game/zs.h"

struct sprite_flower {
  struct sprite hdr;
  uint8_t tileid; // Outline. Fill is +0x10.
  uint32_t tint;
};

#define SPRITE ((struct sprite_flower*)sprite)

/* Init.
 */
 
static int _flower_init(struct sprite *sprite) {
  sprite->layer=50;
  sprite->terminal_velocity=0.0;
  int p=session_flowerp_by_flowerid(sprite->arg);
  if (p<0) return -1;
  const struct session_flower *flower=g.session.flowerv+p;
  SPRITE->tileid=flower->tileid;
  SPRITE->tint=flower->tint;
  return 0;
}

/* Render.
 */
 
static void _flower_render(struct sprite *sprite,int x,int y) {
  if ((x<-NS_sys_tilesize)||(y<-NS_sys_tilesize)||(x>FBW+NS_sys_tilesize)||(y>FBH+NS_sys_tilesize)) return;
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,SPRITE->tileid,sprite->xform);
  graf_set_tint(&g.graf,SPRITE->tint);
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,SPRITE->tileid+0x10,sprite->xform);
  graf_set_tint(&g.graf,0);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_flower={
  .name="flower",
  .objlen=sizeof(struct sprite_flower),
  .init=_flower_init,
  .render=_flower_render,
};
