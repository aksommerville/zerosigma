#include "zs.h"

/* Flower colors.
 */
 
// Keep all channels in 0x10..0xf0: We randomly add or subtract up to 15 from each channel.
const struct color colorv[COLORC]={
  {0xf0,0x10,0x10}, // red
  {0x60,0x80,0xf0}, // blue
  {0xf0,0xa0,0xb0}, // pink
  {0xf0,0xf0,0x20}, // yellow
  {0xa0,0x20,0xf0}, // purple
};

const uint32_t display_colorv[COLORC]={
  0xf01010ff,
  0x6080f0ff,
  0xf0a0b0ff,
  0xf0f020ff,
  0xc060f0ff,
};
 
static uint32_t randomize_color(const struct color *color) {
  uint8_t r=color->r+((rand()&0x1f)-0x10);
  uint8_t g=color->g+((rand()&0x1f)-0x10);
  uint8_t b=color->b+((rand()&0x1f)-0x10);
  return (r<<24)|(g<<16)|(b<<8)|0xff;
}

/* We match colors to positions randomly, but there are always exactly 45 of each color.
 * That's important: If you're playing at the limit, you'll pick 9 of each at each of the 5 days, leaving exactly none at the end.
 * Shape is not important, so we pick randomly.
 */
 
static void session_reset_flowers() {
  g.session.flowerc=0;
  int remaining_by_color[COLORC]={45,45,45,45,45};
  uint16_t flowerid=1;
  struct zs_map *map=g.mapv;
  int mapi=g.mapc;
  for (;mapi-->0;map++) {
    const uint8_t *v=map->v;
    int row=0; for (;row<map->h;row++) {
      int col=0; for (;col<map->w;col++,v++) {
        if ((*v<0x05)||(*v>0x08)) continue;
        if (g.session.flowerc>=FLOWER_LIMIT) {
          fprintf(stderr,"!!! Too many flowers (%d).\n",FLOWER_LIMIT);
          return;
        }
        
        int range=0,i=0;
        for (;i<COLORC;i++) range+=remaining_by_color[i];
        if (!range) return;
        int choice=rand()%range;
        int colorid=0;
        for (i=0;i<COLORC;i++) {
          if ((choice-=remaining_by_color[i])<0) {
            remaining_by_color[i]--;
            colorid=i;
            break;
          }
        }
        
        struct session_flower *flower=g.session.flowerv+g.session.flowerc++;
        flower->flowerid=flowerid++;
        flower->mapid=map->rid;
        flower->x=col;
        flower->y=row;
        flower->tileid=0x40+rand()%3;
        flower->colorid=colorid;
        flower->tint=randomize_color(colorv+flower->colorid);
      }
    }
  }
  fprintf(stderr,"%s: %d flowers in %d maps\n",__func__,g.session.flowerc,g.mapc);
}

/* Reset.
 */
 
int session_reset() {
  if (g.session.flag_listenerv) free(g.session.flag_listenerv);
  struct top tmptopv[TOP_COUNT]; // Don't wipe (top).
  memcpy(tmptopv,g.session.topv,sizeof(tmptopv));
  memset(&g.session,0,sizeof(struct session));
  memcpy(g.session.topv,tmptopv,sizeof(tmptopv));
  g.session.flagv[NS_flag_zero]=0;
  g.session.flagv[NS_flag_one]=1;
  session_reset_flowers();
  return 0;
}

/* Search flower list.
 */
 
