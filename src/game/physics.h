/* physics.h
 * Trying to use as simple a physics model as I can manage.
 * Sprite controllers can move their sprites freely, and we correct everything in a big sweep at the end of each update.
 * Gravity gets applied by physics during this sweep.
 */
 
#ifndef PHYSICS_H
#define PHYSICS_H

#define DEFAULT_TERMINAL_VELOCITY 6.0 /* m/s */

void physics_update(double elapsed);

#endif
