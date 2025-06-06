/* sprite.h
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;

struct sprite {
  const struct sprite_type *type;
  uint32_t arg; // From spawn command.
  int rid;
  const void *cmdv;
  int cmdc;
  double x,y; // In meters (ie map cells)
  uint8_t tileid,xform;
  double phl,phr,pht,phb; // Signed offset to my four edges.
  double terminal_velocity; // Gets a global default initially. Zero for no gravity.
  int suspend_gravity; // Nonzero to suspend gravity temporarily. Controller sets and clears.
  int solid; // Nonzero to participate in sprite-on-sprite physics.
  uint32_t physics_mask; // Which physics values are solid to me, eg (1<<NS_physics_solid).
  int defunct;
  int layer; // Render order within a layer is undefined. Hero is 100, flowers are 50.
  int fragile; // Subject to burnination.
  
  // Private to physics:
  int graviting;
  double gravity; // m/s, volatile
  double gravclock;
  double pvx,pvy;
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  
  // Sprites that do not implement (render) must be single tiles from image:sprites.
  void (*render)(struct sprite *sprite,int x,int y);
  
  // Physics notifications.
  void (*fall_begin)(struct sprite *sprite);
  void (*fall_end)(struct sprite *sprite,double duration,struct sprite *floor); // (duration) in seconds. (floor) only if we landed on a sprite.
  
  // Only relevant to the hero. Other sprites don't survive transitions.
  void (*map_changed)(struct sprite *sprite);
  
  // Epicenter is the hero's position, where she hit the ground. All sprites that implement this get notified each time.
  void (*earthquake)(struct sprite *sprite,double epix,double epiy);
  
  void (*hurt)(struct sprite *sprite,struct sprite *assailant);
};

/* Plain sprite_new() DOES NOT call (type->init).
 * That must happen after the initial properties have been set.
 */
void sprite_del(struct sprite *sprite);
struct sprite *sprite_new(const struct sprite_type *type); // Prefer sprite_spawn(), below.

// These should only be used during scene transitions.
void sprite_unlist(struct sprite *sprite);
int sprite_relist(struct sprite *sprite);
void sprites_drop();

/* _cmd and _rid are only different in the coordinates format.
 * _type does not require a resource.
 */
struct sprite *sprite_spawn_cmd(int col,int row,int rid,uint32_t arg);
struct sprite *sprite_spawn_rid(double x,double y,int rid,uint32_t arg);
struct sprite *sprite_spawn_type(double x,double y,const struct sprite_type *type,uint32_t arg);

const struct sprite_type *sprite_type_from_id(int id);

#define _(tag) extern const struct sprite_type sprite_type_##tag;
SPRCTL_FOR_EACH
#undef _

/* Type-specific API.
 ***************************************************************************/
 
void hero_button_down(struct sprite *sprite,int btnid);
void hero_button_up(struct sprite *sprite,int btnid);

void sprite_flower_remove_by_flowerid(int flowerid);

void squishroom_compress(struct sprite *sprite);

void snore_drop_all();

// Any flamethrower. And we tolerate (r<l) by reversing them.
int flamethrower_touches_box(double l,double t,double r,double b);

#endif
