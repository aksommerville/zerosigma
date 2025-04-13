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
