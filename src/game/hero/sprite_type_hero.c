#include "hero_internal.h"

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Zap history.
 */
 
static void hero_history_zap(struct sprite *sprite) {
  SPRITE->echoc=0;
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  sprite->physics_mask=(1<<NS_physics_solid);
  sprite->pht=-1.125;
  SPRITE->jump_power=HERO_JUMP_POWER_DEFAULT;
  SPRITE->seated=1;
  hero_history_zap(sprite);
  return 0;
}

/* Update animation.
 */
 
static void hero_animate(struct sprite *sprite,double elapsed) {

  // Climbing ladder: Hold current frame, or animate if moving vertically.
  // It's only 4 frames, at half the speed of walking, but we use the same walk timing anyway.
  if (SPRITE->ladderx>0.0) {
    if (!SPRITE->indy) return;
    if ((SPRITE->animclock-=elapsed)>0.0) return;
    SPRITE->animclock+=0.100;
    if (++(SPRITE->animframe)>=8) SPRITE->animframe=0;
    return;
  }

  if ((SPRITE->animclock-=elapsed)>0.0) return;
  SPRITE->animclock+=0.100;
  
  // Walking: Cycle 0..7
  if (SPRITE->walkdx) {
    if (++(SPRITE->animframe)>=8) SPRITE->animframe=0;
    
  // Not walking: Stop at 0 or 4.
  } else {
    switch (SPRITE->animframe) {
      case 0: break;
      case 1: SPRITE->animframe=0; break;
      case 4: break;
      case 5: SPRITE->animframe=4; break;
      case 7: SPRITE->animframe=0; break;
      default: SPRITE->animframe++;
    }
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
 
static void hero_update_jump(struct sprite *sprite,double elapsed) {
  if (!sprite->suspend_gravity) return;
  
  // Wall jump.
  if (SPRITE->walljump) {
    if (SPRITE->walljump_xpower<0.0) {
      if ((SPRITE->walljump_xpower+=elapsed*HERO_WALLJUMPX_DECELERATION)>=0.0) {
        SPRITE->walljump_xpower=0.0;
      } else {
        sprite->x+=SPRITE->walljump_xpower*elapsed;
      }
    } else if (SPRITE->walljump_xpower>0.0) {
      if ((SPRITE->walljump_xpower-=elapsed*HERO_WALLJUMPX_DECELERATION)<=0.0) {
        SPRITE->walljump_xpower=0.0;
      } else {
        sprite->x+=SPRITE->walljump_xpower*elapsed;
      }
    }
    if ((SPRITE->walljump_ypower-=elapsed*HERO_WALLJUMPY_DECELERATION)<=0.0) {
      SPRITE->walljump=0;
      sprite->suspend_gravity=0;
      sprite->gravity=0.0;
      SPRITE->jump_power=0.0;
      if (SPRITE->indx<0) sprite->xform=EGG_XFORM_XREV;
      else if (SPRITE->indx>0) sprite->xform=0;
      return;
    }
    sprite->y-=SPRITE->walljump_ypower*elapsed;
    return;
  }
  
  // Regular jump.
  SPRITE->jump_power-=elapsed*HERO_JUMP_POWER_DECELERATION;
  if (SPRITE->jump_power<=0.0) {
    sprite->suspend_gravity=0;
    return;
  }
  sprite->y-=SPRITE->jump_power*elapsed;
}

/* Check wallgrab.
 * To reduce the complexity of checking state changes, I'm polling for this every frame.
 */
 
static void hero_update_wallgrab(struct sprite *sprite) {
  if (SPRITE->wallgrab) {
    SPRITE->wallgrab=0;
    sprite->terminal_velocity=DEFAULT_TERMINAL_VELOCITY;
  }
  if (!SPRITE->indx) return; // Only grabbing while actively pressing the wall.
  if (SPRITE->seated) return; // Must be aerial, either jumping or falling.
  if (SPRITE->fastfall) return; // Oh Dot, make up your mind.
  if (SPRITE->injump&&(SPRITE->jump_power>HERO_WALLGRAB_JUMP_POWER_LIMIT)) return;
  double x=sprite->x+0.550*SPRITE->indx;
  double y=sprite->y-0.333;
  if (!physics_check_point(x,y)) return;
  SPRITE->wallgrab=1;
  SPRITE->echo_record=0;
  sprite->terminal_velocity=HERO_WALLGRAB_VELOCITY;
}

/* Ladder.
 */
 
static void hero_update_ladder(struct sprite *sprite,double elapsed) {
  sprite->x=SPRITE->ladderx;
  if (SPRITE->indy) {
    sprite->y+=HERO_LADDER_CLIMB_SPEED*elapsed*SPRITE->indy;
    int col=(int)sprite->x,row=(int)sprite->y;
    if ((row<0)||(row>=g.scene.map->h)) {
      // OOB vertically, just roll with it. Probably a ladder into an adjoining map.
    } else if ((col<0)||(col>=g.scene.map->w)||(g.physics[g.scene.map->v[row*g.scene.map->w+col]]!=NS_physics_ladder)) {
      SPRITE->ladderx=0.0;
      SPRITE->animclock=0.0;
      SPRITE->animframe=0;
      sprite->suspend_gravity=0;
      sprite->gravity=0.0;
      SPRITE->seated=1;
      SPRITE->jump_power=HERO_JUMP_POWER_DEFAULT;
      if (SPRITE->indy>0) {
        sprite->y=row-0.5;
      } else {
        sprite->y=row+0.5;
      }
    }
  }
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  hero_animate(sprite,elapsed);
  
  if (SPRITE->ladderx>0.0) {
    hero_update_ladder(sprite,elapsed);
    return;
  }
  
  if (SPRITE->suspendx>0.0) {
    SPRITE->suspendx-=elapsed;
    if ((SPRITE->walkdx<0)&&(SPRITE->walljump_xpower<=0.0)) hero_update_walk(sprite,elapsed);
    else if ((SPRITE->walkdx>0)&&(SPRITE->walljump_xpower>=0.0)) hero_update_walk(sprite,elapsed);
  } else if (SPRITE->walkdx&&(SPRITE->sorefoot<=0.0)) {
    hero_update_walk(sprite,elapsed);
  }
  
  if (SPRITE->injump) hero_update_jump(sprite,elapsed);
  
  if (SPRITE->fastfall) SPRITE->fastfall_clock+=elapsed;
  
  if (SPRITE->sorefoot>0.0) {
    if ((SPRITE->sorefoot-=elapsed)<=0.0) {
      SPRITE->sorefoot=0.0;
    }
  }
  
  hero_update_wallgrab(sprite);
}

/* Render.
 * (x,y) are the middle of my body, the lower tile.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y) {

  /* If we have an echo, render and advance it.
   */
  if (SPRITE->echoc>0) {
    graf_set_tint(&g.graf,0xffffffff);
    int p=SPRITE->echop,i=SPRITE->echoc;
    while (i-->0) {
      struct hero_echo *echo=SPRITE->echov+p;
      if (--(echo->ttl)<=0) {
        if (p==SPRITE->echop) {
          SPRITE->echoc--;
          if (++(SPRITE->echop)>=HERO_ECHO_LIMIT) SPRITE->echop=0;
        }
        if (++p>=HERO_ECHO_LIMIT) p=0;
        continue;
      }
      if (++p>=HERO_ECHO_LIMIT) p=0;
      graf_set_alpha(&g.graf,echo->ttl*4);
      int ex=(int)(echo->x*NS_sys_tilesize)-g.scene.scrollx;
      int ey=(int)(echo->y*NS_sys_tilesize)-g.scene.scrolly;
      graf_draw_tile(&g.graf,g.texid_sprites,ex,ey-NS_sys_tilesize,echo->tileid,echo->xform);
      graf_draw_tile(&g.graf,g.texid_sprites,ex,ey,echo->tileid,echo->xform);
    }
    graf_set_tint(&g.graf,0);
    graf_set_alpha(&g.graf,0xff);
  }

  /* Main body is always two tiles, in the same column.
   */
  uint8_t col=0;
  if (SPRITE->ladderx>0.0) {
    switch (SPRITE->animframe>>1) {
      case 0: col=0x0b; break;
      case 1: col=0x0c; break;
      case 2: col=0x0d; break;
      case 3: col=0x0c; break;
    }
  } else if (SPRITE->sorefoot>0.0) {
    col=7;
  } else if (SPRITE->fastfall) {
    col=(SPRITE->animframe&1)?5:6;
  } else if (SPRITE->wallgrab) {
    col=9;
  } else if (SPRITE->walljump) {
    col=10;
  } else {
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
  }
  graf_draw_tile(&g.graf,g.texid_sprites,x,y-NS_sys_tilesize,0x00+col,sprite->xform);
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,0x10+col,sprite->xform);
  
  /* If we're recording echoes, do that.
   */
  if (SPRITE->echo_record>0) {
    SPRITE->echo_record--;
    int p=SPRITE->echop+SPRITE->echoc;
    if (p>=HERO_ECHO_LIMIT) p-=HERO_ECHO_LIMIT;
    struct hero_echo *echo=SPRITE->echov+p;
    if (SPRITE->echoc<HERO_ECHO_LIMIT) SPRITE->echoc++;
    else SPRITE->echop++;
    echo->x=sprite->x;
    echo->y=sprite->y;
    echo->tileid=col;
    echo->xform=sprite->xform;
    echo->ttl=15; // limit 63
  }
  
  /* When foot sore, draw a red outline fading out.
   */
  if (SPRITE->sorefoot>0.0) {
    int alpha=(int)(SPRITE->sorefoot*500.0);
    if (alpha>0xff) alpha=0xff;
    graf_set_alpha(&g.graf,alpha);
    graf_draw_tile(&g.graf,g.texid_sprites,x,y-NS_sys_tilesize,0x08,sprite->xform);
    graf_draw_tile(&g.graf,g.texid_sprites,x,y,0x18,sprite->xform);
    graf_set_alpha(&g.graf,0xff);
  }
  
  /* When fastfalling, her hat is disconnected.
   */
  if (SPRITE->fastfall) {
    const double minextent=(double)NS_sys_tilesize*0.500;
    const double maxextent=(double)NS_sys_tilesize*1.250;
    const double duration=0.333;
    double t=SPRITE->fastfall_clock/duration;
    if (t>1.0) t=1.0;
    int dy=-(int)(minextent+(maxextent-minextent)*t);
    graf_draw_tile(&g.graf,g.texid_sprites,x,y-NS_sys_tilesize+dy,0x20+(SPRITE->animframe&1),sprite->xform);
  }
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
  .map_changed=hero_history_zap,
};

/* Begin echo.
 */
 
void hero_begin_echo(struct sprite *sprite,int framec) {
  SPRITE->echo_record=framec;
}
