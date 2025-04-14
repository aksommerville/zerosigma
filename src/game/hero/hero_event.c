#include "hero_internal.h"

/* Gravity notifications.
 */
 
void hero_fall_begin(struct sprite *sprite) {
  fprintf(stderr,"%s\n",__func__);
}

void hero_fall_end(struct sprite *sprite) {
  fprintf(stderr,"%s\n",__func__);
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
}

/* Jump input.
 */
 
static void hero_jump_begin(struct sprite *sprite) {
}

static void hero_jump_end(struct sprite *sprite) {
}

/* Item input.
 */
 
static void hero_item_begin(struct sprite *sprite) {
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
