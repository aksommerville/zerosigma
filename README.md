# Dot's Wicked Garden

Requires [Egg](https://github.com/aksommerville/egg) to build.

Submission for Gamedev.js Jam 2025, theme "BALANCE".

Gather flowers of various colors and get scored based on count and color balance.

Apologies for the mismatched repo name! At first I was calling the game "Zero Sigma Florist", but decided I didn't care for that name.

## Timeline

```
  U 2025-04-13T11:00: Brainstorming. Design conceptually on paper, don't touch the keyboard yet.
  U 2025-04-13T12:00: Commit to a concept and platform. Provision repo. Begin work.
  U 2025-04-13 EOD  : Resource loader and hero movements (walk, jump, downjump, walljump...)
  M 2025-04-14      : Linked maps.
  T 2025-04-15      : Episodes.
  W 2025-04-16      : Collect flowers, scoring.
  R 2025-04-17      : Hazards, interactive world things, ...
  F 2025-04-18      : Hazards and challenges.
  S 2025-04-19      : New music and sound effects.
  U 2025-04-20      : Day-end animation and proper scoring.
  M 2025-04-21      : World map
  T 2025-04-22      : World map
  W 2025-04-23      : Touch-up
  R 2025-04-24      : Itch page.
  R 2025-04-24 EOD  : Panic if it doesn't look done yet.
  F 2025-04-25T18:00: Aim to be 100% done and submitted by now. Retain late Friday and early Saturday for emergency overflow.
  F 2025-04-25 EOD  : Game and Itch page must be finished and published. Submit.
  S 2025-04-26T11:00: Submissions close.
```

## Actual Daily Progress

```
  U 2025-04-13: Concept, repo, nice hero walking graphics, sketchy division of labor, provisional partial physics.
  M 2025-04-14: Jump, downjump, walljump. Gravity and basic motions feel ok. Sketch of background music (petal_to_the_metal).
  T 2025-04-15: Music refined, home map cleaned up, fastfall, wallgrab.
  W 2025-04-16: Map connections, ladders, earthquake, echo. Grow flowers.
  R 2025-04-17: Scorekeeping, customers. More music.
  F 2025-04-18: Teleport, squishroom, goat, treadle, flamethrower.
  S 2025-04-19: Sound effects, revised music, started dayend animations in earnest.
  U 2025-04-20: Dayend animations complete. Map layouts. Persisting achievements, but I'm going to remodel them.
  M 2025-04-21: Revised scoring model, display scores at hello
  T 2025-04-22: Options and Dot pic at Hello. Lil fixes.
  W 2025-04-23: Fleshed out graphics, esp terrain. Itch page. Submitted to jam.
  R 2025-04-24
  F 2025-04-25
  S 2025-04-26
```

## TODO

- [x] Basic graphics. Terrain and hero.
- [x] Load resources. map, sprite, tilesheet
- [x] Platforming.
- - [x] Down jump
- - [x] Wall jump
- - [x] Fast fall
- [x] Map loader and connections. Use the regular door command for edge doors too.
- [x] Collect flowers.
- [x] Episodes.
- [x] Scoring.
- [x] Use `n*m` instead of `n**m`. Daily score in 5..45. But at 45, bonus-bump it to 100.
- - [x] Maybe eliminate the 3-colors rule? It feels kind of arbitrary.
- [x] Can I add a dash or longjump or something? Keep wanting to accelerate horizontally.
- - [x] Try an instant horizontal teleport, say 2.5 m. ...love it
- [x] It's still possible to finish a walljump and end up walking backward (facing the wall). Did it twice but can't reproduce reliably.
- - REPRO: Turn around as you press walljump, and release jump immediately.
- [x] Hazards and challenges. Nothing actually harmful, I don't want HP or even die-and-respawn. But things that knock you back, paralyze you...
- - [x] Things to break by fastfalling.
- - [x] A few flowers that become unreachable after fastfall-breaking something, have to pick them before.
- - - We can probably do this with a squishroom: Put a oneway platform 5 meters above it that could only be reached by jumping from the squishroom.
- - [x] Walljump across a 1-meter outcrop.
- - [x] 2-meter outcrop? It's possible but very difficult.
- - [x] Goat that eats flowers if you wake him up.
- - xxx Monkey that grabs your back and restricts jumping.
- - [x] Treadle-triggered flamethrower, burns the flowers.
- - xxx Crush-o-matic, forces you to the edge you entered from.
- - xxx Elevator that drops when you hold a treadle. Must fastfall after it to reach the next level.
- - [x] Can we arrange a challenge that requires you to interrupt a fastfall?
- - [x] Short platforms high up. Far enough apart that eventually you can't make it without teleporting! ...already ready
- [x] Proper maps.
- - [x] Each of the 6 outer maps should be split into pre- and post-challenge. No more than 9 flowers post-challenge, so as long as you enter fresh, you won't need to enter twice.
- - - Or should we add more logic to flower placement, to make even tighter constraints?
- - - Keep in mind that there are 6 outer maps, but only 5 customers.
- - - Home map must introduce walljump and downjump.
- - - Flowers: 9*6 post-challenge, 19*6 pre-challenge, 57 home.
- - - Six challenges:
- - - - [x] Teleport-jump to narrow platforms. nw
- - - - [x] Don't squish the mushroom yet. n
- - - - [x] Fastfall to slip past a flamethrower, and there's a goat below. ne
- - - - [x] Walljump over 1-meter outcrop. sw
- - - - [x] Make the goat press a button. s
- - - - [x] Teleport thru a pillar. se
- - [x] Can I make 6 different regional graphics variations? ...we don't have enough tilesheet or time for fully unique regions. Instead just fill the tilesheet and place things willy nilly.
- [x] Sound effects.
- [x] Menus.
- - [x] Hello
- - [x] Game over -- XXX Remove, just use Hello.
- - [x] Deliver
- [x] Achievements. Calculate these at gameover, persist, and display at gameover and hello. Record best score and time (independently) for each.
- - Finish session.
- - Five valid bouquets.
- - Five perfect bouquets.
- - Bonus tokens to collect?
- - Five valid bouquets without leaving home. (if there's >=45 on the home screen, it's always possible, and there's 57).
- [x] It's weird to show time and score for each achievement, since most are a subset of Five Perfect Bouquets. Model it different.
- [x] Change the music. `cosmic_balance` is too melancholy and `petal_to_the_metal` is too heroic. They should be fun and silly.
- - Save `petal_to_the_metal` for something else, it's a great tune.
- - ...actually I'm not sure. I really do like PTTM. Prioritize the others, maybe keep this.
- - Day-end songs should have a much shorter lead-in.
- [x] Change the title.
- - "Zero Sigma" makes even less sense now that I've decided not to involve either sum or standard deviation in the formula.
- - "Tidy Garden"? "Magic Garden"? "Ninja Garden", if Dot was a ninja instead of a witch.
- - "Dot's Wicked Garden". Colorful Flowers & Black Magic, A Perfect Balance In Every Bouquet!
- [x] Recheck ladder at the peak of a jump, if Y axis nonzero. So you can jump up a ladder instead of climbing, without having to release and re-press Up.
- [x] Use Up instead of Down for picking.
- [x] Seems like the SE room (test maps) is usually getting the same color for all flowers. And these are the very end of the list. Is something wrong with the RNG at flower placement?
- - Reviewed a little closer and yes, it does seem to be noticeably eccentric at least half the time.
- - We were selecting a color based on all available colors, regardless of how many remain. Easy change to pick among the total remaining *flowers* rather than *colors*.
- [x] I must have messed up the RNG again. Ran out of pink flowers in the third episode. ...forgot to decrement the counters, duh
- [x] Playing around fast, I ended up one pixel above the ground on a platform, on map:1. Unclear how. Jump power did not reset until I walked off the platform.
- - Happens pretty reliably if you climb onto a ladder from the ground, then climb it back down all the way. Main symptom is you won't be allowed to pick flowers.
- - Need to cheat `graviting` back on as you exit a ladder. Huh, seems like I was just always hopping off ladders above the ground.
- [x] NW: Make some variety in the pit, it's boring.
- [x] SW: Can I prevent the player from scaling the left wall instead of the right?
- [x] Web (Egg): Bouquet is not showing colors. Dollars to donuts, it's `egg_texture_read_pixels`
- [x] Performance: I'm getting ~7% regularly, which is fine, but it ought to be more like ~4%. I suspect the flower rendering is producing unnecessary churn.
- - [x] Turn off flower rendering to confirm. ...FALSE! We get down near 6% with flower rendering off, but not the dramatic drop I expected. Not worth taking extraordinary measures.
- - [x] Easy: Don't render sprites when they're offscreen. ...was already doing this for flowers
- - [x] Hard: Make a new programmatic tilesheet for flowers, populate at map load. ...not worth the trouble or risk
- - [x] Check all the things that iterate the whole sprite list. Sprite-on-sprite collisions, eg? ...win back about 1% with physics completely stubbed.
- - - There's room for improvement in here, eg a separate index of physics-relevant sprites. But I think that's more effort than the gain is worth.
- - OK I don't see any easy wins here, so let's live with the ~7%
- [ ] Fill the tilesheet with terrain and decorative graphics, and place them all around.
- [ ] Something more interesting for the background graphics? Flat blue sky isn't bad, but maybe some clouds or birds or a distant landscape that scrolls parallax?
- [ ] Decorative bees flying from flower to flower?
- [ ] User reports a black screen after the menu: Win11 and Chrome 135.0.7049.96
- [ ] Re-upload to Itch with audio fix, after jam closes.
