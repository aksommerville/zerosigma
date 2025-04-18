#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "game/zs.h"

#define HERO_WALK_SPEED 8.0 /* m/s */
#define HERO_JUMP_POWER_DEFAULT 23.0 /* m/s */
#define HERO_JUMP_POWER_DECELERATION 48.0 /* m/s**2 */
#define HERO_WALLGRAB_JUMP_POWER_LIMIT 15.0
#define HERO_COYOTE_TIME 0.040 /* s */
#define HERO_WALLJUMPX_INITIAL 18.0 /* m/s */
#define HERO_WALLJUMPY_INITIAL 24.0 /* m/s */
#define HERO_WALLJUMPX_DECELERATION 100.0 /* m/s**2 */
#define HERO_WALLJUMPY_DECELERATION 100.0 /* m/s**2 */
#define HERO_WALLJUMP_SUSPENDX 0.150 /* s */
#define HERO_FASTFALL_VELOCITY 28.0 /* m/s */
#define HERO_WALLGRAB_VELOCITY 4.0 /* m/s */
#define HERO_LADDER_CLIMB_SPEED 4.0 /* m/s */
#define HERO_ECHO_LIMIT 60
#define HERO_TELEPORT_HIGHLIGHT_TIME 0.250 /* s */
#define HERO_TELEPORT_REJECT_TIME 0.125 /* s, a quick blink to signal "yeah you pressed the button but we're not doing that" */

struct sprite_hero {
  struct sprite hdr;
  double animclock;
  int animframe;
  int input; // Full input state.
  int indx,indy,injump,initem; // Digested input state.
  int walkdx; // -1,0,1
  double jump_power;
  int walljump; // Nonzero during operation
  double walljump_xpower;
  double walljump_ypower;
  double suspendx;
  int fastfall;
  double fastfall_clock;
  double sorefoot; // Counts down while hurt.
  int wallgrab;
  int seated;
  double ladderx; // >0.0, center of ladder column, if we're climbing.
  double teleport_highlight; // Counts down after a teleport.
  
  struct hero_echo {
    double x,y;
    uint8_t tileid,xform; // (tileid) is the top tile, we also draw +0x10.
    int ttl;
  } echov[HERO_ECHO_LIMIT];
  int echop,echoc;
  int echo_record; // Counts down while recording.
  
  /* Present if there's a flower we can pick right now.
   * This points weakly into g.session.flowerv.
   */
  struct session_flower *flower;
};

#define SPRITE ((struct sprite_hero*)sprite)

void hero_fall_begin(struct sprite *sprite);
void hero_fall_end(struct sprite *sprite,double duration);

void hero_begin_echo(struct sprite *sprite,int framec);

// Call as a jump crests. We'll check the Y axis and if it's Up and we're near a ladder, we'll grab the ladder.
void hero_rejoin_ladder(struct sprite *sprite);

#endif
