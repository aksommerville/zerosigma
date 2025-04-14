#include "zs.h"

/* Delete.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->type->del) sprite->type->del(sprite);
  free(sprite);
}

/* New.
 */
 
struct sprite *sprite_new(const struct sprite_type *type) {
  if (!type) return 0;
  struct sprite *sprite=calloc(type->objlen,1);
  if (!sprite) return 0;
  sprite->type=type;
  
  sprite->phl=-0.5;
  sprite->phr=0.5;
  sprite->pht=-0.5;
  sprite->phb=0.5;
  sprite->terminal_velocity=DEFAULT_TERMINAL_VELOCITY;
  sprite->solid=0;
  sprite->physics_mask=0;
  
  return sprite;
}

/* Read and apply those commands get can generically.
 */
 
static void sprite_read_generic_commands(struct sprite *sprite) {
  struct rom_command_reader reader={.v=sprite->cmdv,.c=sprite->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case CMD_sprite_tile: {
          sprite->tileid=cmd.argv[0];
          sprite->xform=cmd.argv[1];
        } break;
    }
  }
}

static const struct sprite_type *sprite_type_from_commands(const void *src,int srcc) {
  struct rom_command_reader reader={.v=src,.c=srcc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode!=CMD_sprite_sprctl) continue;
    return sprite_type_from_id((cmd.argv[0]<<8)|cmd.argv[1]);
  }
  return 0;
}

/* Finish spawning sprite:
 *  - Call (type->init) if it exists.
 *  - Add to global list.
 * If anything fails, we delete the sprite and return null.
 */
 
static struct sprite *sprite_spawn_finish(struct sprite *sprite) {
  if (!sprite) return 0;
  if (sprite->type->init&&(sprite->type->init(sprite)<0)) {
    sprite_del(sprite);
    return 0;
  }
  if (g.spritec>=g.spritea) {
    int na=g.spritea+128;
    if (na>INT_MAX/sizeof(void*)) { sprite_del(sprite); return 0; }
    void *nv=realloc(g.spritev,sizeof(void*)*na);
    if (!nv) { sprite_del(sprite); return 0; }
    g.spritev=nv;
    g.spritea=na;
  }
  g.spritev[g.spritec++]=sprite;
  return sprite;
}

/* Spawn, public.
 */

struct sprite *sprite_spawn_cmd(int col,int row,int rid,uint32_t arg) {
  return sprite_spawn_rid((double)col+0.5,(double)row+0.5,rid,arg);
}

struct sprite *sprite_spawn_rid(double x,double y,int rid,uint32_t arg) {
  const void *src=0;
  int srcc=zs_res_get(&src,EGG_TID_sprite,rid);
  struct rom_sprite rspr;
  if (rom_sprite_decode(&rspr,src,srcc)<0) {
    fprintf(stderr,"Failed to load sprite:%d.\n",rid);
    return 0;
  }
  const struct sprite_type *type=sprite_type_from_commands(rspr.cmdv,rspr.cmdc);
  if (!type) {
    fprintf(stderr,"sprite:%d doesn't name a sprite controller.\n",rid);
    return 0;
  }
  struct sprite *sprite=sprite_new(type);
  if (!sprite) return 0;
  sprite->rid=rid;
  sprite->arg=arg;
  sprite->x=sprite->pvx=x;
  sprite->y=sprite->pvy=y;
  sprite->cmdv=src;
  sprite->cmdc=srcc;
  sprite_read_generic_commands(sprite);
  return sprite_spawn_finish(sprite);
}

struct sprite *sprite_spawn_type(double x,double y,const struct sprite_type *type,uint32_t arg) {
  if (!type) return 0;
  struct sprite *sprite=sprite_new(type);
  if (!sprite) return 0;
  sprite->arg=arg;
  sprite->x=sprite->pvx=x;
  sprite->y=sprite->pvy=y;
  return sprite_spawn_finish(sprite);
}
  
/* Type by id.
 */
 
const struct sprite_type *sprite_type_from_id(int id) {
  switch (id) {
    #define _(tag) case NS_sprctl_##tag: return &sprite_type_##tag;
    SPRCTL_FOR_EACH
    #undef _
  }
  return 0;
}
