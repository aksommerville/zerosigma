/* zs_layer_dayend.c
 * Shows the day's score at the end of each.
 */
 
#include "zs.h"

#define LABEL_LIMIT 8

struct zs_layer_dayend {
  struct zs_layer hdr;
  struct label {
    int texid;
    int x,y,w,h;
  } labelv[LABEL_LIMIT];
  int labelc;
};

#define LAYER ((struct zs_layer_dayend*)layer)

/* Delete.
 */
 
static void _dayend_del(struct zs_layer *layer) {
  struct label *label=LAYER->labelv;
  int i=LAYER->labelc;
  for (;i-->0;label++) {
    egg_texture_del(label->texid);
  }
}

/* Update.
 */
 
static void _dayend_update(struct zs_layer *layer,double elapsed,int input,int pvinput) {
  //TODO I want some animation, a unique one for each day, that shows the bouquet either doing its job or failing hilariously.
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    layer->defunct=1;
    if (g.session.summaryc>=DAYC) {
      zs_layer_spawn_gameover();
    } else {
      scene_reset();
    }
  }
}

/* Render.
 */
 
static void _dayend_render(struct zs_layer *layer) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x104020ff);
  struct label *label=LAYER->labelv;
  int i=LAYER->labelc;
  for (;i-->0;label++) graf_draw_decal(&g.graf,label->texid,label->x,label->y,0,0,label->w,label->h,0);
}

/* Calculate score for one bouquet, from and to its summary.
 */
 
static void score_summary(struct summary *summary) {
  summary->score=0;
  int totalc=0,colorc=0,i,q=0,valid=1;
  for (i=0;i<COLORC;i++) {
    if (!summary->flowers[i]) continue;
    if (!q) q=summary->flowers[i];
    else if (q!=summary->flowers[i]) valid=0;
    colorc++;
    totalc+=summary->flowers[i];
  }
  if (totalc<5) {
    summary->rule=10;
    valid=0;
  } else if (colorc<3) {
    summary->rule=9;
    valid=0;
  } else if (!valid) {
    summary->rule=8;
  } else if (totalc>=45) {
    summary->rule=6;
  } else {
    summary->rule=7;
  }
  if (valid) {
    /* Score for invalid bouquets is always zero.
     * If valid, it's driven by (q) and (colorc).
     * How about (q**colorc)? That yields a maximum of 59049 for the day, and 295245 for the session.
     * Also yields a minimum of 1 (five of the same color), so that's cool, it reaches both ends of the range.
     */
    summary->score=1;
    for (i=colorc;i-->0;) summary->score*=q;
  }
}

/* Add labels.
 */
 
static int dayend_add_label_text(struct zs_layer *layer,const char *src,int srcc) {
  if (LAYER->labelc>=LABEL_LIMIT) return -1;
  struct label *label=LAYER->labelv+LAYER->labelc++;
  label->texid=font_tex_oneline(g.font,src,srcc,FBW,0xffffffff);
  egg_texture_get_status(&label->w,&label->h,label->texid);
  return 0;
}

static int dayend_add_label_int(struct zs_layer *layer,int src) {
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
  return dayend_add_label_text(layer,tmp,tmpc);
}

static int dayend_add_label_string(struct zs_layer *layer,int rid,int index) {
  const char *src=0;
  int srcc=strings_get(&src,rid,index);
  if (srcc<0) srcc=0;
  return dayend_add_label_text(layer,src,srcc);
}

/* New.
 * We also do the business logic of scoring for day-end.
 */
 
struct zs_layer *zs_layer_spawn_dayend() {
  struct zs_layer *layer=zs_layer_spawn(sizeof(struct zs_layer_dayend));
  if (!layer) return 0;
  
  layer->opaque=1;
  layer->wants_focus=1;
  layer->del=_dayend_del;
  layer->update=_dayend_update;
  layer->render=_dayend_render;
  
  // Add blank summary to the session.
  if (g.session.summaryc>=DAYC) {
    layer->defunct=1;
    return 0;
  }
  struct summary *summary=g.session.summaryv+g.session.summaryc++;
  memset(summary,0,sizeof(struct summary));
  
  // Count flowers by color from the current bouquet, then drop the bouquet.
  struct session_flower *flower=g.session.bouquetv;
  int i=g.session.bouquetc;
  for (;i-->0;flower++) {
    summary->flowers[flower->colorid]++;
  }
  g.session.bouquetc=0;
  
  // Calculate the bouquet's score.
  score_summary(summary);
  
  if (summary->score) {
    egg_play_song(RID_song_flowers_for_you_positive,0,0);
  } else {
    egg_play_song(RID_song_flowers_for_you_negative,0,0);
  }
  
  //XXX TEMP place some labels. We'll replace this soon with juicy counters and an animated sequence showing the bouquet being accepted or rejected.
  dayend_add_label_text(layer,"END OF DAY",-1);
  dayend_add_label_int(layer,summary->score);
  dayend_add_label_string(layer,1,summary->rule);
  int hsum=0;
  struct label *label=LAYER->labelv;
  for (i=LAYER->labelc;i-->0;label++) hsum+=label->h;
  int y=(FBH>>1)-(hsum>>1);
  for (label=LAYER->labelv,i=LAYER->labelc;i-->0;label++) {
    label->x=(FBW>>1)-(label->w>>1);
    label->y=y;
    y+=label->h;
  }
  
  return layer;
}
