#include "zs.h"

/* Set defaults.
 */
 
static void hiscore_set_default(struct hiscore *hiscore) {
  memset(hiscore,0,sizeof(struct hiscore));
  // The default is all zeroes.
}

/* Decode from untrusted storage.
 * If there's something invalid, or too much content, we'll stop reading and leave in place whatever we had.
 * No validation, and only sets (v), not (validscores) or (newscores).
 */
 
static void hiscore_decode(const char *src,int srcc) {
  fprintf(stderr,"%s, %d bytes\n",__func__,srcc);
  int srcp=0,dstp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if ((src[srcp]<'0')||(src[srcp]>'9')) return;
    int v=0;
    while ((srcp<srcc)&&(src[srcp]>='0')&&(src[srcp]<='9')) {
      v*=10;
      v+=src[srcp++]-'0';
    }
    g.hiscore.v[dstp++]=v;
    if (dstp>=HISCORE_COUNT) return;
  }
}

/* Validate in place.
 * Forces all of (v[]) to legal values and sets (validscores).
 */
 
static void hiscore_validate(struct hiscore *hiscore) {
  // Check each field. If it is out of range, set it zero. Otherwise set the corresponding (validscores) bit.
  #define V(tag) hiscore->v[HISCORE_##tag]
  #define VALID(tag) hiscore->validscores|=1<<HISCORE_##tag;
  if (V(TIME)<=0) {
    // If we don't have a valid TIME, nothing else can be set. Clear it all.
    memset(hiscore,0,sizeof(struct hiscore));
  } else {
    VALID(TIME)
    if (V(VALID_TIME)>0) VALID(VALID_TIME) else V(VALID_TIME)=0;
    if (V(PERFECT_TIME)>0) VALID(PERFECT_TIME) else V(PERFECT_TIME)=0;
    if ((V(SCORE)>0)&&(V(SCORE)<=500)) VALID(SCORE) else V(SCORE)=0;
    if ((V(HERMIT_SCORE)>0)&&(V(HERMIT_SCORE)<=57)) VALID(HERMIT_SCORE) else V(HERMIT_SCORE)=0;
    if ((V(MISER_SCORE)>0)&&(V(MISER_SCORE)<=236)) VALID(MISER_SCORE) else V(MISER_SCORE)=0;
  }
  #undef V
  #undef VALID
}

/* Load.
 */
 
void hiscore_load() {
  hiscore_set_default(&g.hiscore);
  char src[256];
  int srcc=egg_store_get(src,sizeof(src),"hiscore",7);
  if ((srcc>0)&&(srcc<=sizeof(src))) {
    hiscore_decode(src,srcc);
    hiscore_validate(&g.hiscore);
  }
}

/* Calculate and validate a hiscore from (g.session).
 * Don't touch (g.hiscore).
 */
 
void hiscore_from_session(struct hiscore *dst) {
  hiscore_set_default(dst);
  // Nothing gets set if we don't have a complete session.
  if (g.session.summaryc==DAYC) {
    dst->v[HISCORE_TIME]=(int)(g.session.playtime*1000.0);
    int allvalid=1;
    const struct summary *summary=g.session.summaryv;
    int i=g.session.summaryc;
    for (;i-->0;summary++) {
      if (summary->score) {
        dst->v[HISCORE_SCORE]+=summary->score;
      } else {
        allvalid=0;
      }
    }
    if (allvalid) dst->v[HISCORE_VALID_TIME]=dst->v[HISCORE_TIME];
    if (dst->v[HISCORE_SCORE]>=500) dst->v[HISCORE_PERFECT_TIME]=dst->v[HISCORE_TIME];
    if (!g.session.mapchangec) dst->v[HISCORE_HERMIT_SCORE]=dst->v[HISCORE_SCORE];
    if (g.session.miser) dst->v[HISCORE_MISER_SCORE]=dst->v[HISCORE_SCORE];
  }
  hiscore_validate(dst);
}

/* Represent unsigned decimal integer.
 * (dst) must have room for at least 10.
 */

static int decuint_repr(char *dst,int src) {
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

/* Encode.
 * Provide at least (11*HISCORE_COUNT==66) bytes.
 */
 
static int hiscore_encode(char *dst,int dsta) {
  int dstc=0,i=0;
  for (;i<HISCORE_COUNT;i++) {
    dstc+=decuint_repr(dst+dstc,g.hiscore.v[i]);
    dst[dstc++]=' ';
  }
  return dstc;
}

/* Compare two scores, assuming both are valid.
 * If the old one is unset, handle that separate without calling us.
 * Nonzero if (a) is better than (b) -- can mean greater or lesser depending on (id).
 */
 
static int hiscore_better(int id,int a,int b) {
  switch (id) {
    case HISCORE_TIME:
    case HISCORE_VALID_TIME:
    case HISCORE_PERFECT_TIME:
      return (a<b);
    case HISCORE_SCORE:
    case HISCORE_HERMIT_SCORE:
    case HISCORE_MISER_SCORE:
      return (a>b);
  }
  return 0;
}

/* Commit.
 */

void hiscore_commit() {
  fprintf(stderr,"%s...\n",__func__);
  hiscore_from_session(&g.prevscore);
  g.hiscore.newscores=0;
  uint32_t mask=1;
  int i=0; for (;i<HISCORE_COUNT;i++,mask<<=1) {
    if (!(g.prevscore.validscores&mask)) continue;
    if (!(g.hiscore.validscores&mask)) {
      fprintf(stderr,"NEW HIGH SCORE #%d: unset => %d\n",i,g.prevscore.v[i]);
      g.hiscore.v[i]=g.prevscore.v[i];
      g.hiscore.validscores|=mask;
      g.hiscore.newscores|=mask;
    } else if (hiscore_better(i,g.prevscore.v[i],g.hiscore.v[i])) {
      fprintf(stderr,"NEW HIGH SCORE #%d: %d => %d\n",i,g.hiscore.v[i],g.prevscore.v[i]);
      g.hiscore.v[i]=g.prevscore.v[i];
      g.hiscore.validscores|=mask;
      g.hiscore.newscores|=mask;
    }
  }
  if (g.hiscore.newscores) {
    char serial[256];
    int serialc=hiscore_encode(serial,sizeof(serial));
    if ((serialc>0)&&(serialc<=sizeof(serial))) {
      egg_store_set("hiscore",7,serial,serialc);
    }
  }
}

/* Test for perfect scores.
 */
 
int hiscore_is_perfect(int id,int v) {
  switch (id) {
    case HISCORE_SCORE: return (v>=500);
    case HISCORE_HERMIT_SCORE: return (v>=57);
    case HISCORE_MISER_SCORE: return (v>=126);
  }
  return 0;
}
