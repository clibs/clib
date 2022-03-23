# Building clib from source

## OSx

```sh
$ git clone https://github.com/clibs/clib.git /tmp/clib
$ cd /tmp/clib
$ make install
```

## Ubuntu or debian

```sh
# install libcurl
$ sudo apt install libcurl4-gnutls-dev -qq
# clone
$ git clone https://github.com/clibs/clib.git /tmp/clib && cd /tmp/clib
# build
$ make
# put on path
$ sudo make install
```

## Windows (crosscompiling from linux)
The docker image contains the mingw toolchain which is used to compile for windows.
Curl needs to be built from source.
```shell
# Download and compile curl
$ docker run --rm dockcross/windows-static-64-posix > dockcross-windows-x64
$ cat dockcross-windows-x64
$ chmod +x dockcross-windows-x64
$ wget https://curl.haxx.se/download/curl-7.76.0.tar.gz
$ tar xzf curl-*
$ CURL_SRC=curl-*
$ ./dockcross-windows-x64 bash -c 'cd '"$CURL_SRC"' && ./configure --prefix="/work/deps/curl" --host=x86_64-w64-mingw32.static --with-winssl --disable-dependency-tracking --disable-pthreads --enable-threaded-resolver --disable-imap --disable-pop3 --disable-smpt --disable-ldap --disable-mqtt --disable-smb'
$ ./dockcross-windows-x64 bash -c 'cd '"$CURL_SRC"' && make'
$ ./dockcross-windows-x64 bash -c 'cd '"$CURL_SRC"' && make install'
$ git clone https://github.com/clibs/clib.git && cd clib
$ ./dockcross-windows-x64 make all NO_PTHREADS=1 EXE=true
```