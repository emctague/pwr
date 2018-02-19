# Pwr

This is a program designed for laptops. It can easily switch between power-saving and performance modes using
simple, shorthand commands. It will also restart the current display manager unless the `-n` flag is selected, allowing
for all changes to take effect.

## What it switches (if available):

* CPU Performance Governor
* NVIDIA PRIME GPU (`intel` vs. `nvidia`)
* Wireless card power-saving state

## Usage

To switch into Performance mode:

```
pwr perform
```

To switch to power-saving mode:

```
pwr powersave
```

You can also use the shorthand `pe` for performance and `ps` for power-saving.

If you don't want your display manager to restart, add the `-n` flag on the end of the command.

## Building and Installing

Ensure you have `git` and `cmake` installed, as well as an appropriate C++ compiler such as `g++`.

```
git clone https://github.com/emctague/pwr
cd pwr
mkdir build
cd build
cmake ..
make
make install
```

## Copyright and License

Copyright 2018 Ethan McTague.

This program is licensed under the MIT license. See LICENSE for full license text.
