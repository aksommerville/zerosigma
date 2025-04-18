/* zs_layer_gameover.c
 */
 
#include "zs.h"

#define LABEL_LIMIT 8

struct zs_layer_gameover {
  struct zs_layer hdr;
  struct label {
    int texid;
    int x,y,w,h;
  } labelv[LABEL_LIMIT];
  int labelc;
};

#define LAYER ((struct zs_layer_gameover*)layer)

/* Delete.
 */
 
static void _gameover_del(struct zs_layer *layer) {
  struct label *label=LAYER->labelv;
  int i=LAYER->labelc;
  for (;i-->0;label++) {
    egg_texture_del(label->texid);
  }
}

/* Update.
 */
 
static void _gameover_update(struct zs_layer *layer,double elapsed,int input,int pvinput) {
  //TODO
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    layer->defunct=1;
    session_reset();
    scene_reset();
  }
}

/* Render.
 */
 
static void _gameover_render(struct zs_layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x402020ff);
  struct label *label=LAYER->labelv;
  int i=LAYER->labelc;
  for (;i-->0;label++) graf_draw_decal(&g.graf,label->texid,label->x,label->y,0,0,label->w,label->h,0);
}

/* Add labels.
 */
 
static int gameover_add_label_text(struct zs_layer *layer,const char *src,int srcc) {
  if (LAYER->labelc>=LABEL_LIMIT) return -1;
  struct label *label=LAYER->labelv+LAYER->labelc++;
  label->texid=font_tex_oneline(g.font,src,srcc,FBW,0xffffffff);
  egg_texture_get_status(&label->w,&label->h,label->texid);
  return 0;
}

static int gameover_add_label_int(struct zs_layer *layer,int src) {
  char tmp[10];
  int tmpc=0;
  if (src>=1000000000) tmp[tmpc++]='0'+(src/1000000000)%10;
  if (src>= 100000000) tmp[tmpc++]='0'+(src/ 100000000)%10;
  if (src>=  10000000) tmp[tmpc++]='0'+(src/  10000000)%10;
  if (src>=   1000000) tmp[tmpc++]='0'+(src/   1000000)%10;
  if (src>=    100000) tmp[tmpc++]='0'+(src/    100000)%10;
  if (src>=     10000) tmp[tmpc++]='0'+(src/     10000)%10;
  if (src>=      1000) tmp[tmpc++]='0'+(src/      1000)%10;
  if (src>=       100) tmp[tmpc++]='0'+(src/       100)%10;
  if (src>=        10) tmp[tmpc++]='0'+(src/        10)%10;
  tmp[tmpc++]='0'+src%10;
  return gameover_add_label_text(layer,tmp,tmpc);
}

static int gameover_add_label_string(struct zs_layer *layer,int rid,int index) {
  const char *src=0;
  int srcc=strings_get(&src,rid,index);
  if (srcc<0) srcc=0;
  return gameover_add_label_text(layer,src,srcc);
}

static int gameover_add_label_time(struct zs_layer *layer,double sf) {
  int ms=(int)(sf*1000.0);
  int s=ms/1000; ms%=1000;
  int m=s/60; s%=60;
  if (m>99) { m=s=99; ms=999; } // A 100-minute session is pretty crazy, no need to accomodate that.
  char tmp[]={
    '0'+m/10,
    '0'+m%10,
    ':',
    '0'+s/10,
    '0'+s%10,
    '.',
    '0'+ms/100,
    '0'+(ms/10)%10,
    '0'+ms%10,
  };
  return gameover_add_label_text(layer,tmp,sizeof(tmp));
}

/* New.
 */
 
struct zs_layer *zs_layer_spawn_gameover() {
  struct zs_layer *layer=zs_layer_spawn(sizeof(struct zs_layer_gameover));
  if (!layer) return 0;
  
  layer->opaque=1;
  layer->wants_focus=1;
  layer->del=_gameover_del;
  layer->update=_gameover_update;
  layer->render=_gameover_render;
  
  //XXX TEMP place some labels. Not sure what I want for this UI ultimately. Showing the score is certainly part of it.
  gameover_add_label_text(layer,"GAME OVER",-1);
  int score=0,i;
  for (i=DAYC;i-->0;) score+=g.session.summaryv[i].score;
  gameover_add_label_int(layer,score);
  gameover_add_label_time(layer,g.session.playtime);
  int hsum=0;
  struct label *label=LAYER->labelv;
  for (i=LAYER->labelc;i-->0;label++) hsum+=label->h;
  int y=(FBH>>1)-(hsum>>1);
  for (label=LAYER->labelv,i=LAYER->labelc;i-->0;label++) {
    label->x=(FBW>>1)-(label->w>>1);
    label->y=y;
    y+=label->h;
  }
  
  egg_play_song(RID_song_cosmic_balance,0,1);
  
  return layer;
}
