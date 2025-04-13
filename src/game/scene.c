#include "zs.h"

/* Apply commands for newly-loaded map.
 */
 
static int scene_apply_map_commands() {
  struct rom_command_reader reader={.v=g.scene.map->cmdv,.c=g.scene.map->cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
    
      case CMD_map_sprite: {
          uint8_t col=cmd.argv[0];
          uint8_t row=cmd.argv[1];
          uint16_t rid=(cmd.argv[2]<<8)|cmd.argv[3];
          uint32_t arg=(cmd.argv[4]<<24)|(cmd.argv[5]<<16)|(cmd.argv[6]<<8)|cmd.argv[7];
          if (!sprite_spawn_cmd(col,row,rid,arg)) return -1;
        } break;
        
      case CMD_map_door: {
          uint8_t col=cmd.argv[0];
          uint8_t row=cmd.argv[1];
          uint16_t mapid=(cmd.argv[2]<<8)|cmd.argv[3];
          uint8_t dstcol=cmd.argv[4];
          uint8_t dstrow=cmd.argv[5];
          uint16_t flags=(cmd.argv[6]<<8)|cmd.argv[7];
          fprintf(stderr,"TODO: Record door at (%d,%d) to map:%d at (%d,%d) flags=0x%04x\n",col,row,mapid,dstcol,dstrow,flags);
        } break;
    
    }
  }
  return 0;
}

/* Load map etc.
 */
 
static int scene_load_map(int rid) {
  //TODO Drop sprites. Keep the hero, maybe some others?
  if (!(g.scene.map=zs_map_get(rid))) {
    fprintf(stderr,"map:%d not found\n",rid);
    return -1;
  }
  fprintf(stderr,"%s: Loaded map:%d, %dx%d\n",__func__,rid,g.scene.map->w,g.scene.map->h);
  if (scene_apply_map_commands()<0) return -1;
  return 0;
}

/* Reset.
 */
 
int scene_reset() {
  if (scene_load_map(RID_map_home)<0) return -1;
  return 0;
}

/* Update all sprites that do it.
 */
 
static void scene_update_sprites(double elapsed) {
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (!sprite->type->update) continue;
    sprite->type->update(sprite,elapsed);
  }
}

/* Update.
 */
 
void scene_update(double elapsed) {
  scene_update_sprites(elapsed);
  //TODO
}

/* Refresh camera's position.
 */
 
static void scene_calculate_scroll() {
  //TODO
  g.scene.scrollx=0;
  g.scene.scrolly=0;
}

/* Render sky, and if warranted, blotter.
 */
 
static void scene_render_bg() {
  //TODO Check OOB
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0xa0c0e0ff);
}

/* Render map.
 */
 
static void scene_render_map() {
  int cola=g.scene.scrollx/NS_sys_tilesize;
  int rowa=g.scene.scrolly/NS_sys_tilesize;
  int colz=(g.scene.scrollx+FBW-1)/NS_sys_tilesize;
  int rowz=(g.scene.scrolly+FBH-1)/NS_sys_tilesize;
  if (cola<0) cola=0;
  if (rowa<0) rowa=0;
  if (colz>=g.scene.map->w) colz=g.scene.map->w-1;
  if (rowz>=g.scene.map->h) rowz=g.scene.map->h-1;
  if ((cola>colz)||(rowa>rowz)) return;
  int x0=cola*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.scene.scrollx;
  int y=rowa*NS_sys_tilesize+(NS_sys_tilesize>>1)-g.scene.scrolly;
  const uint8_t *srcrow=g.scene.map->v+rowa*g.scene.map->w+cola;
  int row=rowa; for (;row<=rowz;row++,y+=NS_sys_tilesize,srcrow+=g.scene.map->w) {
    int x=x0,col=cola;
    const uint8_t *srcp=srcrow;
    for (;col<=colz;col++,x+=NS_sys_tilesize,srcp++) {
      if (!*srcp) continue; // Tile zero must always be blank.
      graf_draw_tile(&g.graf,g.texid_bg,x,y,*srcp,0);
    }
  }
}

/* Render sprites.
 */
 
static void scene_render_sprites() {
  struct sprite **p=g.spritev;
  int i=g.spritec;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    int x=(int)(sprite->x*NS_sys_tilesize)-g.scene.scrollx;
    int y=(int)(sprite->y*NS_sys_tilesize)-g.scene.scrolly;
    if (sprite->type->render) {
      sprite->type->render(sprite,x,y);
    } else {
      graf_draw_tile(&g.graf,g.texid_sprites,x,y,sprite->tileid,sprite->xform);
    }
  }
}

/* Render overlay.
 */
 
static void scene_render_overlay() {
  //TODO Basket contents.
  //TODO Clock? HP? Minimap? Score?
}

/* Render.
 */
 
void scene_render() {
  scene_calculate_scroll();
  scene_render_bg();
  scene_render_map();
  scene_render_sprites();
  scene_render_overlay();
}
