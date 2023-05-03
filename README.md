# Welcome to the Griddle Engine

## Griddle Engine is an id Tech 1 centric source port for Waffle Iron Studios, Ltd.'s games

**Griddle** is a GZDoom fork that aims to better serve Waffle Iron Studio's freeware and commercial games.

While GZDoom is an advanced Doom engine and does this very well, Griddle aims to further separate itself.

This means adding features that may not align with GZDoom's goals, and discarding backwards compatibility with previous iterations of the engines, and existing mods, while still offering it's very capable core featureset.

## Griddle Features

### Bespoke™ to Griddle
* Level flag keyword for defining on-keypress/buttonpress skippable cutscene levels
* Level flag keyword for stopping total timer the level (cutscene levels do this too)
* Level flag keyword for disabling autosaves on a level entirely (cutscene levels do this too)
* Fine tuning of Randomized ActiveSound playing during A_Chase routine
* Software Mode is disabled
* Better Defaults (including more tight air control)
* Different save directory/user config/etc handling
* Menus use game's smallfont and not that fixedsys looking stuff
* Other minor quality of life fixes

### From PRs not yet merged into GZDoom upstream
* Some objects can be marked as transient (not saved in savestates)
* Pitch correction fixes

### From vkDoom
* Removal of OpenGL ES Backend
* hitbox debugging feature
* Rotating models are paused when game is paused/menus are open
* Level flag keyword for disabiling automap
* Level flag keyword for disabling user-initiated saves
* Even simpler menu layout/organization for default menus

_**More features are planned.**_

Please see license files for individual contributor licenses.

### Licensed under the GPL v3
##### https://www.gnu.org/licenses/quick-guide-gplv3.en.html
---
