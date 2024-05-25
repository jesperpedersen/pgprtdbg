This tutorial will show you how to compile and install pgmoneta on various platforms.

Currently, primarily supported platforms covered in this tutorial are:

1. Fedora (37, 38)
2. Rocky (8.x, 9.x)
3. RHEL (8.x, 9.x)
4. FreeBSD 14

## Requirements
`pgmoneta` requires:

* [gcc 8+](https://gcc.gnu.org) (C17)
* [cmake](https://cmake.org)
* [make](https://www.gnu.org/software/make/)
* [libev](http://software.schmorp.de/pkg/libev.html)

On Fedora, these can be installed using `dnf` or `yum`:
```
dnf install gcc cmake make libev libev-devel
```

## Compile
Compiling pgmoneta on different platforms is the same.

### Release build

The following commands will build and install `pgprtdbg` in the `/usr/local` hierarchy.

```sh
git clone https://github.com/jesperpedersen/pgprtdbg.git
cd pgprtdbg
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

### Debug build

The following commands will create a `DEBUG` version of `pgprtdbg`.

```sh
git clone https://github.com/jesperpedersen/pgprtdbg.git
cd pgprtdbg
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

Remember to set the `log_level` configuration option to `debug5`.
