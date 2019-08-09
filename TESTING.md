# Compatibility Testing #

The following programs have been tested with libvterm.  The score
indicates the level of rendering fidelity.  If you've tested
other programs with libvterm, feel free to update this list.

---
Fidelity score: 5 = excellent, 1 = poor
---

### Linux Testing ###

```
5 - hexedit
5 - cdw
5 - htop
5 - apt
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
```

### Mac OS Testing ###

```
5 - htop
5 - vitetris
4 - homebrew
5 - mdcat
5 - midnight commander
5 - ponysay
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
```

### FreeBSD Testing ###

```
5 - htop
5 - mc
5 - man
5 - nano
5 - tig
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
check in libvterm prevents it.  Both xterm and the  KDE konsole emulator
seems to be resistant to the bug.
