#include "zs.h"

struct zs_layer_play {
  struct zs_layer hdr;
  int pvinput; // Last state reported to scene.
};

#define LAYER ((struct zs_layer_play*)layer)

/* Cleanup.
 */
 
static void _play_del(struct zs_layer *layer) {
}

/* Update.
 */
 
static void _play_update(struct zs_layer *layer,double elapsed,int input,int pvinput) {
  if (input!=pvinput) {
    int bit=0x4000;
    for (;bit;bit>>=1) {
      int nx=input&bit,pv=pvinput&bit;
      if (nx&&!pv) scene_button_down(bit);
      else if (!nx&&pv) scene_button_up(bit);
    }
    LAYER->pvinput=input;
  }
  scene_update(elapsed);
}

/* Focus.
 */
 
static void _play_focus(struct zs_layer *layer,int focus) {
  if (focus) {
    egg_play_song(RID_song_petal_to_the_metal,0,1);
  } else {
    if (LAYER->pvinput) {
      int bit=0x4000;
      for (;bit;bit>>=1) {
        if (LAYER->pvinput&bit) scene_button_up(bit);
      }
      LAYER->pvinput=0;
    }
  }
}

/* Render.
 */
 
static void _play_render(struct zs_layer *layer) {
  scene_render();
}

/* New.
 */
 
struct zs_layer *zs_layer_spawn_play() {
  struct zs_layer *layer=zs_layer_spawn(sizeof(struct zs_layer_play));
  if (!layer) return 0;
  
  layer->opaque=1;
  layer->wants_focus=1;
  layer->del=_play_del;
  layer->update=_play_update;
  layer->focus=_play_focus;
  layer->render=_play_render;
  
  //TODO init
  
  if (scene_reset()<0) {
    layer->defunct=1;
    return 0;
  }
  
  return layer;
}
