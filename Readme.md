# clib(1)

  [![Build Status](https://travis-ci.org/clibs/clib.svg?branch=master)](https://travis-ci.org/clibs/clib)
  [![Codacy Badge](https://app.codacy.com/project/badge/Grade/a196ec36c31349e18b6e4036eab1d02c)](https://www.codacy.com/gh/clibs/clib?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=clibs/clib&amp;utm_campaign=Badge_Grade)

  Package manager for the C programming language.

  ![c package manager screenshot](https://i.cloudup.com/GwqOU2hh9Y.png)

## About

Basically the lazy-man's copy/paste promoting smaller C utilities, also
serving as a nice way to discover these sort of libraries. From my experience
C libraries are scattered all over the web and discovery is relatively poor. The footprint of these libraries is usually quite large and unfocused. The goal of `clibs` is to provide
stand-alone "micro" C libraries for developers to quickly install without coupling
to large frameworks.

You should use `clib(1)` to fetch these files for you and check them into your repository, the end-user and contributors should not require having `clib(1)` installed. This allows `clib(1)` to fit into any new or existing C workflow without friction.

The [listing of packages](https://github.com/clibs/clib/wiki/Packages) acts as the "registry". The registry is used by `clib(1)` when searching for packages.

## Installation and building
Binaries for `clib(1)` releases can be found at [releases](https://github.com/clibs/clib/releases/).
For OSx and linux [libcurl](http://curl.haxx.se/libcurl/) should be installed and linkable.
The windows binaries do not require any libraries to be installed.

See [Building](Building.md) for instructions on how to build clib.


## Usage
More detailed information on how to use `clib` can be found in [Usage](Usage.md).
```
  clib <command> [options]

  Options:

    -h, --help     Output this message
    -V, --version  Output version information

  Commands:

    init                 Start a new project
    i, install [name...] Install one or more packages
    up, update [name...] Update one or more packages
    upgrade [version]    Upgrade clib to a specified or latest version\
    configure [name...]  Configure one or more packages
    build [name...]      Build one or more packages
    search [query]       Search for packages
    help <cmd>           Display help for cmd
```
More information about the Command Line Interface can be found [here](https://github.com/clibs/clib/wiki/Command-Line-Interface).

### Example usage
 Install a few dependencies to `./deps`:

```sh
$ clib install clibs/ms clibs/commander
```

 Install them to `./src` instead:

```sh
$ clib install clibs/ms clibs/commander -o src
```

 When installing libraries from the `clibs` org you can omit the name:

```sh
$ clib install ms file hash
```

 Install some executables:

```sh
$ clib install visionmedia/mon visionmedia/every visionmedia/watch
```

## Clib.json
Information about a clib project or a package is stored in `clib.json`.
In a project that uses `clib` to install dependencies a typical `clib.json` will only contain the required dependencies.
It may look something like:
```json
{
  "name": "Copy and Paste-Inator",
  "version": "0.4.2",
  "description": "Creates copies of yourself to do all of his waiting in lines for you.",
  "dependencies": {
    "clibs/buffer": "0.0.1",
    "clibs/term": "0.0.1",
    "jwerle/throw.h": "0.0.0"
  }
}
```

Packages that can be installed by `clib` should also provide a `clib.json`.
It contains the files that should be installed by `clib` in `"src"`
An example of a clib.json for a package may look like:
```json
{
  "name": "term",
  "version": "0.0.1",
  "repo": "clibs/term",
  "description": "Terminal ansi escape goodies",
  "keywords": ["terminal", "term", "tty", "ansi", "escape", "colors", "console"],
  "license": "MIT",
  "src": ["src/term.c", "src/term.h"]
}
```

Example of a clib.json for an executable:

```json
{
  "name": "mon",
  "version": "1.1.1",
  "repo": "visionmedia/mon",
  "description": "Simple process monitoring",
  "keywords": ["process", "monitoring", "monitor", "availability"],
  "license": "MIT",
  "install": "make install"
}
```

 See [explanation of clib.json](https://github.com/clibs/clib/wiki/Explanation-of-clib.json) for more details.

## Contributing

 If you're interested in being part of this initiative let me know and I'll add you to the `clibs` organization so you can create repos here and contribute to existing ones.
 
 If you have any issues, questions or suggestions, please open an issue [here](https://github.com/clibs/clib/issues). 
 
 You can also find us on Gitter: https://gitter.im/clibs/clib
 
 Also feel free to open a GitHub Discussion [here](https://github.com/clibs/clib/discussions).

 Before committing to the repository, please run `make commit-hook`. This installs a commit hook which formats `.c` and `.h` files.

## Articles

  - [Introducing Clib](https://medium.com/code-adventures/b32e6e769cb3) - introduction to clib
  - [The Advent of Clib: the C Package Manager](https://web.archive.org/web/20200128184218/http://blog.ashworth.in/2014/10/19/the-advent-of-clib-the-c-package-manager.html) - overview article about clib
