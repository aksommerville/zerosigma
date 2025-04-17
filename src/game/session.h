/* session.h
 * Singleton. The globals live in (g.session).
 * Manages long-term state like scoring.
 * Comes into scope after the main menu and goes out of scope after game over.
 */
 
#ifndef SESSION_H
#define SESSION_H

/* TODO Once the maps are final-ish, determine how many flowers we actually need.
 * Should be considerably less than 1024.
 */
#define FLOWER_LIMIT 1024
#define BOUQUET_LIMIT 45 /* 5 colors, up to 9 apiece. */

#define COLORC 5
struct color {
  uint8_t r,g,b;
};
extern const struct color colorv[COLORC];
extern const uint32_t display_colorv[COLORC]; // Packed RGBA for use in general UI. Suitable for black foreground.

struct session {

  /* Flowers are sorted by (flowerid) and also by (mapid,y,x).
   */
  struct session_flower {
    uint16_t flowerid; // Unique within this set. Starts as (index+1) but will drift over time as flowers get removed.
    uint16_t mapid;
    uint8_t x,y;
    uint8_t tileid; // 0x40..0x42, the outline tile. Interior is +0x10.
    uint32_t tint;
    int colorid; // 0..COLORC-1. Could be calculated from (tint), but that's a little hairy and no need for it.
  } flowerv[FLOWER_LIMIT];
  int flowerc;
  
  struct session_flower bouquetv[BOUQUET_LIMIT];
  int bouquetc;
};

int session_reset();

int session_flowerp_by_flowerid(int flowerid);
int session_flowerp_by_mapid(int mapid); // => Position of first flower for this map, never OOB.
int session_flowerp_by_location(int mapid,int x,int y); // => Position or -1, not insertion  point.

#endif
