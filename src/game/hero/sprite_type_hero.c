#include "hero_internal.h"

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  return 0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.100;
    if (++(SPRITE->animframe)>=8) SPRITE->animframe=0;
  }
}

/* Render.
 * (x,y) are the middle of my body, the lower tile.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y) {
  uint8_t col=0;
  switch (SPRITE->animframe) {
    case 0: break;
    case 1: col=1; break;
    case 2: col=2; break;
    case 3: col=1; break;
    case 4: col=0; break;
    case 5: col=3; break;
    case 6: col=4; break;
    case 7: col=3; break;
  }
  graf_draw_tile(&g.graf,g.texid_sprites,x,y-NS_sys_tilesize,0x00+col,sprite->xform);
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,0x10+col,sprite->xform);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};
