#include "hero_internal.h"

/* Gravity notifications.
 */
 
void hero_fall_begin(struct sprite *sprite) {
  SPRITE->seated=0;
}

void hero_fall_end(struct sprite *sprite,double pressure,struct sprite *floor) {
  SPRITE->seated=1;
  SPRITE->jump_power=HERO_JUMP_POWER_DEFAULT;
  
  if (SPRITE->fastfall) {
    SPRITE->fastfall=0;
    SPRITE->echo_record=0;
    sprite->terminal_velocity=DEFAULT_TERMINAL_VELOCITY;
    SPRITE->sorefoot=0.500;
    if (floor&&(floor->type==&sprite_type_squishroom)) {
      squishroom_compress(floor);
    } else {
      egg_play_sound(RID_sound_thump_fastfall);
      scene_begin_earthquake();
    }
    return;
  }
  
  if (pressure>=1.500) { // >=16m
    egg_play_sound(RID_sound_thump_huge);
    //TODO Stun Dot briefly
    //TODO Dust clouds
  } else if (pressure>=1.000) { // >=9m
    egg_play_sound(RID_sound_thump_big);
    //TODO Stun Dot briefly
    //TODO Dust clouds
  } else if (pressure>=0.750) { // >=6m
    egg_play_sound(RID_sound_thump_middle);
    //TODO Dust clouds
  } else if (pressure>=0.500) { // >=3m
    egg_play_sound(RID_sound_thump_little);
  }
}

/* Horizontal input state change.
 */
 
static void hero_indx_changed(struct sprite *sprite) {

  // When it changes to zero, check whether one of the buttons is pressed.
  // This can happen especially with keyboards.
  if (!SPRITE->indx) {
    switch (SPRITE->input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
      case EGG_BTN_LEFT: SPRITE->indx=-1; break;
      case EGG_BTN_RIGHT: SPRITE->indx=1; break;
    }
  }

  // Update transform and walk direction.
  if (SPRITE->indx<0) {
    sprite->xform=EGG_XFORM_XREV;
    SPRITE->walkdx=-1;
  } else if (SPRITE->indx>0) {
    sprite->xform=0;
    SPRITE->walkdx=1;
  } else {
    SPRITE->walkdx=0;
  }
  
  // If we're entering a positive state, bump the animation clock to zero to force a face change.
  // Also release the ladder if applicable.
  if (SPRITE->indx) {
    SPRITE->animclock=0.0;
    if (SPRITE->ladderx>0.0) {
      SPRITE->ladderx=0.0;
      sprite->suspend_gravity=0;
    }
  }
}

/* Grab a ladder if there's one handy.
 * Returns nonzero if grabbed, and updates all relevant state.
 */
 
static int hero_check_ladder(struct sprite *sprite) {
  if (SPRITE->sorefoot>0.0) return 0;
  int col=(int)sprite->x,row=(int)sprite->y;
  if ((col<0)||(row<0)||(col>=g.scene.map->w)||(row>=g.scene.map->h)) return 0;
  uint8_t physics=g.physics[g.scene.map->v[row*g.scene.map->w+col]];
  if (physics!=NS_physics_ladder) {
    // No ladder at my center, but am I standing on one?
    if ((SPRITE->indy>0)&&(row<g.scene.map->h-1)&&(g.physics[g.scene.map->v[(row+1)*g.scene.map->w+col]]==NS_physics_ladder)) {
      sprite->y+=0.75;
      // and proceed
    } else {
      return 0;
    }
  } else if (SPRITE->seated&&(SPRITE->indy>0)) {
    // Ladder here but I'm on the ground and they said "down". Don't do it.
    return 0;
  }
  sprite->x=SPRITE->ladderx=col+0.5;
  sprite->suspend_gravity=1;
  sprite->terminal_velocity=DEFAULT_TERMINAL_VELOCITY;
  SPRITE->fastfall=0;
  SPRITE->echo_record=0;
  SPRITE->wallgrab=0;
  SPRITE->seated=0;
  SPRITE->suspendx=0.0;
  SPRITE->walljump=0;
  return 1;
}

void hero_rejoin_ladder(struct sprite *sprite) {
  if (SPRITE->indy>=0) return;
  hero_check_ladder(sprite);
}

/* Pick flower.
 */
 
