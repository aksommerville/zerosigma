/* scene.h
 * Singleton. The globals live in (g.scene).
 * Manages gameplay, closely associated with zs_layer_play.
 * The scene mostly destroys and rebuilds on map transitions, and completely rebuilds on episode transitions.
 */
 
#ifndef SCENE_H
#define SCENE_H

struct scene {
  struct zs_map *map;
  int scrollx,scrolly; // Top left of view in map pixels, updated at the start of each render.
  double door_blackout;
  double earthquake;
  int sprite_sort_d;
};

int scene_reset();

void scene_button_down(int btnid);
void scene_button_up(int btnid);

void scene_update(double elapsed);
void scene_render();

struct sprite *scene_get_hero();

void scene_begin_earthquake();

#endif
