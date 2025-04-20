/* sprite_type_customer.c
 */
 
#include "game/zs.h"

// Index in strings:1
#define MSGID_BOUQUET_PLEASE 3
#define MSGID_INSUFFICIENT_FLOWER 4
#define MSGID_ACCEPT 5

struct sprite_customer {
  struct sprite hdr;
  int texid_dialogue;
  int dlgw,dlgh;
  int msgid;
  uint8_t tileid0;
  int bouquetc; // (session.bouquetc) sampled at the time we draw the preview, for change detection.
  double initial_blackout;
};

#define SPRITE ((struct sprite_customer*)sprite)

/* Delete.
 */
 
static void _customer_del(struct sprite *sprite) {
  egg_texture_del(SPRITE->texid_dialogue);
}

/* Init.
 */
 
static int _customer_init(struct sprite *sprite) {
  sprite->terminal_velocity=0.0; // Not that they're immune to gravity, it just doesn't come up.
  sprite->layer=101;
  sprite->tileid=0x70+g.session.summaryc*2;
  SPRITE->tileid0=sprite->tileid;
  SPRITE->initial_blackout=0.750;
  return 0;
}

/* Set dialogue.
 */
 
static void customer_dialogue_none(struct sprite *sprite) {
  sprite->tileid=SPRITE->tileid0;
  SPRITE->dlgw=0;
  SPRITE->dlgh=0;
}

static void customer_dialogue(struct sprite *sprite,int msgid) {
  sprite->tileid=SPRITE->tileid0;
  if (msgid==SPRITE->msgid) {
    if (!SPRITE->dlgw) {
      egg_texture_get_status(&SPRITE->dlgw,&SPRITE->dlgh,SPRITE->texid_dialogue);
    }
  } else {
    egg_texture_del(SPRITE->texid_dialogue);
    SPRITE->texid_dialogue=font_texres_multiline(g.font,1,msgid,NS_sys_tilesize*4,NS_sys_tilesize*2,0x000000ff);
    egg_texture_get_status(&SPRITE->dlgw,&SPRITE->dlgh,SPRITE->texid_dialogue);
    SPRITE->msgid=msgid;
  }
}

/* Generate the bouquet image if we don't have it.
 */
 
#define REV32(n) (((n)<<24)|(((n)&0xff00)<<8)|(((n)&0xff0000)>>8)|((n)>>24))
 
static void customer_require_bouquet(struct sprite *sprite) {
  if (g.session.bouquetc==SPRITE->bouquetc) return;
  SPRITE->bouquetc=g.session.bouquetc;
  if (!g.texid_bouquet) {
    g.texid_bouquet=egg_texture_new();
    egg_texture_load_raw(g.texid_bouquet,NS_sys_tilesize,NS_sys_tilesize,NS_sys_tilesize<<2,0,0);
  }
  
  // Draw image:sprites:0x25 into our texture, then read out the pixels.
  // Even though it's just one op, we must use graf and not plain egg functions -- graf controls the global tint and alpha.
  egg_draw_clear(g.texid_bouquet,0);
  graf_set_output(&g.graf,g.texid_bouquet);
  graf_draw_tile(&g.graf,g.texid_sprites,NS_sys_tilesize>>1,NS_sys_tilesize>>1,0x25,0);
  graf_set_output(&g.graf,0);
  graf_flush(&g.graf);
  uint32_t pixels[NS_sys_tilesize*NS_sys_tilesize]={0};
  int err=egg_texture_get_pixels(pixels,sizeof(pixels),g.texid_bouquet);
  
  // Image must contain exactly BOUQUET_LIMIT (45) pixels of pure gray. Locate them.
  uint8_t grayv[BOUQUET_LIMIT];
  int grayc=0;
  uint32_t *p=pixels;
  int i=0; for (;i<NS_sys_tilesize*NS_sys_tilesize;i++,p++) {
    if (*p==0xff808080) {
      grayv[grayc++]=i;
      if (grayc>=BOUQUET_LIMIT) break;
    }
  }
  if (grayc!=BOUQUET_LIMIT) {
    fprintf(stderr,"image:sprites:0x25 must contain exactly %d gray pixels but we found %d.\n",BOUQUET_LIMIT,grayc);
    return;
  }
  
  // Select a random position from (grayv) for each flower in the session's bouquet.
  uint8_t available[BOUQUET_LIMIT];
  int availablec=grayc;
  memcpy(available,grayv,grayc);
  const struct session_flower *flower=g.session.bouquetv;
  for (i=g.session.bouquetc;i-->0;flower++) {
    int p=rand()%availablec;
    uint8_t pxix=available[p];
    availablec--;
    memmove(available+p,available+p+1,availablec-p);
    pixels[pxix]=REV32(flower->tint);
  }
  
  // All remaining gray pixels, fill in with green.
  const uint8_t *dst=available;
  for (i=availablec;i-->0;dst++) {
    pixels[*dst]=0xff20c000;
  }
  
  // Re-upload pixels to the texture.
  egg_texture_load_raw(g.texid_bouquet,NS_sys_tilesize,NS_sys_tilesize,NS_sys_tilesize<<2,pixels,sizeof(pixels));
}