static void hero_pick_flower(struct sprite *sprite) {
  if (!SPRITE->flower) return;
  if (g.session.bouquetc>=BOUQUET_LIMIT) return;
  sprite_flower_remove_by_flowerid(SPRITE->flower->flowerid);
  int p=session_flowerp_by_flowerid(SPRITE->flower->flowerid);
  if (p<0) return;
  memcpy(g.session.bouquetv+g.session.bouquetc++,SPRITE->flower,sizeof(struct session_flower));
  egg_play_sound(RID_sound_pick);
  //TODO Visual fireworks?
  g.session.flowerc--;
  memmove(SPRITE->flower,SPRITE->flower+1,sizeof(struct session_flower)*(g.session.flowerc-p));
  SPRITE->flower=0;
}

/* Vertical input state change.
 */
 
static void hero_indy_changed(struct sprite *sprite) {

  /* If we're proposing a flower-pick, and user pressed Up, do it.
   */
  if (SPRITE->flower&&(SPRITE->indy<0)) {
    hero_pick_flower(sprite);
    return;
  }

  /* If we're on a ladder, no action necessary.
   * Otherwise, on nonzero changes, check whether there's a ladder to grab.
   */
  if (SPRITE->ladderx>0.0) return;
  if (SPRITE->indy) {
    if (hero_check_ladder(sprite)) return;
  }

  /* Abort jump with fastfall.
   */
  if ((SPRITE->indy>0)&&SPRITE->injump&&(SPRITE->jump_power>0.0)) {
    SPRITE->jump_power=0.0;
    sprite->suspend_gravity=0;
    SPRITE->walljump=0;
    SPRITE->fastfall=1;
    SPRITE->fastfall_clock=0.0;
    SPRITE->echo_record=999;
    sprite->gravity=sprite->terminal_velocity=HERO_FASTFALL_VELOCITY;
    return;
  }

  /* Other fastfall start and end cases.
   */
  if (sprite->graviting) {
    if (SPRITE->indy>0) {
      SPRITE->walljump=0;
      SPRITE->fastfall=1;
      SPRITE->fastfall_clock=0.0;
      SPRITE->echo_record=999;
      sprite->gravity=sprite->terminal_velocity=HERO_FASTFALL_VELOCITY;
    } else if (SPRITE->fastfall) {
      SPRITE->fastfall=0;
      SPRITE->echo_record=0;
      sprite->gravity=sprite->terminal_velocity=DEFAULT_TERMINAL_VELOCITY;
    }
    return;
  }
  if (SPRITE->fastfall) {
    SPRITE->fastfall=0;
    SPRITE->echo_record=0;
    sprite->terminal_velocity=DEFAULT_TERMINAL_VELOCITY;
  }

  //TODO Duck.
}

/* Begin down jump.
 */
 
static void hero_downjump(struct sprite *sprite) {
  if (physics_downjump(sprite)) {
    egg_play_sound(RID_sound_downjump);
    SPRITE->seated=0;
  } else {
    egg_play_sound(RID_sound_reject);
  }
}

/* Begin wall jump.
 */
 
static void hero_walljump(struct sprite *sprite,double dx) {
  egg_play_sound(RID_sound_walljump);
  sprite->suspend_gravity=1;
  SPRITE->walljump=1;
  SPRITE->walljump_xpower=dx*HERO_WALLJUMPX_INITIAL;
  SPRITE->walljump_ypower=HERO_WALLJUMPY_INITIAL;
  SPRITE->suspendx=HERO_WALLJUMP_SUSPENDX;
  if (dx<0.0) {
    sprite->xform=0;
  } else {
    sprite->xform=EGG_XFORM_XREV;
  }
  hero_begin_echo(sprite,10);
}

/* Jump input.
 */
 
static void hero_jump_begin(struct sprite *sprite) {

  if (SPRITE->sorefoot>0.0) return;
  
  // No matter what else happens, when you press Jump, you are no longer climbing a ladder.
  if (SPRITE->ladderx>0.0) {
    SPRITE->ladderx=0.0;
    SPRITE->animclock=0.0;
    SPRITE->animframe=0;
    sprite->suspend_gravity=0;
    SPRITE->jump_power=HERO_JUMP_POWER_DEFAULT;
  }

  /* Check wall jump, if I'm not footed. This does not require jump_power.
   * Examine a single point (well, two), just a little beyond my horizontal edges, at my vertical center.
   * If it's solid on both sides, wall-jump straight up.
   * Just one side, kick up and out.
   * Neither, proceed to regular jump.
   */
  if (sprite->suspend_gravity||(sprite->gravclock>0.0)) {
    const double radius=0.600;
    double y=sprite->y-0.333;
    int l=physics_check_point(sprite->x-radius,y);
    int r=physics_check_point(sprite->x+radius,y);
    if (l&&r) {
      hero_walljump(sprite,0.0);
      return;
    } else if (l) {
      hero_walljump(sprite,1.0);
      return;
    } else if (r) {
      hero_walljump(sprite,-1.0);
      return;
    }
  }

  if (SPRITE->jump_power<=0.0) return;
  if (sprite->gravclock>HERO_COYOTE_TIME) return;
  if (SPRITE->indy>0) {
    hero_downjump(sprite);
    SPRITE->injump=0;
    SPRITE->jump_power=HERO_JUMP_POWER_DEFAULT;
  } else {
    sprite->suspend_gravity=1;
    SPRITE->seated=0;
    egg_play_sound(RID_sound_jump);
    hero_begin_echo(sprite,5);
  }
}

