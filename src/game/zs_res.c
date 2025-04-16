#include "zs.h"

/* Receive tilesheet.
 * There should only be one, and its "physics" table gets stored globally.
 */
 
static int zs_load_tilesheet(const void *src,int srcc) {
  struct rom_tilesheet_reader reader;
  if (rom_tilesheet_reader_init(&reader,src,srcc)<0) return -1;
  struct rom_tilesheet_entry entry;
  while (rom_tilesheet_reader_next(&entry,&reader)>0) {
    if (entry.tableid!=NS_tilesheet_physics) continue;
    memcpy(g.physics+entry.tileid,entry.v,entry.c);
  }
  return 0;
}

/* Decode map resource and add to store.
 */
 
static int zs_map_add(const struct rom_res *res) {
  if (g.mapc) {
    const struct zs_map *last=g.mapv+g.mapc-1;
    if (res->rid<=last->rid) return -1;
  }
  if (g.mapc>=g.mapa) {
    int na=g.mapa+16;
    if (na>INT_MAX/sizeof(struct zs_map)) return -1;
    void *nv=realloc(g.mapv,sizeof(struct zs_map)*na);
    if (!nv) return -1;
    g.mapv=nv;
    g.mapa=na;
  }
  struct rom_map rmap;
  if (rom_map_decode(&rmap,res->v,res->c)<0) return -1;
  struct zs_map *map=g.mapv+g.mapc++;
  memset(map,0,sizeof(struct zs_map));
  map->rid=res->rid;
  map->w=rmap.w;
  map->h=rmap.h;
  map->refv=rmap.v;
  map->cmdv=rmap.cmdv;
  map->cmdc=rmap.cmdc;
  if (!(map->v=malloc(map->w*map->h))) return -1;
  memcpy(map->v,rmap.v,map->w*map->h);
  return 0;
}

/* Add resource to the store.
 * We'll receive them in order, that's intrinsic to the format, so just verify, no need to search.
 */
 
static int zs_res_add(const struct rom_res *src) {
  if (g.resc) {
    const struct rom_res *last=g.resv+g.resc-1;
    if (src->tid<last->tid) return -1;
    if ((src->tid==last->tid)&&(src->rid<=last->rid)) return -1;
  }
  if (g.resc>=g.resa) {
    int na=g.resa+32;
    if (na>INT_MAX/sizeof(struct rom_res)) return -1;
    void *nv=realloc(g.resv,sizeof(struct rom_res)*na);
    if (!nv) return -1;
    g.resv=nv;
    g.resa=na;
  }
  struct rom_res *dst=g.resv+g.resc++;
  memcpy(dst,src,sizeof(struct rom_res));
  return 0;
}

/* With all the maps loaded, identify doors and record them in both impacted maps.
 * Doors are stored just once, doesn't matter which side.
 */
 
