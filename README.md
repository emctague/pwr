# Pwr

This is a program designed for laptops. It can easily switch between power-saving and performance modes using
simple, shorthand commands. It will also restart the current display manager unless the `-n` flag is selected, allowing
for all changes to take effect.

## What it switches (if available):

* CPU Performance Governor (`powersave` vs. `performance`)
* NVIDIA PRIME GPU (`intel` vs. `nvidia`)
* Wireless card power-saving state (`on` vs. `off`)

## Usage

To switch into Performance mode:

```
pwr perform
```

To switch to power-saving mode:

```
pwr powersave
```

To toggle:

```
pwr toggle
```

And to check the current state (gives `perform` or `powersave`):

```
pwr query
```

You can also use the shorthand `pe` for performance, `ps` for power-saving, `to` for
toggle, and `qu` for query.

If you don't want your display manager to restart, add the `-n` flag on the end of the command.

## Building and Installing

### From Binary Releases

Check out the [Releases](https://github.com/emctague/pwr/releases).
You can either run the `.sh`-based installer (a self-extracting archive) as root, *or* unpack the .tar.gz file and install its contents manually. Be aware that the installer
and tarball don't automatically set the script as `setuid` - you may have to add the `+s`
flag manually.

### From Source

Ensure you have `git` and `cmake` installed, as well as an appropriate C compiler such as `gcc`.

```
git clone https://github.com/emctague/pwr
cd pwr
mkdir build
cd build
cmake ..
make
make install
```

You can also download a tarball of the latest release and use that
instead of the git repo.

## Copyright and License

Copyright 2018 Ethan McTague.

This program is licensed under the MIT license. See LICENSE for full license text.
