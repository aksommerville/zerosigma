/* zs_res.h
 */
 
#ifndef ZS_RES_H
#define ZS_RES_H

#define ZS_DOOR_LIMIT 8
#define ZS_EDGE_N 1
#define ZS_EDGE_W 2
#define ZS_EDGE_E 3
#define ZS_EDGE_S 4

struct zs_map {
  int rid;
  int w,h;
  const uint8_t *refv; // Cells from original resources. Read-only.
  uint8_t *v; // Volatile real cells.
  const uint8_t *cmdv;
  int cmdc;
  struct zs_door {
    int mapid;
    int edge; // ZS_EDGE_*
    int p,c; // Position and extent along my edge.
    int d; // Offset to destination map coords.
  } doorv[ZS_DOOR_LIMIT];
  int doorc;
};

int zs_res_init();
int zs_res_get(void *dstpp,int tid,int rid);
struct zs_map *zs_map_get(int rid);

#endif
