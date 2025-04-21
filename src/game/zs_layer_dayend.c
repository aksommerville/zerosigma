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
    int after_reveal;
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
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    if (LAYER->clock<REVELATION_TIME) {
      LAYER->clock=REVELATION_TIME+0.001;
    } else {
      layer->defunct=1;
      if (g.session.summaryc>=DAYC) {
        session_top_commit();
        zs_layer_spawn_hello();
      } else {
        scene_reset();
      }
    }
  }
}

/* suitor: Gives the bouquet to the lady, and she either loves it or slaps him.
 */
 
static void dayend_render_suitor(struct zs_layer *layer) {
  const int startx=0;
  const int endx=(FBW>>1)-NS_sys_tilesize-3;
  const double endtime=REVELATION_TIME-1.000;
  int manx,many=(FBH*3)>>2;
  int mansrcx,mansrcy=NS_sys_tilesize*9;
  if (LAYER->clock>=endtime) {
    manx=endx;
    if (LAYER->success||(LAYER->clock<REVELATION_TIME)) mansrcx=0;
    else mansrcx=NS_sys_tilesize*3;
  } else {
    manx=startx+(int)(((endx-startx)*LAYER->clock)/endtime);
    int frame=((int)(LAYER->clock*5.0))&3;
    switch (frame) {
      case 0: case 2: mansrcx=0; break;
      case 1: mansrcx=NS_sys_tilesize; break;
      case 3: mansrcx=NS_sys_tilesize*2; break;
    }
  }
  graf_draw_decal(&g.graf,g.texid_bouquet,manx+(NS_sys_tilesize>>1)+3,many+4,0,0,NS_sys_tilesize,NS_sys_tilesize,0);
  graf_draw_tile(&g.graf,g.texid_sprites,manx+(NS_sys_tilesize),many+NS_sys_tilesize,0x94,0);
  graf_draw_decal(&g.graf,g.texid_sprites,manx,many,mansrcx,mansrcy,NS_sys_tilesize,NS_sys_tilesize<<1,0);

  int ladyx=FBW>>1;
  int ladyy=many;
  int ladysrcx=NS_sys_tilesize*5;
  int ladysrcy=NS_sys_tilesize*9;
  if (LAYER->clock>=REVELATION_TIME) {
    if (LAYER->success) {
      graf_draw_decal(&g.graf,g.texid_sprites,ladyx,ladyy-NS_sys_tilesize,NS_sys_tilesize*7,NS_sys_tilesize*9,NS_sys_tilesize,NS_sys_tilesize,0); // heart
    } else {
      ladysrcx+=NS_sys_tilesize;
      graf_draw_decal(&g.graf,g.texid_sprites,ladyx-8,ladyy+5,NS_sys_tilesize*4,NS_sys_tilesize*10,NS_sys_tilesize,NS_sys_tilesize,0); // slappy hand
      graf_draw_decal(&g.graf,g.texid_sprites,manx,many-NS_sys_tilesize,NS_sys_tilesize*7,NS_sys_tilesize*10,NS_sys_tilesize,NS_sys_tilesize,0); // pain star
    }
  }
  graf_draw_decal(&g.graf,g.texid_sprites,ladyx,ladyy,ladysrcx,ladysrcy,NS_sys_tilesize,NS_sys_tilesize<<1,0);
}

/* spinster: Puts the bouquet in a vase, and if imbalanced, the vase breaks.
 */

