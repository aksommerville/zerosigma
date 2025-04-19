/* sprite_type_flamethrower.c
 * arg: u8 flagid, s4 rangex, s4 rangey, u4 invert, u12 lag
 */
 
#include "game/zs.h"

struct sprite_flamethrower {
  struct sprite hdr;
  int flagid;
  int rangex,rangey;
  int invert;
  int lag;
  uint8_t tileid0;
  int state;
  int listener;
  double animclock;
  int animframe;
  double toggleclock;
};

#define SPRITE ((struct sprite_flamethrower*)sprite)

static void _flamethrower_del(struct sprite *sprite) {
  session_unlisten_flag(SPRITE->listener);
}

static void flamethrower_on_flag(int flagid,int v,void *userdata) {
  struct sprite *sprite=userdata;
  double lag=SPRITE->lag/1000.0;
  if (v) lag=0.0; // Lag only happens when switches go off. On is instantaneous.
  SPRITE->toggleclock=0.0;
  if (SPRITE->invert) v=v?0:1;
  if (v) {
    if (SPRITE->state) return;
  } else {
    if (!SPRITE->state) return;
  }
  if (lag<=0.0) SPRITE->toggleclock=0.001;
  else SPRITE->toggleclock=lag;
}

static int _flamethrower_init(struct sprite *sprite) {
  sprite->terminal_velocity=0;
  sprite->layer=101;
  SPRITE->tileid0=sprite->tileid;
  SPRITE->flagid=sprite->arg>>24;
  SPRITE->rangex=(sprite->arg>>20)&15; if (SPRITE->rangex&8) SPRITE->rangex|=~15;
  SPRITE->rangey=(sprite->arg>>16)&15; if (SPRITE->rangey&8) SPRITE->rangey|=~15;
  SPRITE->invert=(sprite->arg>>12)&15;
  SPRITE->lag=sprite->arg&0xfff;
  
  if (SPRITE->rangex<0) {
    if (SPRITE->rangey) return -1;
    sprite->xform=EGG_XFORM_XREV;
  
  } else if (SPRITE->rangex>0) {
    if (SPRITE->rangey) return -1;
  
  } else if (SPRITE->rangey<0) {
    if (SPRITE->rangex) return -1;
    sprite->xform=EGG_XFORM_XREV|EGG_XFORM_SWAP;
    
  } else if (SPRITE->rangey>0) {
    if (SPRITE->rangex) return -1;
    sprite->xform=EGG_XFORM_SWAP;
    
  } else return -1;
  
  if (SPRITE->invert) {
    if (!session_get_flag(SPRITE->flagid)) {
      SPRITE->state=1;
      sprite->tileid=SPRITE->tileid0+1;
    }
  } else {
    if (session_get_flag(SPRITE->flagid)) {
      SPRITE->state=1;
      sprite->tileid=SPRITE->tileid0+1;
    }
  }
  SPRITE->listener=session_listen_flag(SPRITE->flagid,flamethrower_on_flag,sprite);
  
  return 0;
}

static void flamethrower_check_burnination(struct sprite *sprite) {

  double l=sprite->x+sprite->phl;
  double r=sprite->x+sprite->phr;
  double t=sprite->y+sprite->pht;
  double b=sprite->y+sprite->phb;
  if (SPRITE->rangex<0) {
    l=sprite->x+SPRITE->rangex;
    r=sprite->x;
  } else if (SPRITE->rangex>0) {
    l=sprite->x;
    r=sprite->x+SPRITE->rangex;
  } else if (SPRITE->rangey<0) {
    t=sprite->y+SPRITE->rangey;
    b=sprite->y;
  } else if (SPRITE->rangey>0) {
    t=sprite->y;
    b=sprite->y+SPRITE->rangey;
  }

  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *victim=*p;
    if (!victim->fragile) continue;
    double vl=victim->x+victim->phl;
    double vr=victim->x+victim->phr;
    double vt=victim->y+victim->pht;
    double vb=victim->y+victim->phb;
    if (vl>=r) continue;
    if (vr<=l) continue;
    if (vt>=b) continue;
    if (vb<=t) continue;
    if (victim->type->hurt) victim->type->hurt(victim,sprite);
    else victim->defunct=1;
  }
}

static void _flamethrower_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->toggleclock>0.0) {
    if ((SPRITE->toggleclock-=elapsed)<=0.0) {
      if (SPRITE->state) {
        SPRITE->state=0;
        sprite->tileid=SPRITE->tileid0;
      } else {
        SPRITE->state=1;
        sprite->tileid=SPRITE->tileid0+1;
      }
    }
  }
  if (SPRITE->state) {
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=0.150;
      if (++(SPRITE->animframe)>=2) SPRITE->animframe=0;
    }
    flamethrower_check_burnination(sprite);
  }
}

static void _flamethrower_render(struct sprite *sprite,int x,int y) {
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,sprite->tileid,sprite->xform);
  if (SPRITE->state) {
    int dx=0,dy=0,i=0;
         if (SPRITE->rangex<0) { dx=-NS_sys_tilesize; i=-SPRITE->rangex; }
    else if (SPRITE->rangex>0) { dx= NS_sys_tilesize; i= SPRITE->rangex; }
    else if (SPRITE->rangey<0) { dy=-NS_sys_tilesize; i=-SPRITE->rangey; }
    else if (SPRITE->rangey>0) { dy= NS_sys_tilesize; i= SPRITE->rangey; }
    x+=dx;
    y+=dy;
    for (;i-->0;x+=dx,y+=dy) {
      uint8_t tileid=SPRITE->tileid0+SPRITE->animframe*2;
      if (i) tileid+=2;
      else tileid+=3;
      graf_draw_tile(&g.graf,g.texid_sprites,x,y,tileid,sprite->xform);
    }
  }
}

const struct sprite_type sprite_type_flamethrower={
  .name="flamethrower",
  .objlen=sizeof(struct sprite_flamethrower),
  .del=_flamethrower_del,
  .init=_flamethrower_init,
  .update=_flamethrower_update,
  .render=_flamethrower_render,
};
