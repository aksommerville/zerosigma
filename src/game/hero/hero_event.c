#include "hero_internal.h"

/* Gravity notifications.
 */
 
void hero_fall_begin(struct sprite *sprite) {
}

void hero_fall_end(struct sprite *sprite,double pressure) {
  SPRITE->jump_power=HERO_JUMP_POWER_DEFAULT;
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
  if (SPRITE->indx) {
    SPRITE->animclock=0.0;
  }
}

/* Vertical input state change.
 */
 
static void hero_indy_changed(struct sprite *sprite) {
  //TODO Ladders.
  //TODO Duck.
}

/* Begin down jump.
 */
 
static void hero_downjump(struct sprite *sprite) {
  if (physics_downjump(sprite)) {
    egg_play_sound(RID_sound_downjump);
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
}

/* Jump input.
 */
 
static void hero_jump_begin(struct sprite *sprite) {

  /* Check wall jump, if I'm not footed. This does not require jump_power.
   * Examine a single point (well, two), just a little beyond my horizontal edges, at my vertical center.
   * If it's solid on both sides, wall-jump straight up.
   * Just one side, kick up and out.
   * Neither, proceed to regular jump.
   */
  if (sprite->suspend_gravity||(sprite->gravclock>0.0)) {
    const double radius=0.600;
    int l=physics_check_point(sprite->x-radius,sprite->y);
    int r=physics_check_point(sprite->x+radius,sprite->y);
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
    egg_play_sound(RID_sound_jump);
  }
}

static void hero_jump_end(struct sprite *sprite) {
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
  }
  sprite->suspend_gravity=0;
}

/* Item input.
 */
 
static void hero_item_begin(struct sprite *sprite) {
  //TODO Items. Will there be such a thing?
}

static void hero_item_end(struct sprite *sprite) {
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
    case EGG_BTN_WEST: if (SPRITE->initem) return; SPRITE->initem=1; hero_item_begin(sprite); break;
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
    case EGG_BTN_WEST: if (!SPRITE->initem) return; SPRITE->initem=0; hero_item_end(sprite); break;
  }
}