static void dayend_render_spinster(struct zs_layer *layer) {
  int y=((FBH*3)>>2)-NS_sys_tilesize;
  int tablesrcx=NS_sys_tilesize*8;
  int ladysrcx=NS_sys_tilesize*2;
  int ladysrcy=NS_sys_tilesize*7;
  int endy=y;
  int starty=endy-NS_sys_tilesize;
  int bouquety=endy;
  int bouquetx=(FBW>>1)+(NS_sys_tilesize>>1);
  uint8_t bouquetxform=0;
  if (LAYER->clock<REVELATION_TIME) {
    bouquety=starty+(int)(((endy-starty)*LAYER->clock)/REVELATION_TIME);
  } else if (!LAYER->success) {
    bouquetxform=EGG_XFORM_SWAP|EGG_XFORM_YREV;
    bouquetx+=NS_sys_tilesize/3;
    bouquety+=NS_sys_tilesize/2;
  }
  graf_draw_decal(&g.graf,g.texid_bouquet,bouquetx,bouquety,0,0,NS_sys_tilesize,NS_sys_tilesize,bouquetxform);
  uint8_t ladyxform=EGG_XFORM_XREV;
  if ((LAYER->clock>=REVELATION_TIME)&&!LAYER->success) {
    tablesrcx+=NS_sys_tilesize*2;
    ladysrcx=NS_sys_tilesize*12;
    ladysrcy=NS_sys_tilesize*9;
    ladyxform=0;
  }
  graf_draw_decal(&g.graf,g.texid_sprites,(FBW>>1)-NS_sys_tilesize,y,ladysrcx,ladysrcy,NS_sys_tilesize,NS_sys_tilesize<<1,ladyxform);
  graf_draw_decal(&g.graf,g.texid_sprites,(FBW>>1),y,tablesrcx,NS_sys_tilesize*9,NS_sys_tilesize<<1,NS_sys_tilesize<<1,0);
}

/* bride: Throws the bouquet to the maids, and if imbalanced, the devil catches it.
 */
 
static void dayend_render_bride(struct zs_layer *layer) {
  const double THROW_TIME=2.000;
  const double RELAX_TIME=3.000;

  // Bride stands on the right facing away, and throws her bouquet.
  int bridex=(FBW*2)/3-NS_sys_tilesize;
  int bridey=((FBH*3)/4);
  int bridesrcx=NS_sys_tilesize*4;
  int bridesrcy=NS_sys_tilesize*7;
  uint8_t bridexform=EGG_XFORM_XREV;
  if (LAYER->clock>RELAX_TIME) { // After revelation, just the inert face. (actually, start this a bit before revelation)
    if (LAYER->clock>REVELATION_TIME) bridexform=0;
    graf_draw_decal(&g.graf,g.texid_sprites,bridex,bridey,bridesrcx,bridesrcy,NS_sys_tilesize,NS_sys_tilesize*2,bridexform);
  } else if (LAYER->clock>THROW_TIME) { // Use the speech face when the bouquet is in the air.
    bridesrcx+=NS_sys_tilesize;
    graf_draw_decal(&g.graf,g.texid_sprites,bridex,bridey,bridesrcx,bridesrcy,NS_sys_tilesize,NS_sys_tilesize*2,bridexform);
  } else { // Before the throw, bride is holding the bouquet.
    int srcx=NS_sys_tilesize*13;
    int srcy=NS_sys_tilesize*9;
    int armx=bridex+6;
    int army=bridey+7;
    graf_draw_decal(&g.graf,g.texid_sprites,armx,army,srcx,srcy,NS_sys_tilesize,NS_sys_tilesize,0);
    graf_draw_decal(&g.graf,g.texid_bouquet,armx+5,army-4,0,0,NS_sys_tilesize,NS_sys_tilesize,0);
    graf_draw_decal(&g.graf,g.texid_sprites,bridex,bridey,bridesrcx,bridesrcy,NS_sys_tilesize,NS_sys_tilesize*2,bridexform);
  }
  
  // Bridesmaids stand in a giggling gaggle on the left.
  // After revealed success, the second bridesmaid caught it.
  int bmx=FBW/3;
  int caught=(LAYER->success&&(LAYER->clock>REVELATION_TIME));
  int i=0; for (;i<4;i++,bmx+=13) {
    int giggle=0;
    if ((LAYER->clock>REVELATION_TIME)&&LAYER->success) giggle=((int)(LAYER->clock*10.0+i))&1; // resume giggling after blue girl catches it.
    else if (LAYER->clock<RELAX_TIME) giggle=((int)(LAYER->clock*10.0+i))&1; // giggle in the beginning, then when it's in flight, concentrate.
    if (caught) {
      if (i==1) {
        graf_draw_decal(&g.graf,g.texid_bouquet,bmx+8,bridey+8-giggle,0,0,NS_sys_tilesize,NS_sys_tilesize,0);
        graf_draw_decal(&g.graf,g.texid_sprites,bmx,bridey-giggle,NS_sys_tilesize*4,NS_sys_tilesize*11,NS_sys_tilesize,NS_sys_tilesize*2,0);
      } else if (i>1) {
        graf_draw_decal(&g.graf,g.texid_sprites,bmx,bridey-giggle,NS_sys_tilesize*i,NS_sys_tilesize*11,NS_sys_tilesize,NS_sys_tilesize*2,EGG_XFORM_XREV);
      } else {
        graf_draw_decal(&g.graf,g.texid_sprites,bmx,bridey-giggle,NS_sys_tilesize*i,NS_sys_tilesize*11,NS_sys_tilesize,NS_sys_tilesize*2,0);
      }
    } else {
      graf_draw_decal(&g.graf,g.texid_sprites,bmx,bridey-giggle,NS_sys_tilesize*i,NS_sys_tilesize*11,NS_sys_tilesize,NS_sys_tilesize*2,0);
    }
  }
  
  // Bouquet travels in an arc from the bride to the middle of the bridesmaids.
  // When in hand, it's already rendered.
  if ((LAYER->clock>THROW_TIME)&&(LAYER->clock<REVELATION_TIME)) {
    double xa=(double)(bridex+NS_sys_tilesize);
    double xz=(double)(FBW/3+NS_sys_tilesize);
    double t=(LAYER->clock-THROW_TIME)/(REVELATION_TIME-THROW_TIME);
    double normel=(t-0.5)*2.0; normel=1.0-(normel*normel);
    int x=(int)(xa*(1.0-t)+xz*t);
    int y=bridey-(int)(normel*60.0);
    graf_draw_decal(&g.graf,g.texid_bouquet,x,y,0,0,NS_sys_tilesize,NS_sys_tilesize,0);
  }
  
  // Failure revelation: The Devil catches it!
  if (!LAYER->success&&(LAYER->clock>REVELATION_TIME)) {
    int dstx=FBW/3+(NS_sys_tilesize>>1);
    int dsty=bridey;
    int srcx=NS_sys_tilesize*5;
    int srcy=NS_sys_tilesize*11;
    graf_draw_decal(&g.graf,g.texid_bouquet,dstx+10,dsty-6,0,0,NS_sys_tilesize,NS_sys_tilesize,EGG_XFORM_SWAP);
    graf_draw_decal(&g.graf,g.texid_sprites,dstx,dsty,srcx,srcy,NS_sys_tilesize*2,NS_sys_tilesize*2,0);
  }
}

