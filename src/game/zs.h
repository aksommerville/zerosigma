#ifndef ZS_H
#define ZS_H

#define FBW 320
#define FBH 180

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "egg_rom_toc.h"
#include "shared_symbols.h"

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
} g;

#endif
