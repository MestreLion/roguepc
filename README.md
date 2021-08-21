ROGUEPC
===============================================================================

### Original DOS Epyx Rogue (1985) ported to modern PCs!

![Title Screen](screenshots/title.png)![Level 1, fully explored](screenshots/level1.png)

<sup>*(Yes, these are actual in-game screenshots using `xterm` with the custom font)*</sup>

- Uses original source code from 1985, adapted to compile in modern PCs.
    - All comments from original authors preserved.
    - All changes needed for portability from 16-bit DOS fully marked and explained.
    - Extensive documentation on original code inner mechanics:
      hardware access, BIOS and DOS INT calls, clock and timers, CGA/EGA graphics usage,
      piracy copy protection, splash image, assembly routines, builtin `curses`, etc.
    - Great care taken to preserve as much as possible from original. More than 85% still intact!
- Strictly adheres to C standards to be fully portable to any modern platform.
    - [ISO/IEC C17/C18](https://en.wikipedia.org/wiki/C17_%28C_standard_revision%29), latest ISO C.
    - [POSIX.1-2017](https://pubs.opengroup.org/onlinepubs/9699919799/), latest revision of POSIX 2008.
    - [X/Open Curses, Issue 7](https://publications.opengroup.org/c094), latest Curses specification.
    - [Single UNIX Specification, Version 4 (SUSv4)](https://unix.org/version4/), includes POSIX and Curses.
- Runs in any terminal, text-only, using nothing but `curses`, just like the original.
- Colors and gameplay intact.
- Non-ASCII characters updaded from DOS CP437 to suitable Unicode codepoints.

![Game over screen](screenshots/rip.png)
<sup>*(Better get used to, you'll see this... a **lot**)*</sup>

For latest Debian / Ubuntu systems:
```sh
sudo apt install make pkg-config libncurses-dev
cd src
make
./rogue
```

To enable the original splash screen in graphics mode (requires SDL2):
```sh
sudo apt install libsdl2-dev  # plus above requirements (make, ncurses, etc)
cd src
make all
./rogue-sdl
```

For ASCII mode (like UNIX Rogue, but with colors):
```sh
cd src
make ROGUE_CHARSET=1
./rogue
```

For Ubuntu 18.04, use `libncursesw5-dev` instead of `libncurses-dev`, or
`libncurses5-dev` for ASCII mode.


Strongly suggested:

- Install the custom `PerfectDOSVGA437Unicode.ttf` font. See [tools](tools/)
- Launch the game using [`./roguepc`](roguepc).
  It will auto-select either `rogue-sdl` or `rogue`, and also choose
  `roguepc-xterm`, `roguepc-gnome-terminal` or none depending on what is
  available in your system.
- If your desktop is 1920x1200 or larger, try `./roguepc --fullscreen -fs 36`
  for the ultimate rogue experience!

![splash image](rogue.png)

<sup>*(Yes, it will display the original splash if you compile with SDL2! \o/)*</sup>
