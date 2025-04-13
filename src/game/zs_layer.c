#include "zs.h"

/* Delete layer.
 */
 
void zs_layer_del(struct zs_layer *layer) {
  if (!layer) return;
  if (layer->del) layer->del(layer);
  free(layer);
}

/* New layer.
 */
 
struct zs_layer *zs_layer_new(int len) {
  if (len<(int)sizeof(struct zs_layer)) return 0;
  struct zs_layer *layer=calloc(1,len);
  if (!layer) return 0;
  return layer;
}

struct zs_layer *zs_layer_spawn(int len) {
  if (g.layerc>=g.layera) {
    int na=g.layera+8;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(g.layerv,sizeof(void*)*na);
    if (!nv) return 0;
    g.layerv=nv;
    g.layera=na;
  }
  struct zs_layer *layer=zs_layer_new(len);
  if (!layer) return 0;
  g.layerv[g.layerc++]=layer;
  return layer;
}

/* Test residency.
 */
 
static int zs_layer_is_resident(const struct zs_layer *layer) {
  int i=g.layerc;
  while (i-->0) {
    if (g.layerv[i]==layer) return 1;
  }
  return 0;
}

/* Reap defunct layers.
 */

void zs_layers_reap_defunct() {
  int i=g.layerc;
  while (i-->0) {
    struct zs_layer *layer=g.layerv[i];
    if (!layer->defunct) continue;
    g.layerc--;
    memmove(g.layerv+i,g.layerv+i+1,sizeof(void*)*(g.layerc-i));
    zs_layer_del(layer);
  }
}

/* Get focussed layer.
 */
 
struct zs_layer *zs_layers_get_focus() {
  int i=g.layerc;
  while (i-->0) {
    struct zs_layer *layer=g.layerv[i];
    if (layer->defunct) continue;
    if (!layer->wants_focus) continue;
    if (layer!=g.layer_focus) {
      if (zs_layer_is_resident(g.layer_focus)&&g.layer_focus->focus) {
        g.layer_focus->focus(g.layer_focus,0);
      }
      g.layer_focus=layer;
      if (layer->focus) {
        layer->focus(layer,1);
      }
    }
    return layer;
  }
  if (g.layer_focus) {
    if (zs_layer_is_resident(g.layer_focus)&&g.layer_focus->focus) {
      g.layer_focus->focus(g.layer_focus,0);
    }
    g.layer_focus=0;
  }
  return 0;
}
