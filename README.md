# center-windows
Places new windows at the center of its monitor working area.

Listens for Ctr-Alt-C and centers an active window.

You will want both 64 and 32 bit releases if you are running 64 bit and 32 bit applications on 64bit Windows.

Loaders accept command line as guide to what hooks to install:
* `keyboard-only` to install only keyboard hook (the one responsible for handling ctrl-alt-c keypress)
* `shell-only` to install only shell hook (it will attempt center windows upon their creation)

So, if you are running 64bit Windows, you'll be best served with setup similar to mine:
* Run loader64 without args to handle both keyboard and start of 64bit applications,
* Run loader32 with `shell-only` to handle only start of 32bit applications.

Here is my config in one picture (Windows 10):
![My configuration](https://github.com/itsuart/center-windows/raw/master/my-hooks.png)