static void hero_jump_end(struct sprite *sprite) {
  if (SPRITE->ladderx>0.0) return;
  if (sprite->graviting) {
    // Released while falling, after the jump power expired. Let hero_fall_end() restore it.
  } else if ((SPRITE->jump_power>0.0)&&SPRITE->injump) {
    // Cut jump short. Let gravity take over, and hero_fall_end() will restore jump power.
    SPRITE->jump_power=0.0;
  } else {
    // Already landed. Restore jump power now.
    SPRITE->jump_power=HERO_JUMP_POWER_DEFAULT;
  }
  if (SPRITE->walljump) {
    SPRITE->walljump=0;
    sprite->gravity=0.0;
    if (SPRITE->indx<0) sprite->xform=EGG_XFORM_XREV;
    else if (SPRITE->indx>0) sprite->xform=0;
  }
  sprite->suspend_gravity=0;
  hero_rejoin_ladder(sprite);
}

/* Teleport
 */
 
static void hero_teleport(struct sprite *sprite) {
  if (SPRITE->sorefoot>0.0) return;
  if (SPRITE->ladderx>0.0) return;
  const double reach=2.500;
  double ox=sprite->x;
  if (physics_teleport(sprite,(sprite->xform?-reach:reach))) {
    egg_play_sound(RID_sound_teleport);
    uint32_t arg=sprite->xform;
    struct sprite *teleghost=sprite_spawn_type(ox,sprite->y,&sprite_type_teleghost,arg);
    SPRITE->teleport_highlight=HERO_TELEPORT_HIGHLIGHT_TIME;
  } else {
    egg_play_sound(RID_sound_teleport_reject);
    SPRITE->teleport_highlight=HERO_TELEPORT_REJECT_TIME;
  }
}

/* Input state change.
 */
 
void hero_button_down(struct sprite *sprite,int btnid) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return;
  SPRITE->input|=btnid;
  switch (btnid) {
    case EGG_BTN_LEFT: if (SPRITE->indx<0) return; SPRITE->indx=-1; hero_indx_changed(sprite); break;
    case EGG_BTN_RIGHT: if (SPRITE->indx>0) return; SPRITE->indx=1; hero_indx_changed(sprite); break;
    case EGG_BTN_UP: if (SPRITE->indy<0) return; SPRITE->indy=-1; hero_indy_changed(sprite); break;
    case EGG_BTN_DOWN: if (SPRITE->indy>0) return; SPRITE->indy=1; hero_indy_changed(sprite); break;
    case EGG_BTN_SOUTH: if (SPRITE->injump) return; SPRITE->injump=1; hero_jump_begin(sprite); break;
    case EGG_BTN_WEST: if (SPRITE->initem) return; SPRITE->initem=1; hero_teleport(sprite); break;
  }
}

void hero_button_up(struct sprite *sprite,int btnid) {
  if (!sprite||(sprite->type!=&sprite_type_hero)) return;
  SPRITE->input&=~btnid;
  switch (btnid) {
    case EGG_BTN_LEFT: if (SPRITE->indx>=0) return; SPRITE->indx=0; hero_indx_changed(sprite); break;
    case EGG_BTN_RIGHT: if (SPRITE->indx<=0) return; SPRITE->indx=0; hero_indx_changed(sprite); break;
    case EGG_BTN_UP: if (SPRITE->indy>=0) return; SPRITE->indy=0; hero_indy_changed(sprite); break;
    case EGG_BTN_DOWN: if (SPRITE->indy<=0) return; SPRITE->indy=0; hero_indy_changed(sprite); break;
    case EGG_BTN_SOUTH: if (!SPRITE->injump) return; SPRITE->injump=0; hero_jump_end(sprite); break;
    case EGG_BTN_WEST: if (!SPRITE->initem) return; SPRITE->initem=0; break;
  }
}
