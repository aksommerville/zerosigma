/* sprite_type_goat.c
 * Sleeps until you ground-pound on its floor. Then it wakes up and eats flowers.
 */
 
#include "game/zs.h"

#define WALK_SPEED 3.0

struct sprite_goat {
  struct sprite hdr;
  int sleeping;
  double snoreclock;
  double xlo,xhi; // extent of my roaming range
  uint8_t tileid0;
  double animclock;
  int animframe;
  double eatclock;
};

#define SPRITE ((struct sprite_goat*)sprite)

static int goat_tile_vacant(struct sprite *sprite,uint8_t tileid) {
  uint8_t physics=g.physics[tileid];
  switch (physics) {
    case NS_physics_vacant:
    case NS_physics_ladder:
      return 1;
  }
  return 0;
}

static int goat_tile_floor(struct sprite *sprite,uint8_t tileid) {
  uint8_t physics=g.physics[tileid];
  switch (physics) {
    case NS_physics_solid:
    case NS_physics_oneway:
    case NS_physics_ladder:
      return 1;
  }
  return 0;
}

static void goat_measure_bounds(struct sprite *sprite) {
  SPRITE->xlo=SPRITE->xhi=sprite->x;
  int col=(int)sprite->x;
  int row=(int)sprite->y;
  // Initial cell must be in bounds and must not be on the bottom row. We're going to examine both (row) and (row+1).
  if ((col<0)||(col>=g.scene.map->w)||(row<0)||(row>=g.scene.map->h-1)) return;
  const uint8_t *p=g.scene.map->v+row*g.scene.map->w+col;
  if (!goat_tile_vacant(sprite,*p)) return;
  if (!goat_tile_floor(sprite,p[g.scene.map->w])) return;
  int colc=1;
  while ((col>0)&&goat_tile_vacant(sprite,p[-1])&&goat_tile_floor(sprite,p[g.scene.map->w-1])) { col--; colc++; p--; }
  p+=colc;
  while ((col+colc<g.scene.map->w)&&goat_tile_vacant(sprite,p[0])&&goat_tile_floor(sprite,p[g.scene.map->w+1])) { colc++; p++; }
  SPRITE->xlo=col+0.5;
  SPRITE->xhi=col+colc-0.5;
}

static int _goat_init(struct sprite *sprite) {
  sprite->layer=101;
  sprite->terminal_velocity=0.0;
  SPRITE->sleeping=1;
  SPRITE->tileid0=sprite->tileid;
  goat_measure_bounds(sprite);
  return 0;
}

// If there's a flower here, remove it and begin the eating animation.
static void goat_check_flowers(struct sprite *sprite) {
  int col=(int)sprite->x;
  if ((col<0)||(col>=g.scene.map->w)) return;
  int row=(int)sprite->y;
  if ((row<0)||(row>=g.scene.map->h)) return;
  int flowerp=session_flowerp_by_location(g.scene.map->rid,col,row);
  if (flowerp<0) return;
  sprite_flower_remove_by_flowerid(g.session.flowerv[flowerp].flowerid);
  g.session.flowerc--;
  memmove(g.session.flowerv+flowerp,g.session.flowerv+flowerp+1,sizeof(struct session_flower)*(g.session.flowerc-flowerp));
  egg_play_sound(RID_sound_goatmeal);
  SPRITE->eatclock=2.0;
  SPRITE->animclock=0.0;
  SPRITE->animframe=0;
  sprite->tileid=SPRITE->tileid0+4;
}

static void _goat_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->sleeping) {
    if ((SPRITE->snoreclock-=elapsed)<=0.0) {
      SPRITE->snoreclock+=1.500;
      double x=sprite->x;
      double y=sprite->y;
      if (sprite->xform) x-=0.333;
      else x+=0.333;
      sprite_spawn_type(x,y,&sprite_type_snore,0);
    }
  } else if (SPRITE->eatclock>0.0) {
    if ((SPRITE->eatclock-=elapsed)<=0.0) {
      SPRITE->eatclock=0.0;
      SPRITE->animclock=0.0;
      SPRITE->animframe=-1;
      sprite->tileid=SPRITE->tileid0+1;
    } else {
      if ((SPRITE->animclock-=elapsed)<=0.0) {
        SPRITE->animclock+=0.200;
        if (++(SPRITE->animframe)>=2) SPRITE->animframe=0;
        sprite->tileid=SPRITE->tileid0+4+SPRITE->animframe;
      }
    }
  } else {
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=0.200;
      switch (++(SPRITE->animframe)) {
        default:
        case 0: SPRITE->animframe=0; sprite->tileid=SPRITE->tileid0+1; break;
        case 1: sprite->tileid=SPRITE->tileid0+2; break;
        case 2: sprite->tileid=SPRITE->tileid0+1; break;
        case 3: sprite->tileid=SPRITE->tileid0+3; break;
      }
    }
    sprite->x+=WALK_SPEED*elapsed*(sprite->xform?-1.0:1.0);
    if (sprite->x<SPRITE->xlo) {
      sprite->x=SPRITE->xlo;
      sprite->xform=0;
    } else if (sprite->x>SPRITE->xhi) {
      sprite->x=SPRITE->xhi;
      sprite->xform=EGG_XFORM_XREV;
    }
    goat_check_flowers(sprite);
  }
}

static void _goat_earthquake(struct sprite *sprite,double epix,double epiy) {
  if (!SPRITE->sleeping) return;
  if ((epix<SPRITE->xlo)||(epix>SPRITE->xhi)) return;
  double dy=epiy-sprite->y;
  if ((dy<-0.5)||(dy>0.5)) return;
  SPRITE->sleeping=0;
  sprite->tileid=SPRITE->tileid0+1;
  SPRITE->animclock=0.0;
  SPRITE->animframe=-1;
  snore_drop_all(); // Also kills snores for other goats, but that's probly ok.
}

const struct sprite_type sprite_type_goat={
  .name="goat",
  .objlen=sizeof(struct sprite_goat),
  .init=_goat_init,
  .update=_goat_update,
  .earthquake=_goat_earthquake,
};
