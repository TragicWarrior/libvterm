# Compatibility Testing #

The following programs have been tested with libvterm.  The score
indicates the level of rendering fidelity.  If you've tested
other programs with libvterm, feel free to update this list.

---
Fidelity score: 5 = excellent, 1 = poor
---

### Linux Testing ###

Test Versions:

Xubuntu 16.04 LTS, 18.04 LTS, 19.04
Debian 10 (buster)
Fedora 29, Fedora 30
Centos 7.8

```
5 - hexedit
5 - cdw
5 - htop
5 - apt
5 - yum
5 - bastet
5 - myman
5 - last file manager
5 - ranger
5 - midnight commander (see notes)
5 - nano
4 - links
5 - lynx
4 - finch
5 - network manager (nmtui)
5 - alsamixer
5 - cmatrix
5 - aewan
5 - cboard
5 - dstat
5 - ninvaders
5 - mutt
5 - weechat
5 - tig
5 - iptraf-ng
5 - aaxine
2 - cacaxine
4 - ponysay
5 - cacafire
5 - cacademo
5 - pspg
5 - calcurse
4 - cacaview
5 - partimage
5 - asciijump
5 - vlc
5 - pamix
5 - man
5 - test-colorcube (ncurses test)
5 - yetris
5 - nsnake
```

### Mac OS Testing ###

Test Versions:

Mojave 10.14.5
Mojave 10.14.6

```
5 - htop
5 - vitetris
4 - homebrew
5 - mdcat
5 - midnight commander
4 - ponysay
5 - ninvaders
5 - emojify
5 - lynx
4 - links
5 - mutt
3 - myman
5 - ranger
5 - cmatrix
5 - nano
5 - man
5 - tig
5 - bastet
5 - test-colorcube (ncurses test)
5 - bmon
```

### FreeBSD Testing ###

Test Versions:

FreeBSD 12.0

```
5 - htop
3 - mc
5 - man
5 - nano
5 - tig
5 - bastet
4 - lynx
5 - hexedit
5 - calcurse
5 - cmatrix
```

# Test Notes #


### VLC ###

Testing Configuration:

```
vlc -V aa <file>
vlc -V caca <file>
```

### Mplayer ###

Testing Configuration:

```
mplayer -quiet -framedrop -monitorpixelaspect 0.5 -contrast 2 -vo aa:driver=curses
```

### myman ###

Testing Configuration:

```
myman -z big
myman -2 -z big
```

### aaxine ### 

Testing Configuration:

```
aaxine -reverse -dim -bold -A alsa <file>
```

### Midnight Commander ###

Notes:

When running in "linux" terminal emulation mode, the version of mc which
ships with Ubuntu 18.04 LTS buffers keystrokes when mouse mode is enabled.
Upgrading to a newer version of mc or staring in no mouse mode (-d) will
alleviate the problem.


### cacaxine ###

Notes:

This is a notoriously buggy player.  It often spews out characters that an
emulator can mistake for legitimate escape sequences.  For this reason
it makes for a brutal test on the parsing engine.  As of 2019-08-08,
playback will cause libvte based emulators (xfce terminal, gnome terminal)
to crash because of CSI SU sequences which are out of bounds.  A safety
check makes libvterm less susceptible to it.  Both xterm and the KDE
konsole emulator also seem to be resistant to the bug.
