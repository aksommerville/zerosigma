/* sprite_type_teleghost.c
 * A silhouette of Dot that fades out after she teleports away.
 * arg=xform (low bits)
 */
 
#include "game/zs.h"

#define TELEGHOST_TTL 0.500
#define TELEGHOST_SPEED 4.0
#define TELEGHOST_ALPHA 128.0

struct sprite_teleghost {
  struct sprite hdr;
  double ttl;
};

#define SPRITE ((struct sprite_teleghost*)sprite)

static int _teleghost_init(struct sprite *sprite) {
  sprite->terminal_velocity=0.0;
  sprite->xform=sprite->arg;
  SPRITE->ttl=TELEGHOST_TTL;
  return 0;
}

static void _teleghost_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    sprite->defunct=1;
  }
  if (sprite->xform) {
    sprite->x-=TELEGHOST_SPEED*elapsed;
  } else {
    sprite->x+=TELEGHOST_SPEED*elapsed;
  }
}

static void _teleghost_render(struct sprite *sprite,int x,int y) {
  int alpha=(SPRITE->ttl*TELEGHOST_ALPHA)/TELEGHOST_TTL;
  if (alpha<=0) return;
  if (alpha>0xff) alpha=0xff;
  graf_set_tint(&g.graf,0xffffffff);
  graf_set_alpha(&g.graf,alpha);
  graf_draw_tile(&g.graf,g.texid_sprites,x,y-NS_sys_tilesize,0x00,sprite->xform);
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,0x10,sprite->xform);
  graf_set_tint(&g.graf,0);
  graf_set_alpha(&g.graf,0xff);
}

const struct sprite_type sprite_type_teleghost={
  .name="teleghost",
  .objlen=sizeof(struct sprite_teleghost),
  .init=_teleghost_init,
  .update=_teleghost_update,
  .render=_teleghost_render,
};
