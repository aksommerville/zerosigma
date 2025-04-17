#include "zs.h"

/* Select tint for a flower.
 * They should be random, but also balanced overall.
 * Regular PRNG does meet that requirement, but only on long periods -- probably longer than we can tolerate.
 * Well... I dunno. Let's start dumb and see what happens.
 */
 
// Keep all channels in 0x10..0xf0: We randomly add or subtract up to 15 from each channel.
const struct color colorv[COLORC]={
  {0xf0,0x10,0x10}, // red
  {0x60,0x80,0xf0}, // blue
  {0xf0,0xa0,0xb0}, // pink
  {0xf0,0xf0,0x20}, // yellow
  {0xa0,0x20,0xf0}, // purple
};

const uint32_t display_colorv[COLORC]={
  0xf01010ff,
  0x6080f0ff,
  0xf0a0b0ff,
  0xf0f020ff,
  0xc060f0ff,
};
 
static uint32_t randomize_color(const struct color *color) {
  uint8_t r=color->r+((rand()&0x1f)-0x10);
  uint8_t g=color->g+((rand()&0x1f)-0x10);
  uint8_t b=color->b+((rand()&0x1f)-0x10);
  return (r<<24)|(g<<16)|(b<<8)|0xff;
}

/* Generate the set of flowers.
 */
 
static void session_reset_flowers() {
  g.session.flowerc=0;
  uint16_t flowerid=1;
  struct zs_map *map=g.mapv;
  int mapi=g.mapc;
  for (;mapi-->0;map++) {
    const uint8_t *v=map->v;
    int row=0; for (;row<map->h;row++) {
      int col=0; for (;col<map->w;col++,v++) {
        if ((*v<0x05)||(*v>0x08)) continue;
        if (g.session.flowerc>=FLOWER_LIMIT) {
          fprintf(stderr,"!!! Too many flowers (%d).\n",FLOWER_LIMIT);
          return;
        }
        struct session_flower *flower=g.session.flowerv+g.session.flowerc++;
        flower->flowerid=flowerid++;
        flower->mapid=map->rid;
        flower->x=col;
        flower->y=row;
        flower->tileid=0x40+rand()%3;
        flower->colorid=rand()%COLORC;
        flower->tint=randomize_color(colorv+flower->colorid);
      }
    }
  }
  fprintf(stderr,"%s: %d flowers in %d maps\n",__func__,g.session.flowerc,g.mapc);
}

/* Reset.
 */
 
int session_reset() {
  memset(&g.session,0,sizeof(struct session));
  session_reset_flowers();
  return 0;
}

/* Search flower list.
 */
 
int session_flowerp_by_flowerid(int flowerid) {
  if (flowerid<0) return -1;
  if (flowerid>0xffff) return -g.session.flowerc-1;
  int lo=0,hi=g.session.flowerc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct session_flower *flower=g.session.flowerv+ck;
         if (flowerid<flower->flowerid) hi=ck;
    else if (flowerid>flower->flowerid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int session_flowerp_by_mapid(int mapid) {
  if (mapid<0) return 0;
  if (mapid>0xffff) return g.session.flowerc;
  int lo=0,hi=g.session.flowerc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct session_flower *flower=g.session.flowerv+ck;
         if (mapid<flower->mapid) hi=ck;
    else if (mapid>flower->mapid) lo=ck+1;
    else {
      while ((ck>lo)&&(flower[-1].mapid==mapid)) { ck--; flower--; }
      return ck;
    }
  }
  return lo;
}

int session_flowerp_by_location(int mapid,int x,int y) {
  if ((mapid<0)||(mapid>0xffff)||(x<0)||(x>=0xff)||(y<0)||(y>0xff)) return -1;
  int lo=0,hi=g.session.flowerc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct session_flower *flower=g.session.flowerv+ck;
         if (mapid<flower->mapid) hi=ck;
    else if (mapid>flower->mapid) lo=ck+1;
    else if (y<flower->y) hi=ck;
    else if (y>flower->y) lo=ck+1;
    else if (x<flower->x) hi=ck;
    else if (x>flower->x) lo=ck+1;
    else return ck;
  }
  return -1;
}
