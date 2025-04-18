#include "zs.h"

#define SPACE_FUDGE 0.010

/* Check and correct collisions for one sprite after applying gravity.
 * As a rule, we will only correct upward.
 * Returns nonzero if we corrected something.
 */
 
static int physics_resolve_gravity_collisions(struct sprite *sprite,struct sprite **floor) {
  if (!sprite->physics_mask) return 0;

  /* Grid only needs checked if my foot passed from <= grid line to > grid line.
   */
  double pvy=sprite->pvy+sprite->phb;
  double y=sprite->y+sprite->phb;
  int row=(int)y;
  if (row<0) row=0;
  else if (row>=g.scene.map->h) row=g.scene.map->h-1;
  double frow=(double)row;
  if ((pvy<=frow)&&(y>frow)) {
    int cola=(int)(sprite->x+sprite->phl+SPACE_FUDGE);
    int colz=(int)(sprite->x+sprite->phr-SPACE_FUDGE);
    // Clamp each on both sides. That's important, to implicitly extend the last valid cell.
    if (cola<0) cola=0; else if (cola>=g.scene.map->w) cola=g.scene.map->w-1;
    if (colz<0) colz=0; else if (colz>=g.scene.map->w) colz=g.scene.map->w-1;
    const uint8_t *cell=g.scene.map->v+row*g.scene.map->w+cola;
    int col=cola; for (;col<=colz;col++,cell++) {
      uint8_t physics=g.physics[*cell];
      if (physics==NS_physics_oneway) physics=NS_physics_solid;
      if (physics==NS_physics_ladder) {
        if ((row>0)&&(g.physics[g.scene.map->v[(row-1)*g.scene.map->w+col]]==NS_physics_vacant)) {
          physics=NS_physics_solid;
        }
      }
      if (sprite->physics_mask&(1<<physics)) {
        sprite->y=frow-sprite->phb;
        return 1;
      }
    }
  }
  
  if (sprite->solid) {
    // For now, only the hero can correct in sprite-on-sprite collisions, and we're not going to correct downward.
    // So only do this for the hero. It's fine. No other solid sprite should be using gravity.
    if (sprite->type==&sprite_type_hero) {
      double al=sprite->x+sprite->phl+SPACE_FUDGE;
      double ar=sprite->x+sprite->phr-SPACE_FUDGE;
      double at=sprite->y+sprite->pht;
      double ab=sprite->y+sprite->phb;
      int bi=g.spritec;
      while (bi-->0) {
        struct sprite *b=g.spritev[bi];
        if (!b->solid) continue;
        if (b==sprite) continue;
        double bl=b->x+b->phl;
        double br=b->x+b->phr;
        double bt=b->y+b->pht;
        double bb=b->y+b->phb;
        if (al>=br) continue;
        if (ar<=bl) continue;
        if (ab<=bt) continue;
        if (at>=bb) continue;
        sprite->y=bt-sprite->phb;
        if (floor) *floor=b;
        return 1;
      }
    }
  }
  
  return 0;
}

/* Escape sprite from a rectangular collision.
 * Caller may indicate that one axis is to be unpreferred.
 */
 
static void physics_escape_box(struct sprite *sprite,double l,double t,double r,double b,int horzok,int vertok) {
  double escl=sprite->x+sprite->phr-l;
  double escr=r-sprite->phl-sprite->x;
  double esct=sprite->y+sprite->phb-t;
  double escb=b-sprite->pht-sprite->y;
  if (!horzok) { escl+=99.0; escr+=99.0; }
  if (!vertok) { esct+=99.0; escb+=99.0; }
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
    int cola=(int)(sprite->x+sprite->phl);
    int colz=(int)(sprite->x+sprite->phr);
    int rowa=(int)(sprite->y+sprite->pht);
    int rowz=(int)(sprite->y+sprite->phb);
    if (cola<0) cola=0; else if (cola>=g.scene.map->w) cola=g.scene.map->w-1;
    if (colz<0) colz=0; else if (colz>=g.scene.map->w) colz=g.scene.map->w-1;
    if (rowa<0) rowa=0; else if (rowa>=g.scene.map->h) rowa=g.scene.map->h-1;
    if (rowz<0) rowz=0; else if (rowz>=g.scene.map->h) rowz=g.scene.map->h-1;
    
    // Move in the direction implied by (pvx,pvy). This is important, to avoid stubbing toes.
    // Maybe swap (a,z), then advance (z) so we can treat it as exclusive.
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
    
    int s=g.scene.map->w;
    const uint8_t *cellrow=g.scene.map->v+rowa*g.scene.map->w+cola;
    int row=rowa; for (;row!=rowz;row+=drow,cellrow+=g.scene.map->w*drow) {
      const uint8_t *cell=cellrow;
      int col=cola; for (;col!=colz;col+=dcol,cell+=dcol) {
        uint8_t physics=g.physics[*cell];
        if (!(sprite->physics_mask&(1<<physics))) continue;
        double l=col,t=row,r=col+1.0,b=row+1.0;
        int horzok=1,vertok=1;
        if (!col) l=-999.0; else if (col>=g.scene.map->w-1) r=999.0; else if ((sprite->physics_mask&(1<<g.physics[cell[-1]]))&&(sprite->physics_mask&(1<<g.physics[cell[1]]))) horzok=0;
        if (!row) t=-999.0; else if (row>=g.scene.map->h-1) b=999.0; else if ((sprite->physics_mask&(1<<g.physics[cell[-s]]))&&(sprite->physics_mask&(1<<g.physics[cell[s]]))) vertok=0;
        physics_escape_box(sprite,l,t,r,b,horzok,vertok);
      }
    }
  }
}

