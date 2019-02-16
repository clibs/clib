# Clib Examples and Best Practice

This page will cover:

 - (How to use libraries)[#how-to-use-installed-librarys].
 - (Example Makefile)[#example-makefile].
 - (Example `package.json` for executables)[#example-package.json-for-exacutables].
 - (Making your own library package)[#making-your-own-libraries].
 - (Example `package.json` for libraries)[#example-package.json-for-libraries].
 - (How to install/uninstall executables)[#install-and-uninstall-executables].

For instructions on installation, click [here]().

<br>

## How to use installed libraries:

Lets say you have your project, with a typical directory tree like this:

```
your-project-c/
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
├── src/
│   ├── main.c
│   ├── function.c
│   └── function.h
│
└── test.sh
```

`test.sh` is your test script.<br>
`src` is the directory where you source code is in. And<br>
`deps` is the directory where you libraries will be downloaded when you run `clib install <username/library>`.

Knowing all of that, lets have a look at an example Makefile.

## Example Makefile:

```makefile

# your c compiler
CC = gcc

# where to install
PREFIX = /usr/local/bin

# your output file
TARGET = project-name

# locate all the c files
SRC  = $(wildcard src/*.c)
DEPS = $(wildcard deps/*/*.c)

# get all c file names, then replace .c to .o
# and store that in OBJS
ALLFILE = $(nodir $(SRC) $(DEPS))
OBJS = $(ALLFILE:.c=.o)

.PHONY:
all: $(TARGET)

# compile all object files into TARGET
.PHONY:
$(TARGET): $(OBJS)
	$(CC) $(CFLAG) -o $(TARGET) $(OBJS) $(LDFLAGS)

# loop thrught all c files, turning them into object files
.PHONY:
$(OBJS): $(SRC) $(DEPS)
	$(foreach srcfile, $(SRC), $(CC) $(CFLAGS) -c $(srcfile);)
	$(foreach depfile, $(DEP), $(CC) $(CFLAGS) -c $(depfile);)

# install TARGET to PREFIX
.PHONY:
install: $(BINS)
	cp -f $(TARGET) $(PREFIX)

# clean, remove all .o (object files)
.PHONY:
clean:
	rm -f *.o

# uninstall TARGET from PREFIX
.PHONY:
uninstall:
	rm -f $(PREFIX)/$(TARGET)

# run your tests
.PHONY:
test:
	@./test.sh
```

This is a basic Makefile, and should work for most of your projects.

You *could* have your Makefile install the libraries upon running it, but you
would only need to do that to get the latest version of that library, in this
case you proboly don't want that. You typicly want yo get the lates stable version
for that library. By having a `package.json` file in your project repo, you can
specify what packages you need, and what version of that package. Now have a look
at a example `package.json` file for your project: (executable package)

### Example package.json for exacutables:

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

Starting from the top, `"name"` is your package name. `"version"` is your package version.
`"repo"` is the location of your project, (not including the `https://github.com/`).
`"dependencies"` is all the dependencies your project requiers, along with there version.
`"install"` is the command to install your program (ran as root), (tip: if your project
requiers more then one command to install it, like need to run `./configure`, before `make`,
then do this: `"install": "./configure && make && make install"`). `"uninstall"` is the command
to uninstall your project, more on that later.

## Making your own libraries:

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
│   └── library.h
│
└── test.sh
```

Also like before, your have a `deps` directory (depending on your library, you may not need any
dependencies). Your `Makefile` in this case it is only for the `test.sh`, not needed for installing.
`package.json` contains your library name, dependencies (if you need them), keywords, etc... In
`src/` contains your make code (usally the same name as your library). And you have your `test.sh`
used for testing.

#### Example package.json for libraries:

```json
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

The main difrances (between this, and the executable `package.json`), is now there is `"src"`,
this is where your make library source code is, your can change it, but src is pritty standard.

**TIP:** In the `"dependencies"` section, if you define `"*"` as the version, clib will install
the latest version of that library.

## Install and uninstall executables:

Installing executables is best done as root (with `sudo`), here is a typical install command:

```bash
$ clib install visionmedia/mon
```

To uninstall a package, (as of today) your need to install `clib-uninstall`:

```bash
$ clib install clib-uninstall
```

***TIP:** If you don't specify a username when installing a package or library, clib will
download that package in the default location: `https://github.com/clibs/`.*

After you install `clib-uninstall` you can use it like so:

```bash
$ clib-uninstall <username/package-name>

# for example:
$ clib-uninstall visionmedia/mon
```

<br>
