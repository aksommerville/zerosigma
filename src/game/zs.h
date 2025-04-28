#ifndef ZS_H
#define ZS_H

#define FBW 320
#define FBH 180

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "opt/rom/rom.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"
#include "zs_layer.h"
#include "zs_res.h"
#include "sprite.h"
#include "session.h"
#include "scene.h"
#include "physics.h"
#include "hiscore.h"

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
  int pvinput;
  int enable_sound;
  int enable_music;
  int songid;
  
  // zs_res.c
  struct rom_res *resv;
  int resc,resa;
  struct zs_map *mapv;
  int mapc,mapa;
  uint8_t physics[256]; // From the main tilesheet. All terrain graphics must use just one resource.
  int bgimageid; // Ignore what's stored in the maps; they all must use the same image.
  int texid_bg;
  int texid_sprites;
  int texid_uibits;
  int texid_bouquet;
  uint32_t *bouquetbits; // 16x16xRGBA, contains image:sprites:0x25, with the (bouquetgrayv) pixels volatile
  uint8_t bouquetgrayv[BOUQUET_LIMIT]; // Positions in bouquetbits of gray pixels
  
  // zs_layer.c
  struct zs_layer **layerv;
  int layerc,layera;
  struct zs_layer *layer_focus; // private; use zs_layers_get_focus()
  
  // zs_sprite.c
  struct sprite **spritev;
  int spritec,spritea;
  
  struct session session;
  struct scene scene;
  struct hiscore hiscore;
  struct hiscore prevscore;
} g;

#define egg_play_sound(rid) ({ if (g.enable_sound) egg_play_sound(rid); })
void zs_toggle_sound();
void zs_toggle_music(); // Assumes (repeat) when toggling on. This should only be accessible from the Hello menu, whose music does repeat.
void zs_play_song(int rid,int repeat);

#define TILE_IS_FLOWER(tileid) ((((tileid)>=0x05)&&((tileid)<=0x08))||((tileid)==0xab))

/* Rewrites (g.texid_bouquet) based on (g.session.bouquetv), which should be finished.
 */
void refresh_bouquet_image();

#endif