/* Pick the sprite that will move to escape a sprite-on-sprite collision.
 * (-1,1) for (a,b). Anything else, skip it.
 */
 
static int physics_pick_moveable_sprite(const struct sprite *a,const struct sprite *b) {
  // I think we only care about sprite-on-sprite collisions when one of them is the hero.
  // That assumption might not hold forever.
  if (a->type==&sprite_type_hero) return -1;
  if (b->type==&sprite_type_hero) return 1;
  return 0;
}

/* Resolve sprite-on-sprite collisions.
 */
 
static void physics_resolve_sprites() {
  int ai=g.spritec;
  while (ai-->0) {
    struct sprite *a=g.spritev[ai];
    if (!a->solid) continue;
    double al=a->x+a->phl;
    double ar=a->x+a->phr;
    double at=a->y+a->pht;
    double ab=a->y+a->phb;
    int bi=ai;
    while (bi-->0) {
      struct sprite *b=g.spritev[bi];
      if (!b->solid) continue;
      double bl=b->x+b->phl;
      double br=b->x+b->phr;
      double bt=b->y+b->pht;
      double bb=b->y+b->phb;
      
      // Calculate escapements for (a), regardless of who will actually do the moving.
      // If any escapement is zero or negative, there's no collision.
      double escl=ar-bl; if (escl<=0.0) continue;
      double escr=br-al; if (escr<=0.0) continue;
      double esct=ab-bt; if (esct<=0.0) continue;
      double escb=bb-at; if (escb<=0.0) continue;
      double dx=0.0,dy=0.0;
      if ((escl<=escr)&&(escl<=esct)&&(escl<=escb)) dx=-escl;
      else if ((escr<=esct)&&(escr<=escb)) dx=escr;
      else if (esct<=escb) dy=-esct;
      else dy=escb;
      
      // Apply entirely to one or the other. If we want pushable sprites, just apportion this delta by inverse mass and apply to both.
      switch (physics_pick_moveable_sprite(a,b)) {
        case 1: {
            b->x-=dx;
            b->y-=dy;
          } break;
        case -1: {
            a->x+=dx;
            a->y+=dy;
            al=a->x+a->phl;
            ar=a->x+a->phr;
            at=a->y+a->pht;
            ab=a->y+a->phb;
          } break;
      }
    }
  }
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

  // As a safety measure against infinite holes, stop applying gravity to sprites two meters below the world's bottom.
  // If you fall into a hole where there isn't a door, you can jump out of it.
  double absolute_floor=g.scene.map->h+2.0;

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
    
    struct sprite *floor=0;
    if (physics_resolve_gravity_collisions(sprite,&floor)||(sprite->y>=absolute_floor)) {
      if (sprite->graviting) {
        sprite->graviting=0;
        sprite->pvy=sprite->y;
        if (sprite->type->fall_end) sprite->type->fall_end(sprite,sprite->gravclock,floor);
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
  if (row<0) return 0;
  if (row>=g.scene.map->h) return 0;
  
  // Each cell under the sprite, on that row, must have physics vacant, oneway, or ladder. OOB is ok.
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
      case NS_physics_ladder:
        break;
      default: return 0;
    }
  }
  
  // OK! Drop (y) and also (pvy) so at rectification we won't think we've passed thru the floor.
  sprite->y+=0.010;
  sprite->pvy=sprite->y;
  return 1;
}

/* Teleport.
 */
 