/* widow: Puts the bouquet on her late husband's grave.
 * If valid, his ghost pops up and gives a thumbs-up.
 * If invalid, there's an earthquake and we pan down to see him rolling in his grave.
 */
 
static void dayend_render_dowager(struct zs_layer *layer) {
  int srcx,srcy,dstx,dsty;
  
  // Failure, we scroll the whole scene up to show mister rolling in his grave.
  int basey=FBH-41;
  if (!LAYER->success&&(LAYER->clock>REVELATION_TIME)) {
    const int scrollmax=72-41;
    int scroll=(int)(((LAYER->clock-REVELATION_TIME)*scrollmax)/1.500);
    if (scroll>scrollmax) scroll=scrollmax;
    basey-=scroll;
  }
  graf_draw_decal(&g.graf,g.texid_uibits,0,basey,0,67,FBW,72,0);
  
  // Widow is stationary with respect to the scene. Get her position, but don't render yet.
  int widowx=(FBW>>1)-30;
  int widowy=basey-10;
  
  // Bouquet travels from widow's hand and lands on the grave well before revelation, allow a beat with it still.
  const double RELEASE_TIME=2.000;
  const double TRAVEL_TIME=3.000;
  if (LAYER->clock<RELEASE_TIME) {
    int x=widowx+NS_sys_tilesize-(NS_sys_tilesize>>1)-2;
    int y=widowy+(NS_sys_tilesize>>1);
    graf_draw_decal(&g.graf,g.texid_bouquet,x,y,0,0,NS_sys_tilesize,NS_sys_tilesize,0);
  } else {
    double t=LAYER->clock/TRAVEL_TIME;
    if (t>=1.0) t=1.0;
    double xa=(double)(widowx-8);
    double xz=(double)(widowx+NS_sys_tilesize*2);
    int x=(int)(xa*(1.0-t)+xz*t);
    double ya=(double)(widowy+(NS_sys_tilesize>>1));
    double yz=(double)(widowy+NS_sys_tilesize+6);
    int y=(int)(ya*(1.0-t)+yz*t);
    graf_draw_decal(&g.graf,g.texid_bouquet,x,y,0,0,NS_sys_tilesize,NS_sys_tilesize,EGG_XFORM_SWAP|EGG_XFORM_YREV);
  }
  
  // Render widow, in front of the bouquet.
  srcx=NS_sys_tilesize*6;
  srcy=NS_sys_tilesize*7;
  graf_draw_decal(&g.graf,g.texid_sprites,widowx,widowy,srcx,srcy,NS_sys_tilesize,NS_sys_tilesize*2,EGG_XFORM_XREV);
  
  // Happy ghost on top, after success revelation.
  if (LAYER->success&&(LAYER->clock>REVELATION_TIME)) {
    srcx=156;
    srcy=1;
    dstx=widowx+NS_sys_tilesize*2;
    dsty=basey-NS_sys_tilesize;
    graf_draw_decal(&g.graf,g.texid_uibits,dstx,dsty,srcx,srcy,26,43,0);
  }
  
  // Unhappy skeleton on bottom, after failure revelation.
  if (!LAYER->success&&(LAYER->clock>REVELATION_TIME)) {
    const int w=53,h=19;
    int frame=((int)(LAYER->clock*10.0))&7;
    uint8_t xform=0;
    switch (frame) {
      case 5: frame=3; xform=EGG_XFORM_YREV; break;
      case 6: frame=2; xform=EGG_XFORM_YREV; break;
      case 7: frame=1; xform=EGG_XFORM_YREV; break;
    }
    dstx=129;
    dsty=basey+47;
    srcx=1;
    srcy=140+frame*(h+1);
    graf_draw_decal(&g.graf,g.texid_uibits,dstx,dsty,srcx,srcy,w,h,xform);
  }
}

