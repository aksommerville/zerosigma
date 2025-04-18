/* scene.h
 * Singleton. The globals live in (g.scene).
 * Manages gameplay, closely associated with zs_layer_play.
 * The scene mostly destroys and rebuilds on map transitions, and completely rebuilds on episode transitions.
 */
 
#ifndef SCENE_H
#define SCENE_H

// Keep this above 1.0: From 1 to 0, we fade out.
#define SCENE_FINISH_TIME 1.5

struct scene {
  struct zs_map *map;
  int scrollx,scrolly; // Top left of view in map pixels, updated at the start of each render.
  double door_blackout;
  double earthquake;
  int sprite_sort_d;
  double finish; // Counts down if >0.
};

int scene_reset();

void scene_button_down(int btnid);
void scene_button_up(int btnid);

void scene_update(double elapsed);
void scene_render();

struct sprite *scene_get_hero();

void scene_begin_earthquake(double epix,double epiy);

#endif