/* Update.
 */
 
static void _customer_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->initial_blackout>0.0) SPRITE->initial_blackout-=elapsed;
  struct sprite *hero=scene_get_hero();
  if (!hero||(SPRITE->initial_blackout>0.0)) {
    customer_dialogue_none(sprite);
  } else {
    double dx=hero->x-sprite->x;
    double dy=hero->y-sprite->y;
    if ((dx<-2.5)||(dx>=0.5)||(dy<-1.0)||(dy>1.0)||(hero->xform!=0)) { // hero xform zero means facing right
      customer_dialogue_none(sprite);
    } else if (!g.session.bouquetc) {
      customer_dialogue(sprite,MSGID_BOUQUET_PLEASE);
      sprite->tileid=SPRITE->tileid0+1;
    } else if (g.session.bouquetc<5) {
      customer_dialogue(sprite,MSGID_INSUFFICIENT_FLOWER);
    } else {
      customer_dialogue(sprite,MSGID_ACCEPT);
      customer_require_bouquet(sprite);
    }
  }
  // We're fiddling capriciously with globals. It's cool though, because we know there is never more than one customer.
  if (SPRITE->dlgw&&(SPRITE->msgid==MSGID_ACCEPT)) {
    if (g.scene.finish<=0.0) g.scene.finish=SCENE_FINISH_TIME;
  } else {
    g.scene.finish=0.0;
  }
}

/* Render.
 */
 
static void _customer_render(struct sprite *sprite,int x,int y) {

  // Two tiles vertically, similar to the hero.
  graf_draw_tile(&g.graf,g.texid_sprites,x,y-NS_sys_tilesize,sprite->tileid,sprite->xform);
  graf_draw_tile(&g.graf,g.texid_sprites,x,y,sprite->tileid+0x10,sprite->xform);
  
  // If we have a dialogue texture, draw it in a word bubble.
  if ((SPRITE->dlgw>0)&&(SPRITE->dlgh>0)) {
    int dstx=x+NS_sys_tilesize-SPRITE->dlgw; // Lets it spill half a meter to the right.
    int dsty=y-(NS_sys_tilesize<<1)-SPRITE->dlgh; // Leaves some room above the head.
    graf_draw_rect(&g.graf,dstx-1,dsty-1,SPRITE->dlgw+2,SPRITE->dlgh+2,0xffffffff);
    graf_draw_decal(&g.graf,SPRITE->texid_dialogue,dstx,dsty,0,0,SPRITE->dlgw,SPRITE->dlgh,0);
    // Stemless border around the dialogue:
    graf_draw_rect(&g.graf,dstx-1,dsty-2,SPRITE->dlgw+2,1,0x000000ff);
    graf_draw_rect(&g.graf,dstx-2,dsty-1,1,SPRITE->dlgh+2,0x000000ff);
    graf_draw_rect(&g.graf,dstx+SPRITE->dlgw+1,dsty-1,1,SPRITE->dlgh+2,0x000000ff);
    graf_draw_rect(&g.graf,dstx-1,dsty+SPRITE->dlgh+1,SPRITE->dlgw+2,1,0x000000ff);
    // And finally, the stem is a tile:
    graf_draw_tile(&g.graf,g.texid_sprites,x,dsty+SPRITE->dlgh+1,0x24,0);
  }
  
  // If we are showing the ACCEPT text and have a bouquet image, show that floating in front of my face.
  if ((SPRITE->dlgw>0)&&(SPRITE->msgid==MSGID_ACCEPT)&&g.texid_bouquet) {
    int bx=x-NS_sys_tilesize-(NS_sys_tilesize>>1);
    int by=y-NS_sys_tilesize-(NS_sys_tilesize>>1);
    graf_draw_decal(&g.graf,g.texid_bouquet,bx,by,0,0,NS_sys_tilesize,NS_sys_tilesize,0);
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_customer={
  .name="customer",
  .objlen=sizeof(struct sprite_customer),
  .del=_customer_del,
  .init=_customer_init,
  .update=_customer_update,
  .render=_customer_render,
};
