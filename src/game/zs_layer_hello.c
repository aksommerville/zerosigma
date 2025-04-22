/* zs_layer_hello.c
 */
 
#include "zs.h"

#define MSG_LIMIT 5
#define MSG_TIME 5.000 /* >=MSG_FADE_IN_TIME+MSG_FADE_OUT_TIME */
#define MSG_FADE_IN_TIME 0.500
#define MSG_FADE_OUT_TIME 0.500

struct zs_layer_hello {
  struct zs_layer hdr;
  
  // Reports for the top scores and most recent session.
  // Either or both can be empty (eg both are empty the first time we launch).
  int texid_top;
  int topw,toph;
  int texid_prev;
  int prevw,prevh;
  
  int msgv[MSG_LIMIT];
  int msgc; // Constant for my life, but not the same every time!
  int msgp;
  double msgclock;
  int msg_texid;
  int msgw,msgh;
};

#define LAYER ((struct zs_layer_hello*)layer)

/* Delete.
 */
 
static void _hello_del(struct zs_layer *layer) {
  egg_texture_del(LAYER->texid_top);
  egg_texture_del(LAYER->texid_prev);
  egg_texture_del(LAYER->msg_texid);
}

/* Redraw msg bits. Caller sets (msgp) valid first.
 */
 
static void hello_redraw_msg(struct zs_layer *layer) {
  egg_texture_del(LAYER->msg_texid);
  LAYER->msg_texid=font_texres_oneline(g.font,1,LAYER->msgv[LAYER->msgp],FBW,0xa0c0a0ff);
  egg_texture_get_status(&LAYER->msgw,&LAYER->msgh,LAYER->msg_texid);
}

/* Update.
 */
 
static void _hello_update(struct zs_layer *layer,double elapsed,int input,int pvinput) {
  
  if ((LAYER->msgclock+=elapsed)>=MSG_TIME) {
    LAYER->msgclock=0.0;
    if (++(LAYER->msgp)>=LAYER->msgc) LAYER->msgp=0;
    hello_redraw_msg(layer);
  }
  
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
  graf_draw_decal(&g.graf,g.texid_uibits,0,20,0,240,FBW,44,0);
  
  int alpha=0xff;
  if (LAYER->msgclock<MSG_FADE_IN_TIME) alpha=(int)((LAYER->msgclock*255.0)/MSG_FADE_IN_TIME);
  else {
    double rem=MSG_TIME-LAYER->msgclock;
    if (rem<MSG_FADE_OUT_TIME) alpha=(int)((rem*255.0)/MSG_FADE_OUT_TIME);
  }
  if (alpha<0xff) graf_set_alpha(&g.graf,alpha);
  graf_draw_decal(&g.graf,LAYER->msg_texid,(FBW>>1)-(LAYER->msgw>>1),70,0,0,LAYER->msgw,LAYER->msgh,0);
  if (alpha<0xff) graf_set_alpha(&g.graf,0xff);
  
  graf_draw_decal(&g.graf,LAYER->texid_top,1,FBH-LAYER->toph-1,0,0,LAYER->topw,LAYER->toph,0);
  graf_draw_decal(&g.graf,LAYER->texid_prev,(FBW>>1)-(LAYER->prevw>>1),FBH-LAYER->prevh-1,0,0,LAYER->prevw,LAYER->prevh,0);
}

/* Helpers for rendering report bits.
 * Destination must be sized (FBW,FBH).
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

static int hello_repr_hiscore(char *dst,int dsta,int id,int v) {
  switch (id) {
    case HISCORE_TIME:
    case HISCORE_VALID_TIME:
    case HISCORE_PERFECT_TIME:
      return hello_repr_time(dst,dsta,v);
  }
  return hello_repr_decuint(dst,dsta,v);
}

static int hello_render_right(uint32_t *dst,int x,int y,const char *src,int srcc,uint32_t rgba) {
  int w=font_measure_line(g.font,src,srcc);
  return font_render_string(dst,FBW,FBH,FBW<<2,x-w,y,g.font,src,srcc,rgba);
}

static int hello_render_center(uint32_t *dst,int x,int y,const char *src,int srcc,uint32_t rgba) {
  int w=font_measure_line(g.font,src,srcc);
  return font_render_string(dst,FBW,FBH,FBW<<2,x-(w>>1),y,g.font,src,srcc,rgba);
}

/* Generate the top scores texture.
 */
 