int session_flowerp_by_flowerid(int flowerid) {
  if (flowerid<0) return -1;
  if (flowerid>0xffff) return -g.session.flowerc-1;
  int lo=0,hi=g.session.flowerc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct session_flower *flower=g.session.flowerv+ck;
         if (flowerid<flower->flowerid) hi=ck;
    else if (flowerid>flower->flowerid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

int session_flowerp_by_mapid(int mapid) {
  if (mapid<0) return 0;
  if (mapid>0xffff) return g.session.flowerc;
  int lo=0,hi=g.session.flowerc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct session_flower *flower=g.session.flowerv+ck;
         if (mapid<flower->mapid) hi=ck;
    else if (mapid>flower->mapid) lo=ck+1;
    else {
      while ((ck>lo)&&(flower[-1].mapid==mapid)) { ck--; flower--; }
      return ck;
    }
  }
  return lo;
}

int session_flowerp_by_location(int mapid,int x,int y) {
  if ((mapid<0)||(mapid>0xffff)||(x<0)||(x>=0xff)||(y<0)||(y>0xff)) return -1;
  int lo=0,hi=g.session.flowerc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct session_flower *flower=g.session.flowerv+ck;
         if (mapid<flower->mapid) hi=ck;
    else if (mapid>flower->mapid) lo=ck+1;
    else if (y<flower->y) hi=ck;
    else if (y>flower->y) lo=ck+1;
    else if (x<flower->x) hi=ck;
    else if (x>flower->x) lo=ck+1;
    else return ck;
  }
  return -1;
}

/* Flags.
 */
 
int session_get_flag(int flagid) {
  if ((flagid<0)||(flagid>=NS_flag_COUNT)) return 0;
  return g.session.flagv[flagid];
}

void session_set_flag(int flagid,int v) {
  if ((flagid<2)||(flagid>=NS_flag_COUNT)) return; // sic 2: Flags zero and one are not allowed to change.
  v&=0xff;
  if (g.session.flagv[flagid]==v) return;
  g.session.flagv[flagid]=v;
  int i=g.session.flag_listenerc;
  while (i-->0) {
    struct flag_listener *listener=g.session.flag_listenerv+i;
    if ((listener->flagid==flagid)||(listener->flagid<0)) {
      listener->cb(flagid,v,listener->userdata);
    }
  }
}

/* Flag listeners.
 */
 
int session_listen_flag(int flagid,void (*cb)(int flagid,int v,void *userdata),void *userdata) {
  if (g.session.broadcast_in_progress) return -1;
  if (!cb) return -1;
  if (g.session.flag_listenerc>=g.session.flag_listenera) {
    if (g.session.flag_listenera>256) {
      fprintf(stderr,"Too many flag listeners. Likely leak. [%s:%d]\n",__FILE__,__LINE__);
      return -1;
    }
    int na=g.session.flag_listenera+16;
    if (na>INT_MAX/sizeof(struct flag_listener)) return -1;
    void *nv=realloc(g.session.flag_listenerv,sizeof(struct flag_listener)*na);
    if (!nv) return -1;
    g.session.flag_listenerv=nv;
    g.session.flag_listenera=na;
  }
  if (g.session.flag_listener_next<1) g.session.flag_listener_next=1;
  struct flag_listener *listener=g.session.flag_listenerv+g.session.flag_listenerc++;
  listener->listenerid=g.session.flag_listener_next++;
  listener->flagid=flagid;
  listener->cb=cb;
  listener->userdata=userdata;
  return listener->listenerid;
}

void session_unlisten_flag(int listenerid) {
  int lo=0,hi=g.session.flag_listenerc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct flag_listener *listener=g.session.flag_listenerv+ck;
         if (listenerid<listener->listenerid) hi=ck;
    else if (listenerid>listener->listenerid) lo=ck+1;
    else {
      g.session.flag_listenerc--;
      memmove(listener,listener+1,sizeof(struct flag_listener)*(g.session.flag_listenerc-ck));
      return;
    }
  }
  fprintf(stderr,"%s !!!! listenerid %d not found !!!! listenerc=%d\n",__func__,listenerid,g.session.flag_listenerc);
}

/* Add up score.
 */
 
int session_calculate_score() {
  const struct summary *summary=g.session.summaryv;
  int i=g.session.summaryc;
  int total=0;
  for (;i-->0;summary++) total+=summary->score;
  return total;
}

