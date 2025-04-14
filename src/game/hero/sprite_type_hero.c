#include "hero_internal.h"

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  sprite->physics_mask=(1<<NS_physics_solid);
  return 0;
}

/* Update animation.
 */
 
static void hero_animate(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)>0.0) return;
  SPRITE->animclock+=0.100;
  
  // Walking: Cycle 0..7
  if (SPRITE->walkdx) {
    if (++(SPRITE->animframe)>=8) SPRITE->animframe=0;
    
  // Not walking: Stop at 0 or 4.
  } else {
    if (!(SPRITE->animframe&3)) return;
    if (++(SPRITE->animframe)>=8) SPRITE->animframe=0;
  }
}

/* Update walking.
 */
 
static void hero_update_walk(struct sprite *sprite,double elapsed) {
  sprite->x+=HERO_WALK_SPEED*elapsed*SPRITE->walkdx;
}

/* Update jumping.
 * Returns >0 if a jump is in progress, ie caller should suspend gravity.
 */
 
static int hero_update_jump(struct sprite *sprite,double elapsed) {
  if (!SPRITE->injump) return 0;
  sprite->y-=8.0*elapsed;//TODO
  return 1;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  hero_animate(sprite,elapsed);
  if (SPRITE->walkdx) hero_update_walk(sprite,elapsed);
  hero_update_jump(sprite,elapsed);
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
  .fall_begin=hero_fall_begin,
  .fall_end=hero_fall_end,
};
