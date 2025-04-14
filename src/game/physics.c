#include "zs.h"

#define SPACE_FUDGE 0.010

/* Check and correct collisions for one sprite after applying gravity.
 * As a rule, we will only correct upward.
 * Returns nonzero if we corrected something.
 */
 
static int physics_resolve_gravity_collisions(struct sprite *sprite) {
  if (!sprite->physics_mask) return 0;

  /* Grid only needs checked if my foot passed from <= grid line to > grid line.
   */
  double pvy=sprite->pvy+sprite->phb;
  double y=sprite->y+sprite->phb;
  int row=(int)y;
  double frow=(double)row;
  if ((pvy<=frow)&&(y>frow)) {
    int cola=(int)(sprite->x+sprite->phl+SPACE_FUDGE); if (cola<0) cola=0;
    int colz=(int)(sprite->x+sprite->phr-SPACE_FUDGE); if (colz>=g.scene.map->w) colz=g.scene.map->w-1;
    const uint8_t *cell=g.scene.map->v+row*g.scene.map->w+cola;
    int col=cola; for (;col<=colz;col++,cell++) {
      uint8_t physics=g.physics[*cell];
      if (physics==NS_physics_oneway) physics=NS_physics_solid;
      if (sprite->physics_mask&(1<<physics)) {
        sprite->y=frow-sprite->phb;
        return 1;
      }
    }
  }
  
  //TODO Sprite-on-sprite collisions.
  return 0;
}

/* Escape sprite from a rectangular collision.
 */
 
static void physics_escape_box(struct sprite *sprite,double l,double t,double r,double b) {
  double escl=sprite->x+sprite->phr-l;
  double escr=r-sprite->phl-sprite->x;
  double esct=sprite->y+sprite->phb-t;
  double escb=b-sprite->pht-sprite->y;
  if ((escl<=escr)&&(escl<=esct)&&(escl<=escb)) {
    sprite->x=l-sprite->phr;
  } else if ((escr<=esct)&&(escr<=escb)) {
    sprite->x=r-sprite->phl;
  } else if (esct<=escb) {
    sprite->y=t-sprite->phb;
  } else {
    sprite->y=b-sprite->pht;
  }
}

/* Resolve sprite-on-map collisions.
 */
 
static void physics_resolve_map() {
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (!sprite->physics_mask) continue;
    int cola=(int)(sprite->x+sprite->phl); if (cola<0) cola=0;
    int colz=(int)(sprite->x+sprite->phr); if (colz>=g.scene.map->w) colz=g.scene.map->w-1;
    int rowa=(int)(sprite->y+sprite->pht); if (rowa<0) rowa=0;
    int rowz=(int)(sprite->y+sprite->phb); if (rowz>=g.scene.map->h) rowz=g.scene.map->h-1;
    if ((cola>colz)||(rowa>rowz)) continue; // Totally possible, for sprites entirely off-screen.
    
    // Move in the direction implied by (pvx,pvy). This is important, to avoid stubbing toes.
    int dcol,drow;
    if (sprite->pvx<=sprite->x) {
      dcol=1;
      colz++;
    } else {
      dcol=-1;
      int tmp=cola;
      cola=colz;
      colz=tmp;
      colz--;
    }
    if (sprite->pvy<=sprite->y) {
      drow=1;
      rowz++;
    } else {
      drow=-1;
      int tmp=rowa;
      rowa=rowz;
      rowz=tmp;
      rowz--;
    }
    
    const uint8_t *cellrow=g.scene.map->v+rowa*g.scene.map->w+cola;
    int row=rowa; for (;row!=rowz;row+=drow,cellrow+=g.scene.map->w*drow) {
      const uint8_t *cell=cellrow;
      int col=cola; for (;col!=colz;col+=dcol,cell+=dcol) {
        uint8_t physics=g.physics[*cell];
        if (!(sprite->physics_mask&(1<<physics))) continue;
        physics_escape_box(sprite,col,row,col+1.0,row+1.0);
      }
    }
  }
}

/* Resolve sprite-on-sprite collisions.
 */
 
static void physics_resolve_sprites() {
  //TODO
}

/* Replace each sprite's (pvx,pvy).
 */
 
static void physics_assign_pv() {
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    sprite->pvx=sprite->x;
    sprite->pvy=sprite->y;
  }
}

/* Apply gravity.
 */
 
static void physics_apply_gravity(double elapsed) {
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (sprite->suspend_gravity||(sprite->terminal_velocity<=0.0)) {
      sprite->gravclock=0.0f;
      continue;
    }
    
    sprite->gravity+=GRAVITY_ACCELERATION*elapsed;
    if (sprite->gravity<=0.0) continue;
    if (sprite->gravity>=sprite->terminal_velocity) {
      sprite->gravity=sprite->terminal_velocity;
    }
    sprite->gravclock+=elapsed;
    sprite->y+=sprite->gravity*elapsed;
    
    if (physics_resolve_gravity_collisions(sprite)) {
      if (sprite->graviting) {
        sprite->graviting=0;
        sprite->pvy=sprite->y;
        if (sprite->type->fall_end) sprite->type->fall_end(sprite,sprite->gravclock);
      } else {
        // Collision on a first-try for gravity: Forget we tried it.
        sprite->y=sprite->pvy;
      }
      sprite->gravity=GRAVITY_START;
      sprite->gravclock=0.0;
      
    } else {
      sprite->pvy=sprite->y;
      if (!sprite->graviting) {
        sprite->graviting=1;
        if (sprite->type->fall_begin) sprite->type->fall_begin(sprite);
      }
    }
  }
}

/* Update, main entry point.
 */
 
void physics_update(double elapsed) {
  physics_resolve_map();
  physics_resolve_sprites();
  physics_assign_pv();
  physics_apply_gravity(elapsed);
}

/* Downjump.
 */
 
int physics_downjump(struct sprite *sprite) {

  // Foot must be flush on a grid line, or extremely close:
  double bottom=sprite->y+sprite->phb;
  double whole,fract;
  fract=modf(bottom,&whole);
  if (fract>=0.999) whole+=1.0;
  else if (fract>0.001) return 0;
  int row=(int)whole;
  if (row>=g.scene.map->h) return 0;
  
  // Each cell under the sprite, on that row, must have physics vacant or oneway. OOB is ok.
  // At least one must be oneway.
  int cola=(int)(sprite->x+sprite->phl+SPACE_FUDGE); if (cola<0) cola=0;
  int colz=(int)(sprite->x+sprite->phr-SPACE_FUDGE); if (colz>=g.scene.map->w) colz=g.scene.map->w-1;
  int oneway=0;
  const uint8_t *cell=g.scene.map->v+row*g.scene.map->w+cola;
  int col=cola; for (;col<=colz;col++,cell++) {
    uint8_t physics=g.physics[*cell];
    switch (physics) {
      case NS_physics_oneway: oneway=1; // pass
      case NS_physics_vacant:
        break;
      default: return 0;
    }
  }
  
  // OK! Drop (y) and also (pvy) so at rectification we won't think we've passed thru the floor.
  sprite->y+=0.010;
  sprite->pvy=sprite->y;
  return 1;
}

/* Check point.
 */
 
int physics_check_point(double x,double y) {
  int col=(int)x;
  if ((col<0)||(col>=g.scene.map->w)) return 0;
  int row=(int)y;
  if ((row<0)||(row>=g.scene.map->h)) return 0;
  uint8_t cell=g.scene.map->v[row*g.scene.map->w+col];
  uint8_t physics=g.physics[cell];
  switch (physics) {
    case NS_physics_solid:
      return 1;
  }
  return 0;
}
