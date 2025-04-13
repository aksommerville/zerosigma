#include "zs.h"

struct zs_layer_play {
  struct zs_layer hdr;
};

#define LAYER ((struct zs_layer_play*)layer)

/* Cleanup.
 */
 
static void _play_del(struct zs_layer *layer) {
}

/* Update.
 */
 
static void _play_update(struct zs_layer *layer,double elapsed,int input,int pvinput) {
  //TODO Detect and deliver input events.
  scene_update(elapsed);
}

/* Focus.
 */
 
static void _play_focus(struct zs_layer *layer,int focus) {
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