/* High score persistence.
 * Format is space-delimited postive decimal integers corresponding to (topv[0].time,topv[0].score,...).
 */
 
void session_top_load() {
  memset(g.session.topv,0,sizeof(g.session.topv));
  char src[256];
  int srcc=egg_store_get(src,sizeof(src),"hiscore",7);
  if ((srcc<1)||(srcc>sizeof(src))) return;
  int srcp=0,topp=0,fieldp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if ((src[srcp]<'0')||(src[srcp]>'9')) return;
    int v=0;
    while ((srcp<srcc)&&(src[srcp]>='0')&&(src[srcp]<='9')) {
      v*=10;
      v+=src[srcp++]-'0';
    }
    switch (fieldp) {
      case 0: g.session.topv[topp].time=v; fieldp=1; break;
      case 1: g.session.topv[topp].score=v; fieldp=0; topp++; if (topp>=TOP_COUNT) return; break;
    }
  }
}

static int decuint_repr(char *dst,int dsta,int src) {
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

static void session_top_save() {
  char tmp[256];
  const struct top *top=g.session.topv;
  int i=TOP_COUNT,tmpc=0;
  for (;i-->0;top++) {
    tmpc+=decuint_repr(tmp+tmpc,sizeof(tmp)-tmpc,top->time);
    tmp[tmpc++]=' ';
    tmpc+=decuint_repr(tmp+tmpc,sizeof(tmp)-tmpc,top->score);
    if (i) tmp[tmpc++]=' ';
  }
  egg_store_set("hiscore",7,tmp,tmpc);
}

/* Update one top if the incoming score is better than what we have.
 * Return nonzero if changed.
 */
 
static int session_top_commit_1(struct top *top,int score,int time) {
  if (!top->time) { // No previous score was set.
    top->time=time;
    top->score=score;
    top->time_new=1;
    top->score_new=1;
    return 1;
  }
  int dirty=0;
  if (time<top->time) {
    top->time=time;
    top->time_new=1;
    dirty=1;
  }
  if (score>top->score) {
    top->score=score;
    top->score_new=1;
    dirty=1;
  }
  return dirty;
}

/* Commit loose score into (topv), and save if a record was broken.
 */
 
void session_top_commit() {
  fprintf(stderr,"%s\n",__func__);
  if (g.session.summaryc!=DAYC) return; // We're not looking at a finished game...?
  
  int score=0,i=g.session.summaryc,allvalid=1,dirty=0;
  const struct summary *summary=g.session.summaryv;
  for (;i-->0;summary++) {
    score+=summary->score;
    if (!summary->score) allvalid=0;
  }
  int time=(int)(g.session.playtime*1000.0);
  fprintf(stderr,"score=%d time=%d allvalid=%d mapchangec=%d\n",score,time,allvalid,g.session.mapchangec);
  
  // Clear the "new" flags.
  struct top *top=g.session.topv;
  for (i=TOP_COUNT;i-->0;top++) top->time_new=top->score_new=0;
  
  // FINISHED if you finished at all, which you did.
  if (session_top_commit_1(g.session.topv+TOP_FINISHED,score,time)) dirty=1;
  
  // ALL_VALID if you got points from every customer.
  if (allvalid) {
    if (session_top_commit_1(g.session.topv+TOP_ALL_VALID,score,time)) dirty=1;
  }
  
  // ALL_PERFECT if you have 500 points; there's no other way to do that.
  if (score==500) {
    if (session_top_commit_1(g.session.topv+TOP_ALL_PERFECT,score,time)) dirty=1;
  }
  
  // HERMIT if you got ALL_VALID and also didn't leave the home map.
  if (allvalid&&!g.session.mapchangec) {
    if (session_top_commit_1(g.session.topv+TOP_HERMIT,score,time)) dirty=1;
  }
  
  if (dirty) {
    fprintf(stderr,"Set a new high score. Saving...\n");
    session_top_save();
  }
}
