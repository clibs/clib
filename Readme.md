
# clib(1)

  Package manager for the C programming language.

  ![c package manager screenshot](https://i.cloudup.com/GwqOU2hh9Y.png)

## Installation

```
$ git clone https://github.com/clibs/clib.git /tmp/clib
$ cd /tmp/clib
$ make install
```

## About

  Basically the lazy-man's copy/paste promoting smaller C utilities, also
  serving as a nice way to discover these sort of libraries. From my experience
  C libraries are scattered all over the web and discovery is relatively poor. The footprint of these libraries is usually quite large and unfocused. The goal of `clibs` is to provide
  stand-alone "micro" C libraries for developers to quickly install without coupling
  to large frameworks.

  You should use `clib(1)` to fetch these files for you and check them into your repository, the end-user and contributors should not require having `clib(1)` installed. This allows clib(1) to fit into any new or existing C workflow without friction.

  The wiki [listing of packages](https://github.com/clibs/clib/wiki/Packages) acts as the "registry" and populates the `clib-search(1)` results.

## Usage

```
  Usage: clib [options] [command]

  Commands:

    install <pkg>          install the given package(s)
    search [query]         search available packages
    help [cmd]             display help for [cmd]

  Options:

    -h, --help     output usage information
    -V, --version  output the version number
```

## Examples

 Install a few dependencies to `./deps`:

```
$ clib install clibs/ms clibs/commander
```

 Install them to `./src` instead:

```
$ clib install clibs/ms clibs/commander -o src
```

 When installing libraries from the `clibs` org you can omit the name:

```
$ clib install ms file hash
```

 Install some executables:

```
$ clib install visionmedia/mon visionmedia/every visionmedia/watch
```

  Once again with brace expansion, you do love brace expansion right? ;)

```
$ clib install visionmedia/{mon,every,watch}
```

## package.json

 Example of a package.json explicitly listing the source:

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

 Example of a package.json for an executable:

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

## Contributing

 If you're interested in being part of this initiative let me know and I'll add you to the `clibs` organization so you can create repos here and contribute to existing ones.

## Articles

  - [Introducing Clib](https://medium.com/code-adventures/b32e6e769cb3) - introduction to clib

## License 

  MIT