int physics_teleport(struct sprite *sprite,double dx) {
  const double min=0.050;
  double ox=sprite->x;
  double nx=ox+dx;
  
  // Don't allow teleporting off-screen because it introduces a danger of penetrating the neighbor map too deeply.
  if (nx+sprite->phl<0.0) {
    nx=-sprite->phl;
  } else if (nx+sprite->phr>g.scene.map->w) {
    nx=g.scene.map->w-sprite->phr;
  }
  
  // First check the map only on the target bounds. If that position is legal, great.
  // Without this clause, Dot would not be able to teleport thru narrow walls. (we do want her to)
  int okpos=1;
  int rowa=(int)(sprite->y+sprite->pht+SPACE_FUDGE); if (rowa<0) rowa=0;
  int rowz=(int)(sprite->y+sprite->phb-SPACE_FUDGE); if (rowz>=g.scene.map->h) rowz=g.scene.map->h-1;
  {
    int cola=(int)(nx+sprite->phl); if (cola<0) cola=0;
    int colz=(int)(nx+sprite->phr); if (rowz>=g.scene.map->w) colz=g.scene.map->w-1;
    const uint8_t *cellrow=g.scene.map->v+rowa*g.scene.map->w+cola;
    int row=rowa; for (;row<=rowz;row++,cellrow+=g.scene.map->w) {
      const uint8_t *cellp=cellrow;
      int col=cola; for (;col<=colz;col++,cellp++) {
        uint8_t physics=g.physics[*cellp];
        switch (physics) {
          case NS_physics_solid: okpos=0; goto _done_precheck_;
        }
      }
    }
    if (sprite->solid) {
      double l=nx+sprite->phl;
      double r=nx+sprite->phr;
      double t=sprite->y+sprite->pht;
      double b=sprite->y+sprite->phb;
      struct sprite **p=g.spritev;
      int i=g.spritec;
      for (;i-->0;p++) {
        struct sprite *other=*p;
        if (!other->solid) continue;
        if (other==sprite) continue;
        double ol=other->x+other->phl;
        double or=other->x+other->phr;
        double ot=other->y+other->pht;
        double ob=other->y+other->phb;
        if (l>=or) continue;
        if (r<=ol) continue;
        if (t>=ob) continue;
        if (b<=ot) continue;
        okpos=0;
        goto _done_precheck_;
      }
    }
   _done_precheck_:;
  }
  
  // If there's a solid map cell, bump to its edge per dx sign. Walk from far to near.
  if (!okpos) {
    int cola,colz,dcol;
    if (dx<0.0) {
      cola=(int)(nx+sprite->phl);
      colz=(int)(sprite->x+sprite->phr);
      if (cola<0) cola=0; else if (cola>=g.scene.map->w) cola=g.scene.map->w-1;
      if (colz<0) colz=0; else if (colz>=g.scene.map->w) colz=g.scene.map->w-1;
      colz++;
      dcol=1;
    } else {
      cola=(int)(nx+sprite->phr);
      colz=(int)(sprite->x+sprite->phl);
      if (cola<0) cola=0; else if (cola>=g.scene.map->w) cola=g.scene.map->w-1;
      if (colz<0) colz=0; else if (colz>=g.scene.map->w) colz=g.scene.map->w-1;
      colz--;
      dcol=-1;
    }
    int col=cola; for (;col!=colz;col+=dcol) {
      const uint8_t *p=g.scene.map->v+rowa*g.scene.map->w+col;
      int row=rowa; for (;row<=rowz;row++,p+=g.scene.map->w) {
        uint8_t physics=g.physics[*p];
        switch (physics) {
          case NS_physics_solid: {
              if (dcol==1) {
                nx=(double)col+1.0-sprite->phl;
              } else {
                nx=(double)col-sprite->phr;
              }
            } break;
        }
      }
    }
    if (sprite->solid) {
      double l,r;
      if (dx<0.0) {
        l=nx+sprite->phl;
        r=sprite->x+sprite->phr;
      } else {
        l=sprite->x+sprite->phl;
        r=nx+sprite->phr;
      }
      double t=sprite->y+sprite->pht;
      double b=sprite->y+sprite->phb;
      struct sprite **p=g.spritev;
      int i=g.spritec;
      for (;i-->0;p++) {
        struct sprite *other=*p;
        if (!other->solid) continue;
        if (other==sprite) continue;
        double ot=other->y+other->pht;
        double ob=other->y+other->phb;
        if ((t>=ob)||(b<=ot)) continue;
        double ol=other->x+other->phl;
        double or=other->x+other->phr;
        if (l>=or) continue;
        if (r<=ol) continue;
        if (dx<0.0) {
          l=or;
          nx=l-sprite->phl;
        } else {
          r=ol;
          nx=r-sprite->phr;
        }
      }
    }
  }
  
  // If we've backed up beyond the minimum threshold, forget it.
  double d=nx-ox;
  if (dx<0.0) {
    if (d>-min) return 0;
  } else {
    if (d<min) return 0;
  }
  
  // Apply change. Write it to (pvx) too, so we won't accidentally undo it during rectification.
  sprite->x=sprite->pvx=nx;
  return 1;
}

/* Check point.
 */
 
int physics_check_point(double x,double y) {
  int col=(int)x;
  if (col<0) col=0;
  else if (col>=g.scene.map->w) col=g.scene.map->w-1;
  int row=(int)y;
  if (row<0) row=0;
  else if (row>=g.scene.map->h) row=g.scene.map->h-1;
  uint8_t cell=g.scene.map->v[row*g.scene.map->w+col];
  uint8_t physics=g.physics[cell];
  switch (physics) {
    case NS_physics_solid:
      return 1;
  }
  // This is currently only used for walljump and wallgrab. I can live without sprite-on-sprite collisions.
  return 0;
}