static int zs_res_add_door(struct zs_map *a,const struct rom_command *cmd) {

  // Split command, acquire "b" map, confirm we have room for a new door in each.
  int ax=cmd->argv[0];
  int ay=cmd->argv[1];
  int brid=(cmd->argv[2]<<8)|cmd->argv[3];
  int bx=cmd->argv[4];
  int by=cmd->argv[5];
  int comment=(cmd->argv[6]<<8)|cmd->argv[7]; // not actually using yet, probly won't
  struct zs_map *b=zs_map_get(brid);
  if (!b) {
    fprintf(stderr,"map:%d not found, referenced by door in map:%d\n",brid,a->rid);
    return -1;
  }
  if (a->doorc>=ZS_DOOR_LIMIT) {
    fprintf(stderr,"Too many doors in map:%d, limit %d\n",a->rid,ZS_DOOR_LIMIT);
    return -1;
  }
  if (b->doorc>=ZS_DOOR_LIMIT) {
    fprintf(stderr,"Too many doors in map:%d, limit %d\n",brid,ZS_DOOR_LIMIT);
    return -1;
  }
  
  /* Position must lie on one edge, with the other axis in 1..c-2, ie not in a corner.
   * The remote position must lie on the opposite edge.
   * Initially set each door just 1 cell wide.
   */
  struct zs_door *adoor=a->doorv+a->doorc++;
  struct zs_door *bdoor=b->doorv+b->doorc++;
  adoor->mapid=brid;
  bdoor->mapid=a->rid;
  #define ASSERT_POSITION(n,lo,hi,rid,x,y) { \
    if (((n)<(lo))||((n)>(hi))) { \
      fprintf(stderr,"Invalid door position %d,%d in map:%d\n",x,y,rid); \
      return -1; \
    } \
  }
  if (ax==0) {
    ASSERT_POSITION(ay,1,a->h-2,a->rid,ax,ay)
    ASSERT_POSITION(bx,b->w-1,b->w-1,b->rid,bx,by)
    ASSERT_POSITION(by,1,b->h-2,b->rid,bx,by)
    adoor->edge=ZS_EDGE_W;
    bdoor->edge=ZS_EDGE_E;
    adoor->p=ay;
    adoor->c=1;
    adoor->d=by-ay;
    bdoor->p=by;
    bdoor->c=1;
    bdoor->d=ay-by;
  } else if (ax==a->w-1) {
    ASSERT_POSITION(ay,1,a->h-2,a->rid,ax,ay)
    ASSERT_POSITION(bx,0,0,b->rid,bx,by)
    ASSERT_POSITION(by,1,b->h-2,b->rid,bx,by)
    adoor->edge=ZS_EDGE_E;
    bdoor->edge=ZS_EDGE_W;
    adoor->p=ay;
    adoor->c=1;
    adoor->d=by-ay;
    bdoor->p=by;
    bdoor->c=1;
    bdoor->d=ay-by;
  } else if (ay==0) {
    ASSERT_POSITION(ax,1,a->w-2,a->rid,ax,ay)
    ASSERT_POSITION(by,b->h-1,b->h-1,b->rid,bx,by)
    ASSERT_POSITION(bx,1,b->w-2,b->rid,bx,by)
    adoor->edge=ZS_EDGE_N;
    bdoor->edge=ZS_EDGE_S;
    adoor->p=ax;
    adoor->c=1;
    adoor->d=bx-ax;
    bdoor->p=bx;
    bdoor->c=1;
    bdoor->d=ax-bx;
  } else if (ay==a->h-1) {
    ASSERT_POSITION(ax,1,a->w-2,a->rid,ax,ay)
    ASSERT_POSITION(by,0,0,b->rid,bx,by)
    ASSERT_POSITION(bx,1,b->w-2,b->rid,bx,by)
    adoor->edge=ZS_EDGE_S;
    bdoor->edge=ZS_EDGE_N;
    adoor->p=ax;
    adoor->c=1;
    adoor->d=bx-ax;
    bdoor->p=bx;
    bdoor->c=1;
    bdoor->d=ax-bx;
  } else {
    fprintf(stderr,"Invalid position %d,%d for door in map:%d\n",ax,ay,a->rid);
    return -1;
  }
  #undef ASSERT_POSITION
  
  /* Expand both sides of the door.
   * They must reach a solid tile at the same time on both sides.
   * This is easy to mess up when designing, careful checking here really does matter.
   * CHECK_CELL evaluates true if the door is finished, false to proceed, or returns if invalid.
   */
  #define CHECK_CELL(ax,ay,bx,by) ({ \
    int _ax=(ax),_ay=(ay),_bx=(bx),_by=(by),result; \
    if ( \
      (_ax<0)||(_ax>=a->w)|| \
      (_ay<0)||(_ay>=a->h)|| \
      (_bx<0)||(_bx>=b->w)|| \
      (_by<0)||(_by>=b->h) \
    ) { \
      fprintf(stderr,"Door between map:%d and map:%d extends past the edge.\n",a->rid,b->rid); \
      return -1; \
    } \
    uint8_t aphysics=g.physics[a->v[_ay*a->w+_ax]]; \
    uint8_t bphysics=g.physics[b->v[_by*b->w+_bx]]; \
    if ((aphysics==NS_physics_solid)&&(bphysics==NS_physics_solid)) result=1; \
    else result=0; \
    (result); \
  })
  if ((adoor->edge==ZS_EDGE_N)||(adoor->edge==ZS_EDGE_S)) { // expand horizontally
    for (;;) {
      if (CHECK_CELL(adoor->p-1,ay,bdoor->p-1,by)) break;
      adoor->p--;
      bdoor->p--;
      adoor->c++;
      bdoor->c++;
    }
    for (;;) {
      if (CHECK_CELL(adoor->p+adoor->c,ay,bdoor->p+bdoor->c,by)) break;
      adoor->c++;
      bdoor->c++;
    }
  } else { // expand vertically
    for (;;) {
      if (CHECK_CELL(ax,adoor->p-1,bx,bdoor->p-1)) break;
      adoor->p--;
      bdoor->p--;
      adoor->c++;
      bdoor->c++;
    }
    for (;;) {
      if (CHECK_CELL(ax,adoor->p+adoor->c,bx,bdoor->p+bdoor->c)) break;
      adoor->c++;
      bdoor->c++;
    }
  }
  #undef CHECK_CELL
  
  return 0;
}
 
