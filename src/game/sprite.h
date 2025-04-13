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
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  
  // Sprites that do not implement (render) must be single tiles from image:sprites.
  void (*render)(struct sprite *sprite,int x,int y);
};

/* Plain sprite_new() DOES NOT call (type->init).
 * That must happen after the initial properties have been set.
 */
void sprite_del(struct sprite *sprite);
struct sprite *sprite_new(const struct sprite_type *type); // Prefer sprite_spawn(), below.

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

#endif
