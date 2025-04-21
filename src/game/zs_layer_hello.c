/* zs_layer_hello.c
 */
 
#include "zs.h"

struct zs_layer_hello {
  struct zs_layer hdr;
  int texid_top;
  int topw,toph;
};

#define LAYER ((struct zs_layer_hello*)layer)

/* Delete.
 */
 
static void _hello_del(struct zs_layer *layer) {
  egg_texture_del(LAYER->texid_top);
}

/* Update.
 */
 
static void _hello_update(struct zs_layer *layer,double elapsed,int input,int pvinput) {
  //TODO
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    layer->defunct=1;
    session_reset();
    scene_reset();
    zs_layer_spawn_play();
  }
}

/* Render.
 */
 
static void _hello_render(struct zs_layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x401030ff);
  graf_draw_decal(&g.graf,LAYER->texid_top,1,FBH-LAYER->toph-1,0,0,LAYER->topw,LAYER->toph,0);
}

/* Generate the top scores texture.
 */
 
static int hello_repr_time(char *dst,int dsta,int ms) {
  if (dsta<9) return 9;
  if (ms<0) ms=0;
  int s=ms/1000; ms%=1000;
  int min=s/60; s%=60;
  if (min>99) { min=s=99; ms=999; }
  if (min>=10) dst[0]='0'+min/10; else dst[0]=' ';
  dst[1]='0'+min%10;
  dst[2]=':';
  dst[3]='0'+s/10;
  dst[4]='0'+s%10;
  dst[5]='.';
  dst[6]='0'+ms/100;
  dst[7]='0'+(ms/10)%10;
  dst[8]='0'+ms%10;
  return 9;
}

static int hello_repr_decuint(char *dst,int dsta,int src) {
  if (dsta<10) return 0;
  if (src<0) src=0;
  int dstc=0;
  if (src>=1000000000) dst[dstc++]='0'+(src/1000000000)%10;
  if (src>= 100000000) dst[dstc++]='0'+(src/ 100000000)%10;
  if (src>=  10000000) dst[dstc++]='0'+(src/  10000000)%10;
  if (src>=   1000000) dst[dstc++]='0'+(src/   1000000)%10;
  if (src>=    100000) dst[dstc++]='0'+(src/    100000)%10;
  if (src>=     10000) dst[dstc++]='0'+(src/     10000)%10;
  if (src>=      1000) dst[dstc++]='0'+(src/      1000)%10;
  if (src>=       100) dst[dstc++]='0'+(src/       100)%10;
  if (src>=        10) dst[dstc++]='0'+(src/        10)%10;
  dst[dstc++]='0'+src%10;
  return dstc;
}

static void hello_render_right(uint32_t *dst,int x,int y,const char *src,int srcc) {
  int w=font_measure_line(g.font,src,srcc);
  font_render_string(dst,FBW,FBH,FBW<<2,x-w,y,g.font,src,srcc,0xffffffff);
}
 
static void hello_generate_top(struct zs_layer *layer) {
  uint32_t *pixels=calloc(FBW*4,FBH);
  if (!pixels) return;
  
  int timew=font_measure_line(g.font,"WW:WW.WWW",9);
  int scorew=font_measure_line(g.font,"WWW",3);
  int margin=5;
  const char *lockedname=0;
  int lockednamec=strings_get(&lockedname,1,13);
  int lineh=font_get_line_height(g.font);
  
  int dsty=0,i=0,maxw=1;
  const struct top *top=g.session.topv;
  for (;i<TOP_COUNT;i++,top++) {
    int w=timew+margin+scorew+margin;
    if (top->time) {
      char tmp[16];
      int tmpc=hello_repr_time(tmp,sizeof(tmp),top->time);
      hello_render_right(pixels,timew,dsty,tmp,tmpc);
      tmpc=hello_repr_decuint(tmp,sizeof(tmp),top->score);
      hello_render_right(pixels,timew+margin+scorew,dsty,tmp,tmpc);
      const char *src=0;
      int srcc=strings_get(&src,1,14+i);
      w+=font_render_string(pixels,FBW,FBH,FBW<<2,w,dsty,g.font,src,srcc,0xffffffff);
    } else {
      hello_render_right(pixels,timew,dsty,"--:--.---",9);
      hello_render_right(pixels,timew+margin+scorew,dsty,"---",3);
      w+=font_render_string(pixels,FBW,FBH,FBW<<2,w,dsty,g.font,lockedname,lockednamec,0xffffffff);
    }
    if (w>maxw) maxw=w;
    dsty+=lineh;
  }
  
  LAYER->topw=maxw;
  LAYER->toph=dsty;
  egg_texture_load_raw(LAYER->texid_top,LAYER->topw,LAYER->toph,FBW<<2,pixels,FBW*FBH*4);
  free(pixels);
}

/* New.
 */
 
struct zs_layer *zs_layer_spawn_hello() {
  struct zs_layer *layer=zs_layer_spawn(sizeof(struct zs_layer_hello));
  if (!layer) return 0;
  
  layer->opaque=1;
  layer->wants_focus=1;
  layer->del=_hello_del;
  layer->update=_hello_update;
  layer->render=_hello_render;
  
  egg_play_song(RID_song_wild_flowers_none_can_tame,0,1);
  
  if ((LAYER->texid_top=egg_texture_new())<1) { layer->defunct=1; return 0; }
  hello_generate_top(layer);
  
  return layer;
}
