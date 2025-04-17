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
          if (rid==RID_sprite_hero) {
            // The hero sprite is special. Only spawn if it we don't already have one.
            if (scene_get_hero()) break;
          }
          if (!sprite_spawn_cmd(col,row,rid,arg)) return -1;
        } break;
    
    }
  }
  return 0;
}

/* Load map etc.
 */
 
static int scene_load_map(int rid) {

  struct sprite *hero=scene_get_hero();
  if (hero) sprite_unlist(hero);
  sprites_drop();
  if (hero&&(sprite_relist(hero)<0)) return -1;
  
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
    if (sprite->defunct) continue;
    sprite->type->update(sprite,elapsed);
  }
}

/* Reap defunct sprites.
 */
 
static void reap_defunct_sprites() {
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (!sprite->defunct) continue;
    g.spritec--;
    memmove(g.spritev+i,g.spritev+i+1,sizeof(void*)*(g.spritec-i));
    sprite_del(sprite);
  }
}

/* Input events.
 */
 
void scene_button_down(int btnid) {
  hero_button_down(scene_get_hero(),btnid);
}

void scene_button_up(int btnid) {
  hero_button_up(scene_get_hero(),btnid);
}

/* If the hero is OOB, locate the relevant door and pass thru it.
 * We'll poll this every cycle.
 */
 
static void scene_check_doors_1(int edge,int loc,struct sprite *hero) {
  const struct zs_door *door=g.scene.map->doorv;
  int i=g.scene.map->doorc;
  for (;i-->0;door++) {
    if (door->edge!=edge) continue;
    if ((loc<door->p)||(loc>=door->p+door->c)) continue;
    struct zs_map *nmap=zs_map_get(door->mapid);
    if (nmap) switch (edge) {
      case ZS_EDGE_W: hero->y+=door->d; hero->x+=nmap->w; break;
      case ZS_EDGE_E: hero->y+=door->d; hero->x-=g.scene.map->w; break;
      case ZS_EDGE_N: hero->x+=door->d; hero->y+=nmap->h; break;
      case ZS_EDGE_S: hero->x+=door->d; hero->y-=g.scene.map->h; break;
    }
    scene_load_map(door->mapid);
    g.scene.door_blackout=0.500;
    if (hero&&hero->type->map_changed) hero->type->map_changed(hero);
    return;
  }
}
 
static void scene_check_doors(double elapsed) {
  if (g.scene.door_blackout>0.0) {
    g.scene.door_blackout-=elapsed;
    return;
  }
  struct sprite *hero=scene_get_hero();
  if (!hero) return;
  if (hero->x<0.0) scene_check_doors_1(ZS_EDGE_W,(int)hero->y,hero);
  else if (hero->x>=g.scene.map->w) scene_check_doors_1(ZS_EDGE_E,(int)hero->y,hero);
  else if (hero->y<0.0) scene_check_doors_1(ZS_EDGE_N,(int)hero->x,hero);
  else if (hero->y>=g.scene.map->h) scene_check_doors_1(ZS_EDGE_S,(int)hero->x,hero);
}

/* Update.
 */
 
void scene_update(double elapsed) {
  if ((g.scene.earthquake-=elapsed)<=0.0) {
    g.scene.earthquake=0.0;
  }
  scene_update_sprites(elapsed);
  reap_defunct_sprites();
  physics_update(elapsed);
  scene_check_doors(elapsed);
  //TODO Terminal conditions.
}

/* Refresh camera's position.
 */
 
static void scene_calculate_scroll() {
  struct sprite *hero=scene_get_hero();
  if (!hero) return;
  int worldw=g.scene.map->w*NS_sys_tilesize;
  if (worldw<=FBW) {
    g.scene.scrollx=(worldw>>1)-(FBW>>1);
  } else {
    int midx=(int)(hero->x*NS_sys_tilesize);
    g.scene.scrollx=midx-(FBW>>1);
    if (g.scene.scrollx<0) g.scene.scrollx=0;
    else if (g.scene.scrollx>worldw-FBW) g.scene.scrollx=worldw-FBW;
  }
  int worldh=g.scene.map->h*NS_sys_tilesize;
  if (worldh<=FBH) {
    g.scene.scrolly=(worldh>>1)-(FBH>>1);
  } else {
    int midy=(int)(hero->y*NS_sys_tilesize);
    g.scene.scrolly=midy-(FBH>>1);
    if (g.scene.scrolly<0) g.scene.scrolly=0;
    else if (g.scene.scrolly>worldh-FBH) g.scene.scrolly=worldh-FBH;
  }
  if (g.scene.earthquake>0.0) {
    int p=(int)(g.scene.earthquake*22.0)&7;
    switch (p) {
      case 0: g.scene.scrolly+=1; break;
      case 1: g.scene.scrolly+=0; break;
      case 2: g.scene.scrolly-=1; break;
      case 3: g.scene.scrolly-=2; break;
      case 4: g.scene.scrolly-=1; break;
      case 5: g.scene.scrolly-=0; break;
      case 6: g.scene.scrolly+=1; break;
      case 7: g.scene.scrolly+=2; break;
    }
    //if (g.scene.scrolly<0) g.scene.scrolly=0;
    //else if (g.scene.scrolly>worldh-FBH) g.scene.scrolly=worldh-FBH;
  }
}

/* Render sky, and if warranted, blotter.
 */
 
static void scene_render_bg() {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0xa0c0e0ff);
  if (g.scene.scrollx<0) graf_draw_rect(&g.graf,0,0,-g.scene.scrollx,FBH,0x000000ff);
  if (g.scene.scrolly<0) graf_draw_rect(&g.graf,0,0,FBW,-g.scene.scrolly,0x000000ff);
  int over;
  if ((over=g.scene.scrollx+FBW-g.scene.map->w*NS_sys_tilesize)>0) graf_draw_rect(&g.graf,FBW-over,0,over,FBH,0x000000ff);
  if ((over=g.scene.scrolly+FBH-g.scene.map->h*NS_sys_tilesize)>0) graf_draw_rect(&g.graf,0,FBH-over,FBW,over,0x000000ff);
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

/* Sprite list.
 */
 
struct sprite *scene_get_hero() {
  int i=g.spritec;
  while (i-->0) {
    struct sprite *sprite=g.spritev[i];
    if (sprite->type==&sprite_type_hero) return sprite;
  }
  return 0;
}

/* Begin earthquake.
 */
 
void scene_begin_earthquake() {
  g.scene.earthquake=1.000;
}
