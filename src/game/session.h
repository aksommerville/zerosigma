/* session.h
 * Singleton. The globals live in (g.session).
 * Manages long-term state like scoring.
 * Comes into scope after the main menu and goes out of scope after game over.
 */
 
#ifndef SESSION_H
#define SESSION_H

/* 5 colors, 5 days, and up to nine per color per day.
 * A single bouquet can't have more than 45 flowers.
 * The world should have exactly 225 flowers, exactly 45 of each color.
 * So it is only just barely possible to exhaust the world's flowers, and never possible to need another one.
 */
#define FLOWER_LIMIT 225
#define BOUQUET_LIMIT 45

#define DAYC 5

#define COLORC 5
struct color {
  uint8_t r,g,b;
};
extern const struct color colorv[COLORC];
extern const uint32_t display_colorv[COLORC]; // Packed RGBA for use in general UI. Suitable for black foreground.

struct session {

  double playtime; // s. Doesn't count time in menus.

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
  
  struct summary {
    int flowers[COLORC]; // Count by color.
    int score;
    int rule; // Index in strings:1 (6..10). Only 6 and 7 are valid, and will be accompanied by a nonzero score.
  } summaryv[DAYC];
  int summaryc;
  
  uint8_t flagv[NS_flag_COUNT];
  
  struct flag_listener {
    int listenerid;
    int flagid;
    void (*cb)(int flagid,int v,void *userdata);
    void *userdata;
  } *flag_listenerv;
  int flag_listenerc,flag_listenera;
  int flag_listener_next;
  int broadcast_in_progress;
};

int session_reset();

int session_flowerp_by_flowerid(int flowerid);
int session_flowerp_by_mapid(int mapid); // => Position of first flower for this map, never OOB.
int session_flowerp_by_location(int mapid,int x,int y); // => Position or -1, not insertion  point.

/* Flags zero and one never change.
 * You can listen on a negative flagid to receive all changes.
 * You must clean up any listener you create.
 * It is an error to add a new listener during a callback.
 */
int session_get_flag(int flagid);
void session_set_flag(int flagid,int v);
int session_listen_flag(int flagid,void (*cb)(int flagid,int v,void *userdata),void *userdata); // => listenerid
void session_unlisten_flag(int listenerid);

#endif
