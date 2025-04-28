/* refresh_bouquet_image.c
 * Replace (g.texid_bouquet) with a 16x16 image based on (g.session.bouquetv).
 */

#include "zs.h"

void refresh_bouquet_image() {
  int i;

  if (!g.texid_bouquet) {
    g.texid_bouquet=egg_texture_new();
  }

  /* If we don't have the loose pixels yet, acquire them.
   * Use a single-use texture for the transfer.
   * On MacOS, the re-upload is randomly (?) failing, and I suspect it's something to do with replacing that texture with a renderbuffer still attached.
   * We have to use graf for this because there might be a render in progress, and it controls the global tint and alpha.
   */
  if (!g.bouquetbits) {
    int texidtmp=egg_texture_new();
    egg_texture_load_raw(texidtmp,NS_sys_tilesize,NS_sys_tilesize,NS_sys_tilesize<<2,0,0);
    egg_draw_clear(texidtmp,0);
    graf_set_output(&g.graf,texidtmp);
    graf_draw_tile(&g.graf,g.texid_sprites,NS_sys_tilesize>>1,NS_sys_tilesize>>1,0x25,0);
    graf_set_output(&g.graf,0);
    graf_flush(&g.graf);
    int len=NS_sys_tilesize*NS_sys_tilesize*4;
    if (!(g.bouquetbits=malloc(len))) {
      egg_texture_del(texidtmp);
      return;
    }
    egg_texture_get_pixels(g.bouquetbits,len,texidtmp);
    egg_texture_del(texidtmp);

    // Find the pure-gray pixels, those are the ones we replace each time.
    int c=0;
    int p=0; for (;p<256;p++) {
      if (g.bouquetbits[p]!=0xff808080) continue;
      if (c>=BOUQUET_LIMIT) {
        fprintf(stderr,"*** %s: Too many gray pixels in bouquet source image. Should be %d\n",__func__,BOUQUET_LIMIT);
        break;
      }
      g.bouquetgrayv[c++]=p;
    }
    if (c<BOUQUET_LIMIT) {
      fprintf(stderr,"*** %s: Too few gray pixels in bouquet source image. Should be %d\n",__func__,BOUQUET_LIMIT);
    }
  }
  
  // Select a random position from (g.bouquetgrayv) for each flower in the session's bouquet.
  uint8_t available[BOUQUET_LIMIT];
  int availablec=BOUQUET_LIMIT;
  memcpy(available,g.bouquetgrayv,BOUQUET_LIMIT);
  const struct session_flower *flower=g.session.bouquetv;
  for (i=g.session.bouquetc;i-->0;flower++) {
    int p=rand()%availablec;
    uint8_t pxix=available[p];
    availablec--;
    memmove(available+p,available+p+1,availablec-p);
    #define REV32(n) (((n)<<24)|(((n)&0xff00)<<8)|(((n)&0xff0000)>>8)|((n)>>24))
    g.bouquetbits[pxix]=REV32(flower->tint);
    #undef REV32
  }
  
  // All remaining gray pixels, fill in with green.
  const uint8_t *dst=available;
  for (i=availablec;i-->0;dst++) {
    g.bouquetbits[*dst]=0xff20c000;
  }
  
  // Re-upload pixels to the texture.
  egg_texture_load_raw(g.texid_bouquet,NS_sys_tilesize,NS_sys_tilesize,NS_sys_tilesize<<2,g.bouquetbits,NS_sys_tilesize*NS_sys_tilesize*4);
}
