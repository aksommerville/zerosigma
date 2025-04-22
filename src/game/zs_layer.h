/* zs_layer.h
 * Top-level UI is arranged in layers.
 */
 
#ifndef ZS_LAYER_H
#define ZS_LAYER_H

struct zs_layer {
  int opaque; // Nonzero if layers behind me don't need to render.
  int defunct; // Nonzero to delete at the next opportunity. Avoid removing layers in real time.
  int wants_focus; // Nonzero to receive "update" events. Only the topmost such layer receives them.
  void (*del)(struct zs_layer *layer);
  void (*update)(struct zs_layer *layer,double elapsed,int input,int pvinput);
  void (*render)(struct zs_layer *layer);
  void (*focus)(struct zs_layer *layer,int focus);
};

void zs_layer_del(struct zs_layer *layer);
struct zs_layer *zs_layer_new(int len); // See typed ctors below.
struct zs_layer *zs_layer_spawn(int len);

void zs_layers_reap_defunct();
struct zs_layer *zs_layers_get_focus();

/* Create a new typed layer.
 * These all return weak on success.
 */
struct zs_layer *zs_layer_spawn_play();
struct zs_layer *zs_layer_spawn_dayend();
struct zs_layer *zs_layer_spawn_hello();

#endif
