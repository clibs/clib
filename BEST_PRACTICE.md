# Clib Examples and Best Practice

This page will cover:

 - [How to use libraries](#how-to-use-installed-libraries-for-your-project).
 - [Example Makefile](#example-makefile).
 - [Example `package.json` for executables](#example-packagejson-for-executable-project).
 - [Making your own library package](#making-your-own-libraries).
 - [Example `package.json` for libraries](#example-packagejson-for-libraries).
 - [How to install/uninstall executables](#install-and-uninstall-executables-packages).

For instructions on installation, check out the [README](https://github.com/clibs/clib#installation).

## How to use installed libraries for your project

Lets say you have your project, with a typical directory tree like this:

```
your-project/
│
├── deps/
│   ├── trim.c/
│   │   ├── trim.h
│   │   ├── trim.c
│   │   └── package.json
│   │
│   ├── commander/
│   │   ├─ commander.h
│   │   ├─ commander.c
│   │   └─ package.json
│   │
│   └── logger/
│       ├── logger.h
│       ├── logger.c
│       └── package.json
│
├── LICENSE
│
├── Makefile
│
├── README.md
│
├── package.json
│
└── src/
    ├── main.c
    ├── function.c
    ├── function.h
    └── etc...
```

`src` is the directory where you source code is in. And `deps` is the directory where you libraries will be downloaded when you run `clib install <username/library>`.

Knowing all of that, lets have a look at an example Makefile.

### Example Makefile

```makefile
# your c compiler
CC = gcc

# where to install
PREFIX = /usr/local/bin

# your project name
TARGET = your-project

CFLAGS = -Ideps -Wall

# all the source files
SRC = $(wildcard src/*.c)
SRC += $(wildcard deps/*/*.c)

OBJS = $(SRC:.c=.o)

.PHONY:
all: $(TARGET)

.PHONY:
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS)

.PHONY:
%.o: %.c
	$(CC) $(DEP_FLAG) $(CFLAGS) $(LDFLAGS) -o $@ -c $<

.PHONY:
clean:
	rm -f $(OBJS)

.PHONY:
install: $(TARGET)
	cp -f $(TARGET) $(PREFIX)

.PHONY:
uninstall: $(PREFIX)/$(TARGET)
	rm -f $(PREFIX)/$(TARGET)
```

This is a basic Makefile, and should work for most of your projects.

You *could* have your Makefile install the libraries upon running it, but you
would only need to do that to get the latest version of the library(s), in this
case you probably don't want that. You typically want yo get the latest stable version
for that library. By having a `package.json` file in your project repo, you can
specify what packages you need, and what version of that package. Now have a look
at a example `package.json` file for your project: (executable package)

### Example package.json for executable project

```json
{
  "name": "executable-name",
  "version": "1.0.0",
  "repo": "your-github-name/project-name",
  "dependencies": {
    "stephenmathieson/trim.c": "0.0.2",
    "clibs/commander": "1.3.2",
    "clibs/logger": "0.0.1",
  },
  "install": "make install",
  "uninstall": "make uninstall"
}
```

Starting from the top, `"name"` is your package name. `"version"` is your package version. `"repo"` is the location of your project, (not including the `https://github.com/`). `"dependencies"` is all the dependencies your project requires, along with there version. `"install"` is the command to install your program (ran as root), (tip: if your project requires more then one command to install it, like need to run `./configure`, before `make`, then do this: `"install": "./configure && make && make install"`). `"uninstall"` is the command to uninstall your project, [more on that later](#install-and-uninstall-executables).
 
_**NOTE:** Make sure you have a relese as the same version in your `package.json` file, otherwise the download will fail. If you always want your package at the latest version, then put `master` as your version._

## Making your own libraries

Now that your know how to use libraries, heres how to make your own:

Like before, heres a typical project directory tree:

```
your-library-c/
│
├── deps/
│   ├── path-join.c/
│   │   ├── path-join.h
│   │   ├── path-join.c
│   │   └── package.json
│   │
│   └── strdup/
│       ├─ strdup.h
│       ├─ strdup.c
│       └─ package.json
│
├── LICENSE
│
├── Makefile
│
├── README.md
│
├── package.json
│
├── src/
│   ├── library.c
│   ├── library.h
│   └── etc...
│
└── test.sh
```

Also like before, your have a `deps` directory (depending on your library, you may not need any
dependencies). Your `Makefile` in this case it is only for the `test.sh`, not needed for installing.
`package.json` contains your library name, dependencies (if you need them), keywords, etc... In
`src/` contains your make code (usally the same name as your library). And you have your `test.sh`
used for testing.

### Example package.json for libraries

```
{
  "name": "your-lib-name",
  "version": "1.0.0",
  "repo": "your-github-name/library-name",
  "description": "What my library does",
  "keywords": [
    "somthing",
    "cool",
    "mylib",
  ],
  "dependencies": {
    "stephenmathieson/path-join.c": "0.0.6",
    "clibs/strdup": "*"
  }
  "license": "YOUR_LIB LICENSE",
  "src": [
    "src/library.c",
    "src/library.h"
  ],
}
```

The main differences (between this, and the executable `package.json`), is now there is `"src"`,
this is where your make library source code is, your can change it, but src is petty standard.

**TIP:** In the `"dependencies"` section, if you define `"*"` as the version, clib will install
the latest version of that library.

_**NOTE:** Just like your executable package, you will want to tag a relese with the same name
as your version specified in your `package.json`._

## Install and uninstall executables packages

Installing executables is best done as root (with `sudo`), here is a typical install command:

```bash
$ sudo clib install visionmedia/mon
```

To uninstall a package, (as of today) your need to install `clib-uninstall`:

```bash
$ sudo clib install clib-uninstall
```

_**TIP:** If you don't specify a username when installing a package or library, clib will
download that package in the default location: `https://github.com/clibs/`._

After you install `clib-uninstall` you can use it like so:

```bash
$ sudo clib-uninstall <username/package-name>

# for example:
$ sudo clib-uninstall visionmedia/mon
```

<br>