static int zs_res_populate_doors() {
  struct zs_map *map=g.mapv;
  int mapi=g.mapc;
  for (;mapi-->0;map++) {
    struct rom_command_reader reader={.v=map->cmdv,.c=map->cmdc};
    struct rom_command cmd;
    while (rom_command_reader_next(&cmd,&reader)>0) {
      if (cmd.opcode!=CMD_map_door) continue;
      if (zs_res_add_door(map,&cmd)<0) return -1;
    }
  }
  return 0;
}

/* Initialize client-side resource store.
 */
 
int zs_res_init() {
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return -1;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    switch (res->tid) {
    
      // Some resources get indexed as is.
      case EGG_TID_sprite: {
          if (zs_res_add(res)<0) return -1;
        } break;
        
      // Maps get looked at a lot so they get their own store.
      case EGG_TID_map: if (zs_map_add(res)<0) return -1; break;
      
      // There should be only one tilesheet. Expand its "physics" table.
      case EGG_TID_tilesheet: {
          g.bgimageid=res->rid;
          if (zs_load_tilesheet(res->v,res->c)<0) return -1;
        } break;
      
      // Anything else, drop it.
    }
  }
  
  // Load the background and sprites textures. They stay loaded throughout.
  if (!g.texid_bg&&((g.texid_bg=egg_texture_new())<1)) return -1;
  if (egg_texture_load_image(g.texid_bg,g.bgimageid)<0) return -1;
  if (!g.texid_sprites&&((g.texid_sprites=egg_texture_new())<1)) return -1;
  if (egg_texture_load_image(g.texid_sprites,RID_image_sprites)<0) return -1;
  
  if (zs_res_populate_doors()<0) return -1;
  
  return 0;
}

/* Search TOC.
 */
 
static int zs_res_search(int tid,int rid) {
  int lo=0,hi=g.resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct rom_res *q=g.resv+ck;
         if (tid<q->tid) hi=ck;
    else if (tid>q->tid) lo=ck+1;
    else if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Get one resource.
 */
 
int zs_res_get(void *dstpp,int tid,int rid) {
  int p=zs_res_search(tid,rid);
  if (p<0) return 0;
  const struct rom_res *res=g.resv+p;
  *(const void**)dstpp=res->v;
  return res->c;
}

/* Get one map.
 */
 
struct zs_map *zs_map_get(int rid) {
  int lo=0,hi=g.mapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct zs_map *map=g.mapv+ck;
         if (rid<map->rid) hi=ck;
    else if (rid>map->rid) lo=ck+1;
    else return map;
  }
  return 0;
}
