/* zs_res.h
 */
 
#ifndef ZS_RES_H
#define ZS_RES_H

struct zs_map {
  int rid;
  int w,h;
  const uint8_t *refv; // Cells from original resources. Read-only.
  uint8_t *v; // Volatile real cells.
  const uint8_t *cmdv;
  int cmdc;
};

int zs_res_init();
int zs_res_get(void *dstpp,int tid,int rid);
struct zs_map *zs_map_get(int rid);

#endif
