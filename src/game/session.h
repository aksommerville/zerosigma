/* session.h
 * Singleton. The globals live in (g.session).
 * Manages long-term state like scoring.
 * Comes into scope after the main menu and goes out of scope after game over.
 */
 
#ifndef SESSION_H
#define SESSION_H

struct session {
  int TODO;
};

int session_reset();

#endif