/* parfumier: Drops the bouquet into his perfume machine, then tries to pour from the machine into a bottle.
 * If invalid, it spits fire!
 */
 
static void dayend_render_parfumier(struct zs_layer *layer) {

  // Corporate banner in the background, static.
  graf_draw_decal(&g.graf,g.texid_uibits,(FBW>>1)+NS_sys_tilesize*3,(FBH>>1)+12,183,1,78,32,0);
  
  // Establish conveyor belt's position, most of the rest depends on it. Ditto parfumier.
  int cvw=196,cvh=7;
  int cvx=(FBW>>1)-(cvw>>1);
  int cvy=FBH-NS_sys_tilesize*2;
  int pfx=(FBW>>1)-NS_sys_tilesize*3;
  int pfy=cvy-NS_sys_tilesize+3;
  
  // Establish belt timing.
  int bottlephase=0;
  if (LAYER->success&&(LAYER->clock>REVELATION_TIME)) {
    bottlephase=((int)((LAYER->clock-REVELATION_TIME)*12.000))%12;
  }
  
  // Bouquet arcs from parfumier to machine, before revelation.
  // Draw before the machine; we actually spend some time at the end moving inside it.
  if (LAYER->clock<REVELATION_TIME) {
    double xa=(double)(pfx);
    double xz=(double)((FBW>>1)-10);
    double t=LAYER->clock/REVELATION_TIME;
    int x=(int)(xa*(1.0-t)+xz*t);
    double normel=(t-0.5)*2.0; normel=1.0-(normel*normel);
    int y=pfy-NS_sys_tilesize-(int)(normel*50);
    graf_draw_decal(&g.graf,g.texid_bouquet,x,y,0,0,NS_sys_tilesize,NS_sys_tilesize,0);
  }
  
  // The Machine.
  int mx=55,my=140;
  if ((bottlephase>=6)&&(bottlephase<=9)) mx=87;
  else if (!LAYER->success&&(LAYER->clock>REVELATION_TIME)) mx=87;
  graf_draw_decal(&g.graf,g.texid_uibits,(FBW>>1)-20,cvy-36,mx,my,31,55,0);
  
  // Conveyor belt, 3 frames. Animates only after success revelation.
  int cvframe=bottlephase%3;
  graf_draw_decal(&g.graf,g.texid_uibits,cvx,cvy,123,45+cvframe*cvh,cvw,cvh,0);
  
  // Bottles. They move after success revelation. Trying to do this without any per-bottle state...
  int bottlew=6,bottleh=10;
  int bottley=cvy-bottleh;
  int bottlex=cvx+bottlephase;
  int fullc=(int)(LAYER->clock-REVELATION_TIME);
  if (bottlephase>=7) fullc++;
  int i=0; for (;;i++,bottlex+=12) {
    int srcx=183;
    if (bottlex>(FBW>>1)-8) {
      if (!LAYER->success) break;
      if (LAYER->clock<REVELATION_TIME) break;
      if (--fullc<0) break;
      srcx+=bottlew+1;
      if (bottlex>=cvx+cvw) {
        bottley+=(bottlex-cvw-cvx)*2;
        bottlex=cvx+cvw;
        if (bottley>=cvy+cvh) break;
      }
    }
    graf_draw_decal(&g.graf,g.texid_uibits,bottlex,bottley,srcx,34,bottlew,bottleh,0);
  }
  
  // Fire, on revealed failure.
  if (!LAYER->success&&(LAYER->clock>REVELATION_TIME)) {
    int srcx=(((int)(LAYER->clock*4.000))&1)?119:151;
    graf_draw_decal(&g.graf,g.texid_uibits,(FBW>>1)-20,cvy-36,srcx,140,31,55,0);
  }
  
  // Parfumier stands in front of the conveyor, a bit to the left of center.
  if (LAYER->clock<1.0) { // At first, we see him chucking the bouquet.
    graf_draw_decal(&g.graf,g.texid_sprites,pfx,pfy,NS_sys_tilesize*10,NS_sys_tilesize*7,NS_sys_tilesize,NS_sys_tilesize*2,0);
  } else {
    graf_draw_decal(&g.graf,g.texid_sprites,pfx,pfy,NS_sys_tilesize*8,NS_sys_tilesize*7,NS_sys_tilesize,NS_sys_tilesize*2,EGG_XFORM_XREV);
  }

  // Box to receive the bottls. Static.
  graf_draw_decal(&g.graf,g.texid_uibits,cvx+cvw-6,pfy+NS_sys_tilesize*2-12,262,1,20,12,0);
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
  for (;i-->0;label++) {
    if (label->after_reveal&&(LAYER->clock<REVELATION_TIME)) continue;
    graf_draw_decal(&g.graf,label->texid,label->x,label->y,0,0,label->w,label->h,0);
  }
  
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

static struct label *dayend_add_label_iii(struct zs_layer *layer,int rid,int index,int a,int b,int c) {
  char tmp[256];
  struct strings_insertion insv[]={
    {'i',{.i=a}},
    {'i',{.i=b}},
    {'i',{.i=c}},
  };
  int tmpc=strings_format(tmp,sizeof(tmp),rid,index,insv,3);
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
  
  // Generate labels.
  struct label *label;
  if (label=dayend_add_label_iii(layer,1,11,g.session.summaryc,DAYC,0)) {
    label->x=(FBW>>1)-(label->w>>1);
    label->y=18;
  }
  int total_score=session_calculate_score(); // The new one is already present.
  int previous_score=total_score-summary->score;
  if (label=dayend_add_label_iii(layer,1,12,previous_score,summary->score,total_score)) {
    label->x=(FBW>>1)-(label->w>>1);
    label->y=(FBH>>1)+2;
    label->after_reveal=1;
  }
  
  return layer;
}
