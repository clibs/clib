
# cpm

  Package manager for the C programming language.

  ![c package manager screenshot](http://f.cl.ly/items/0u1k3G0e1U0f1Q411e3N/cpm.png)

## Installation

  Using "c-pm" as "cpm" is taken in npm...

```
$ npm install -g c-pm
```

## About

  Basically the lazy-man's copy / paste, promoting smaller C utilities, also
  serving as a nice way to discover these sort of libraries, from my experience
  they are scattered all over the web and discovery is relatively poor. The wiki [listing of packages](https://github.com/visionmedia/cpm/wiki/Packages) acts as the "registry" and populates the `cpm-search(1)` results.

  `cpm(1)` is not a package manager for an end-user experience, for example it currently
  has no concept of dependencies etc, you should use `cpm(1)` to fetch these files for you
  and check them into your repository, the end-user and contributors will not require `cpm(1)`.

## Usage

```

Usage: cpm [options] [command]

Commands:

  install <pkg>          install the given package(s)
  help [cmd]             display help for [cmd]

Options:

  -h, --help     output usage information
  -V, --version  output the version number

```

## Examples

 Install a few dependencies to `./deps`:

```
$ cpm install visionmedia/ms.c visionmedia/commander.c
```

 Install them to `./src` instead:

```
$ cpm install visionmedia/ms.c visionmedia/commander.c -o src
```

 Install some executables:

```
$ cpm install visionmedia/mon visionmedia/every visionmedia/watch
```

  Once again with brace expansion, you do love brace expansion right? ;)

```
$ cpm install visionmedia/{mon,every,watch}
```

## package.json

 Example of a package.json explicitly listing the source:

```json
{
  "name": "term",
  "version": "0.0.1",
  "repo": "visionmedia/term.c",
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

## License 

  MIT
