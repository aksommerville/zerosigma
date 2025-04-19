/* zs_layer_dayend.c
 * Shows the day's score at the end of each.
 */
 
#include "zs.h"

#define LABEL_LIMIT 8
#define REVELATION_TIME 4.600 /* Try to match the music. */
#define BANNER_PERIOD 0.500
#define BANNER_DUTY_CYCLE 0.300

struct zs_layer_dayend {
  struct zs_layer hdr;
  struct label {
    int texid;
    int x,y,w,h;
  } labelv[LABEL_LIMIT];
  int labelc;
  double clock; // Counts up forever.
  int success;
  int customer; // 0..4. Knowable from (g.session.summaryc), but let this exclusively drive the choice of animation, so I can toggle for review.
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
  LAYER->clock+=elapsed;
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

/* suitor: Gives the bouquet to the lady, and she either loves it or slaps him.
 */
 
static void dayend_render_suitor(struct zs_layer *layer) {
  //TODO
}

/* spinster: Puts the bouquet in a vase, and if imbalanced, the vase breaks.
 */

static void dayend_render_spinster(struct zs_layer *layer) {
  //TODO
}

/* bride: Throws the bouquet to the maids, and if imbalanced, the devil catches it.
 */
 
static void dayend_render_bride(struct zs_layer *layer) {
  //TODO
}

/* dowager: Puts the bouquet on her late husband's grave.
 * If valid, his ghost pops up and gives a thumbs-up.
 * If invalid, there's an earthquake and we pan down to see him rolling in his grave.
 */
 
static void dayend_render_dowager(struct zs_layer *layer) {
  //TODO
}

/* parfumier: Drops the bouquet into his perfume machine, then tries to pour from the machine into a bottle.
 * If invalid, it spits fire!
 */
 
static void dayend_render_parfumier(struct zs_layer *layer) {
  //TODO
}

/* Render.
 */
 
static void _dayend_render(struct zs_layer *layer) {

  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x104020ff);
  
  switch (LAYER->customer) {
    case 0: dayend_render_suitor(layer); break;
    case 1: dayend_render_spinster(layer); break;
    case 2: dayend_render_bride(layer); break;
    case 3: dayend_render_dowager(layer); break;
    case 4: dayend_render_parfumier(layer); break;
  }
  
  struct label *label=LAYER->labelv;
  int i=LAYER->labelc;
  for (;i-->0;label++) graf_draw_decal(&g.graf,label->texid,label->x,label->y,0,0,label->w,label->h,0);
  
  if (LAYER->clock>=REVELATION_TIME) {
    double bannertime=fmod(LAYER->clock-REVELATION_TIME,BANNER_PERIOD);
    if (bannertime>BANNER_DUTY_CYCLE) graf_set_alpha(&g.graf,0x80);
      int srcx=1,srcy=1,w=154,h=43;
      if (LAYER->success) {
        srcy=45;
        w=121;
        h=21;
      }
      int dstx=(FBW>>1)-(w>>1);
      int dsty=(FBH/3)-(h>>1);
      graf_draw_decal(&g.graf,g.texid_uibits,dstx,dsty,srcx,srcy,w,h,0);
    graf_set_alpha(&g.graf,0xff);
  }
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
  } else if (!valid) {
    summary->rule=8;
  } else if (totalc>=45) {
    summary->rule=6;
  } else {
    summary->rule=7;
  }
  if (valid) {
    // Score for invalid bouquets is always zero.
    // If valid, it's (q*colorc), so 1..45, except if it's maxed out, bonus-bump to 100.
    if ((q==9)&&(colorc==5)) summary->score=100;
    else summary->score=q*colorc;
  }
}

/* Add labels.
 */
 
static struct label *dayend_add_label_text(struct zs_layer *layer,const char *src,int srcc) {
  if (LAYER->labelc>=LABEL_LIMIT) return 0;
  struct label *label=LAYER->labelv+LAYER->labelc++;
  label->texid=font_tex_oneline(g.font,src,srcc,FBW,0xffffffff);
  egg_texture_get_status(&label->w,&label->h,label->texid);
  return label;
}

static struct label *dayend_add_label_int(struct zs_layer *layer,int src) {
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

static struct label *dayend_add_label_string(struct zs_layer *layer,int rid,int index) {
  const char *src=0;
  int srcc=strings_get(&src,rid,index);
  if (srcc<0) srcc=0;
  return dayend_add_label_text(layer,src,srcc);
}

static struct label *dayend_add_label_ii(struct zs_layer *layer,int rid,int index,int a,int b) {
  char tmp[256];
  struct strings_insertion insv[]={
    {'i',{.i=a}},
    {'i',{.i=b}},
  };
  int tmpc=strings_format(tmp,sizeof(tmp),rid,index,insv,2);
  if ((tmpc<0)||(tmpc>sizeof(tmp))) tmpc=0;
  return dayend_add_label_text(layer,tmp,tmpc);
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
  
  // Arguably not our problem, but the creation of a dayend layer is a good signal that sprites should be dropped.
  // This must be done at some point before session_reset().
  sprites_drop();
  
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
  
  LAYER->customer=g.session.summaryc-1;
  if (summary->score) {
    egg_play_song(RID_song_flowers_for_you_positive,0,0);
    LAYER->success=1;
  } else {
    egg_play_song(RID_song_flowers_for_you_negative,0,0);
    LAYER->success=0;
  }
  
  //XXX TEMP place some labels. We'll replace this soon with juicy counters and an animated sequence showing the bouquet being accepted or rejected.
  struct label *label=dayend_add_label_ii(layer,1,11,g.session.summaryc,DAYC);
  if (!label) { layer->defunct=1; return 0; }
  label->x=1;
  label->y=1;
  
  return layer;
}
