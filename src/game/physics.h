/* physics.h
 * Trying to use as simple a physics model as I can manage.
 * Sprite controllers can move their sprites freely, and we correct everything in a big sweep at the end of each update.
 * Gravity gets applied by physics during this sweep.
 */
 
#ifndef PHYSICS_H
#define PHYSICS_H

#define DEFAULT_TERMINAL_VELOCITY 17.0 /* m/s */
#define GRAVITY_ACCELERATION 20.0 /* m/s**2 */
#define GRAVITY_START -0.125 /* m/s. <0 to create coyote time; effective gravity clamps to zero. */

void physics_update(double elapsed);

/* If (sprite) is supported exclusively by oneway tiles, move it down slightly to skip them and return positive.
 * Can't do that, return zero.
 */
int physics_downjump(struct sprite *sprite);

/* Nonzero if this point is within some solid body.
 */
int physics_check_point(double x,double y);

#endif
