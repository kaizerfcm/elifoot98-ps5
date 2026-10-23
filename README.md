# Elifoot 98

PS5 homebrew (`PPSA99098`) that boots the Windows 3.1 game Elifoot 98 inside DOSBox Pure. The game disk is packaged at `/app0/assets/elifoot98` and is not stored in git. Options opens the launcher. Create types text.

The frontend and DOSBox Pure are GPL-2.0-or-later. The build system is GPL-3.0-or-later, so the combined title is GPL-3.0-or-later. See [LICENSE](LICENSE).

## Build and install

```bash
export ELIFOOT_DATA=/path/to/elifoot98-data   # optional; a sibling extract is detected
make
```

`make` writes `dist/PPSA99098.zip`. Install that `PPSA99098` folder with [ProsperoStore](https://prosperostore.net/) / ShadowMountPlus. The title does not use `/download0`.

## Play

- D-pad or left stick moves. Cross confirms. Circle goes back.
- Square is the game's `A`. Triangle is `M`. L1 and R1 are the keypad `*` and `-` shortcuts.
- Right stick moves the mouse. R2 left-clicks. L2 right-clicks.
- Options opens the launcher: saves, USB patch zips, and cheats. Create opens the on-screen keyboard.
- Saves are the `.e98` files in `D:\ELI98\JOGOS`. Wipe restores `hdd.img` from `hdd.img.base`.

## Known issues

The game disk has to be supplied at build time. Boot and controller response need a real PS5 to confirm: the title should reach the Elifoot menu, not stop at `Unable to change to: ELI98`, and menu keys should answer on the next frame.
