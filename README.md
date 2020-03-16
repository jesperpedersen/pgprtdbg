# pgprtdbg

`pgprtdbg` is an application that turns [PostgreSQL](https://www.postgresql.org) protocol v3 interaction into a trace file.

See [Getting Started](./doc/GETTING_STARTED.md) on how to get started with `pgprtdbg`.

See [Configuration](./doc/CONFIGURATION.md) on how to configure `pgprtdbg`.

## Tested platforms

* [Fedora](https://getfedora.org/) 28+
* [RHEL](https://www.redhat.com/en/technologies/linux-platforms/enterprise-linux) 7.x with
  [EPEL](https://access.redhat.com/solutions/3358) and
  [DevTools](https://developers.redhat.com/products/developertoolset/overview) 8+
* [RHEL](https://www.redhat.com/en/technologies/linux-platforms/enterprise-linux) 8.x with
  [AppStream](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/installing_managing_and_removing_user-space_components/using-appstream_using-appstream)

## Compiling the source

`pgprtdbg` requires

* [gcc 8+](https://gcc.gnu.org) (C17)
* [cmake](https://cmake.org)
* [make](https://www.gnu.org/software/make/)
* [libev](http://software.schmorp.de/pkg/libev.html)

```sh
dnf install gcc cmake make libev libev-devel
```

Alternative [clang 8+](https://clang.llvm.org/) can be used.

### Build

The following commands will install `pgprtdbg` in the `/usr/local` hierarchy
and run the default configuration.

```sh
git clone https://github.com/jesperpedersen/pgprtdbg.git
cd pgprtdbg
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
/usr/local/bin/pgprtdbg -c /usr/local/etc/pgprtdbg.conf
```

See [RPM](./doc/RPM.md) for how to build a RPM of `pgprtdbg`.

## Contributing

Contributions to `pgprtdbg` are managed on [GitHub.com](https://github.com/jesperpedersen/pgprtdbg/)

* [Ask a question](https://github.com/jesperpedersen/pgprtdbg/issues)
* [Raise an issue](https://github.com/jesperpedersen/pgprtdbg/issues)
* [Feature request](https://github.com/jesperpedersen/pgprtdbg/issues)
* [Code submission](https://github.com/jesperpedersen/pgprtdbg/pulls)

Contributions are most welcome !

## License

[BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause)
