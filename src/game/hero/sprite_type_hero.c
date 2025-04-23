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
  sprite->layer=100;
  sprite->physics_mask=(1<<NS_physics_solid);
  sprite->pht=-1.125;
  SPRITE->jump_power=HERO_JUMP_POWER_DEFAULT;
  SPRITE->seated=1;
  hero_history_zap(sprite);
  return 0;
}

/* Change map. Drop transient things.
 */
 
static void _hero_map_changed(struct sprite *sprite) {
  hero_history_zap(sprite);
  SPRITE->flower=0;
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
  if (SPRITE->walkdx&&SPRITE->seated) {
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
      hero_rejoin_ladder(sprite);
      return;
    }
    sprite->y-=SPRITE->walljump_ypower*elapsed;
    return;
  }
  
  // Regular jump.
  SPRITE->jump_power-=elapsed*HERO_JUMP_POWER_DECELERATION;
  if (SPRITE->jump_power<=0.0) {
    sprite->suspend_gravity=0;
    hero_rejoin_ladder(sprite);
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
  SPRITE->flower=0;
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

/* Check for a candidate flower.
 * We'll poll this every frame.
 * (except it doesn't happen while on a ladder; that's fine, there's definitely no flower-picking while laddered)
 */
 
static void hero_update_flower(struct sprite *sprite) {
  SPRITE->flower=0;
  if (!SPRITE->seated) return;
  if (g.session.bouquetc>=BOUQUET_LIMIT) return;
  
  /* This polls often so keep it lean: First check tileid, since that's constant-time.
   * If it's a flowerable tile, then search by the exact location (O(log2(n)) time).
   */
  int col=(int)sprite->x,row=(int)sprite->y;
  if ((col<0)||(col>=g.scene.map->w)||(row<0)||(row>=g.scene.map->h)) return;
  uint8_t tileid=g.scene.map->v[row*g.scene.map->w+col];
  if ((tileid<0x05)||(tileid>0x08)) return;
  int flowerp=session_flowerp_by_location(g.scene.map->rid,col,row);
  if (flowerp<0) return;
  
  /* We're going to display the bouquet content with a single digit per color.
   * There is never a situation where you'd want more than 9 of a color, that's just too many.
   * If we already have 9 of this color, don't allow picking.
   */
  struct session_flower *flower=g.session.flowerv+flowerp;
  const struct session_flower *q=g.session.bouquetv;
  int alreadyc=0,i=g.session.bouquetc;
  for (;i-->0;q++) {
    if (q->colorid==flower->colorid) alreadyc++;
  }
  if (alreadyc>=9) return;
  
  SPRITE->flower=flower;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  hero_animate(sprite,elapsed);
  
  if ((SPRITE->teleport_highlight-=elapsed)<=0.0) {
    SPRITE->teleport_highlight=0.0;
  }
  
  if (SPRITE->autoclock>0.0) {
    if ((SPRITE->autoclock-=elapsed)<=0.0) {
      sprite->suspend_gravity=0;
    } else {
      sprite->x+=SPRITE->autodx*elapsed;
      sprite->y+=SPRITE->autody*elapsed;
      return;
    }
  }
    
  
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
  hero_update_flower(sprite);
}

/* Flower indicator.
 */
 
static void hero_draw_flower_indicator(struct sprite *sprite,int x,int y) {
  y-=NS_sys_tilesize*2;
  graf_draw_tile(&g.graf,g.texid_sprites,x-NS_sys_tilesize,y,0x22,0);
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,0x23,0);
  graf_draw_tile(&g.graf,g.texid_sprites,x+NS_sys_tilesize-3,y-2,SPRITE->flower->tileid,0);
  graf_set_tint(&g.graf,SPRITE->flower->tint);
  graf_draw_tile(&g.graf,g.texid_sprites,x+NS_sys_tilesize-3,y-2,SPRITE->flower->tileid+0x10,0);
  graf_set_tint(&g.graf,0);
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
  } else if (!SPRITE->seated) {
    col=14;
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
  if (SPRITE->teleport_highlight>0.0) {
    int alpha=(SPRITE->teleport_highlight*255.0)/HERO_TELEPORT_HIGHLIGHT_TIME;
    if (alpha>0xff) alpha=0xff;
    graf_set_tint(&g.graf,0xffffff00|alpha);
    graf_draw_tile(&g.graf,g.texid_sprites,x,y-NS_sys_tilesize,0x00+col,sprite->xform);
    graf_draw_tile(&g.graf,g.texid_sprites,x,y,0x10+col,sprite->xform);
    graf_set_tint(&g.graf,0);
  } else {
    graf_draw_tile(&g.graf,g.texid_sprites,x,y-NS_sys_tilesize,0x00+col,sprite->xform);
    graf_draw_tile(&g.graf,g.texid_sprites,x,y,0x10+col,sprite->xform);
  }
  
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
  
  /* If there's a pickable flower, draw an indicator above her head.
   */
  if (SPRITE->flower) hero_draw_flower_indicator(sprite,x,y);
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
  .map_changed=_hero_map_changed,
  .hurt=hero_hurt,
};

/* Begin echo.
 */
 
void hero_begin_echo(struct sprite *sprite,int framec) {
  SPRITE->echo_record=framec;
}
