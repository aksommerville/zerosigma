#include "zs.h"

struct g g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {
  
  int fbw=0,fbh=0;
  egg_texture_get_status(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer size disagreement. header=%d,%d metadata=%d,%d\n",FBW,FBH,fbw,fbh);
    return -1;
  }
  
  if ((g.romc=egg_get_rom(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_get_rom(g.rom,g.romc)!=g.romc) return -1;
  strings_set_rom(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image_resource(g.font,0x0020,RID_image_font9_0020)<0) return -1;
  
  if (zs_res_init()<0) {
    fprintf(stderr,"Error loading resources.\n");
    return -1;
  }
  
  srand_auto();
  
  if (!zs_layer_spawn_hello()) return -1;
  
  return 0;
}

void egg_client_update(double elapsed) {
  int input=egg_input_get_one(0);
  int pvinput=g.pvinput;
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) egg_terminate(0);
    g.pvinput=input;
  }
  struct zs_layer *layer=zs_layers_get_focus();
  if (!layer) {
    egg_terminate(1);
    return;
  }
  if (layer->update) layer->update(layer,elapsed,input,pvinput);
  zs_layers_reap_defunct();
}

void egg_client_render() {
  graf_reset(&g.graf);
  
  // Find the last opaque layer.
  int opaquep=-1,i;
  for (i=g.layerc;i-->0;) {
    struct zs_layer *layer=g.layerv[i];
    if (layer->defunct) continue;
    if (layer->opaque) {
      opaquep=i;
      break;
    }
  }
  
  // If we don't have an opaque layer, fill with black. This shouldn't happen.
  if (opaquep<0) {
    graf_draw_rect(&g.graf,0,0,FBW,FBH,0x000000ff);
    opaquep=0;
  }
  
  // Render all layers from (opaquep) up, in order.
  for (i=opaquep;i<g.layerc;i++) {
    struct zs_layer *layer=g.layerv[i];
    if (layer->defunct) continue;
    if (!layer->render) continue;
    layer->render(layer);
  }
  
  graf_flush(&g.graf);
}