static void hello_generate_top(struct zs_layer *layer) {

  if (!g.hiscore.validscores) return;

  uint32_t *pixels=calloc(FBW*4,FBH);
  if (!pixels) return;
  
  int leftw=font_measure_line(g.font,"WW:WW.WWW",9);
  int margin=5;
  int lineh=font_get_line_height(g.font);
  
  int dsty=lineh,i=0,maxw=1;
  uint32_t bit=1;
  const int *v=g.hiscore.v;
  for (;i<HISCORE_COUNT;i++,v++,bit<<=1) {
    int w=leftw+margin;
    if (!(g.hiscore.validscores&bit)) {
      w+=font_render_string(pixels,FBW,FBH,FBW<<2,w,dsty,g.font,"???",3,0x808080ff);
    } else {
      uint32_t color=(g.hiscore.newscores&bit)?0xffff00ff:hiscore_is_perfect(i,*v)?0xffc060ff:0xa0c0f0ff;
      char tmp[16];
      int tmpc=hello_repr_hiscore(tmp,sizeof(tmp),i,*v);
      if ((tmpc>0)&&(tmpc<=sizeof(tmp))) {
        hello_render_right(pixels,leftw,dsty,tmp,tmpc,color);
      }
      const char *src=0;
      int srcc=strings_get(&src,1,15+i);
      w+=font_render_string(pixels,FBW,FBH,FBW<<2,w,dsty,g.font,src,srcc,color);
    }
    if (w>maxw) maxw=w;
    dsty+=lineh;
  }
  // Add "High Scores" at the top, now that we know the full width.
  const char *src=0;
  int srcc=strings_get(&src,1,13);
  hello_render_center(pixels,maxw>>1,0,src,srcc,0xffffffff);
  
  LAYER->topw=maxw;
  LAYER->toph=dsty;
  egg_texture_load_raw(LAYER->texid_top,LAYER->topw,LAYER->toph,FBW<<2,pixels,FBW*FBH*4);
  free(pixels);
}

/* Generate the recent score texture.
 */
 
static void hello_generate_prev(struct zs_layer *layer) {

  // The first time we run, there will never be anything to show here. Get out fast if so.
  if (!g.prevscore.validscores) return;

  uint32_t *pixels=calloc(FBW*4,FBH);
  if (!pixels) return;
  int lineh=font_get_line_height(g.font);
  
  // Draw everything line by line centered in the framebuffer -- we'll crop both left and right at upload.
  int maxw=1,dsty=0,srcc,tmpc,w;
  char tmp[16];
  const char *src;
  
  srcc=strings_get(&src,1,14);
  w=hello_render_center(pixels,FBW>>1,dsty,src,srcc,0xffffffff);
  if (w>maxw) maxw=w;
  dsty+=lineh;
  
  tmpc=hello_repr_time(tmp,sizeof(tmp),g.prevscore.v[HISCORE_TIME]);
  w=hello_render_center(pixels,FBW>>1,dsty,tmp,tmpc,0xa0c0f0ff);
  if (w>maxw) maxw=w;
  dsty+=lineh;
  
  tmpc=hello_repr_decuint(tmp,sizeof(tmp),g.prevscore.v[HISCORE_SCORE]);
  w=hello_render_center(pixels,FBW>>1,dsty,tmp,tmpc,0xa0c0f0ff);
  if (w>maxw) maxw=w;
  dsty+=lineh;
  
  if (g.prevscore.validscores&(1<<HISCORE_HERMIT_SCORE)) {
    srcc=strings_get(&src,1,15+HISCORE_HERMIT_SCORE);
    w=hello_render_center(pixels,FBW>>1,dsty,src,srcc,0xffff00ff);
    if (w>maxw) maxw=w;
    dsty+=lineh;
  }
  
  if (g.prevscore.validscores&(1<<HISCORE_MISER_SCORE)) {
    srcc=strings_get(&src,1,15+HISCORE_MISER_SCORE);
    w=hello_render_center(pixels,FBW>>1,dsty,src,srcc,0xffff00ff);
    if (w>maxw) maxw=w;
    dsty+=lineh;
  }
  
  if (maxw>FBW) maxw=FBW;
  LAYER->prevw=maxw;
  LAYER->prevh=dsty;
  egg_texture_load_raw(LAYER->texid_prev,LAYER->prevw,LAYER->prevh,FBW<<2,pixels+(FBW>>1)-(LAYER->prevw>>1),FBW*FBH*4);
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
  if ((LAYER->texid_prev=egg_texture_new())<1) { layer->defunct=1; return 0; }
  hello_generate_top(layer);
  hello_generate_prev(layer);
  
  LAYER->msgv[LAYER->msgc++]=21;
  LAYER->msgv[LAYER->msgc++]=22;
  LAYER->msgv[LAYER->msgc++]=23;
  // Two extra messages if you've got a perfect 500, but haven't got the Miser or Hermit bonuses.
  // Only show one of them, even if both qualify.
  if (g.hiscore.v[HISCORE_SCORE]>=500) {
    if (!(g.hiscore.validscores&(1<<HISCORE_HERMIT_SCORE))) LAYER->msgv[LAYER->msgc++]=24;
    else if (!(g.hiscore.validscores&(1<<HISCORE_MISER_SCORE))) LAYER->msgv[LAYER->msgc++]=25;
  }
  hello_redraw_msg(layer);
  
  return layer;
}
