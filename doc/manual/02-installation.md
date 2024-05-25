\newpage

# Installation

## Fedora

You need to add the [PostgreSQL YUM repository](https://yum.postgresql.org/), for example for Fedora 40

```
dnf install -y https://download.postgresql.org/pub/repos/yum/reporpms/F-40-x86_64/pgdg-fedora-repo-latest.noarch.rpm
```

and do the install via

```
dnf install -y pgprtdbg
```

Additional information

* [PostgreSQL YUM](https://yum.postgresql.org/howto/)
* [Linux downloads](https://www.postgresql.org/download/linux/redhat/)

## RHEL 8 / RockyLinux 8

```
dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
dnf install -y https://download.postgresql.org/pub/repos/yum/reporpms/EL-8-x86_64/pgdg-redhat-repo-latest.noarch.rpm
```

and do the install via

```
dnf install -y pgprtdbg
```

## RHEL 9 / RockyLinux 9

```
dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm
dnf install -y https://download.postgresql.org/pub/repos/yum/reporpms/EL-9-x86_64/pgdg-redhat-repo-latest.noarch.rpm
```

and do the install via

```
dnf install -y pgprtdbg
```

## Compiling the source

We recommend using Fedora to test and run [**pgprtdbg**][pgprtdbg], but other Linux systems, FreeBSD and MacOS are also supported.

[**pgprtdbg**][pgprtdbg] requires

* [gcc 8+](https://gcc.gnu.org) (C17)
* [cmake](https://cmake.org)
* [make](https://www.gnu.org/software/make/)
* [libev](http://software.schmorp.de/pkg/libev.html)

```sh
dnf install git gcc cmake make libev libev-devel
```

Alternative [clang 8+](https://clang.llvm.org/) can be used.


### RHEL / RockyLinux

On RHEL / Rocky, before you install the required packages some additional repositories need to be enabled or installed first.

First you need to install the subscription-manager

``` sh
dnf install subscription-manager
```

It is ok to disregard the registration and subscription warning.

Otherwise, if you have a Red Hat corporate account (you need to specify the company/organization name in your account), you can register using

``` sh
subscription-manager register --username <your-account-email-or-login> --password <your-password> --auto-attach
```

Then install the EPEL repository,

``` sh
dnf install epel-release
```

Then to enable powertools

``` sh
# On RHEL 8 / Rocky 8
dnf config-manager --set-enabled codeready-builder-for-rhel-8-rhui-rpms
dnf config-manager --set-enabled powertools
dnf install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

# On RHEL 9 / Rocky 9, PowerTools is called crb (CodeReady Builder)
dnf config-manager --set-enabled codeready-builder-for-rhel-9-rhui-rpms
dnf config-manager --set-enabled crb
dnf install https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm
```

Then use the `dnf` command for [**pgprtdbg**][pgprtdbg] to install the required packages.


### FreeBSD

On FreeBSD, `pkg` is used instead of `dnf` or `yum`.

Use `pkg install <package name>` to install the following packages

``` sh
git gcc cmake libev
```

### Build

#### Release build

The following commands will install [**pgprtdbg**][pgprtdbg] in the `/usr/local` hierarchy.

```sh
git clone https://github.com/jesperpedersen/pgprtdbg.git
cd pgprtdbg
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

See [RPM](https://github.com/jesperpedersen/pgprtdbg/blob/main/doc/RPM.md) for how to build a RPM of [**pgprtdbg**][pgprtdbg].

#### Debug build

The following commands will create a `DEBUG` version of [**pgprtdbg**][pgprtdbg].

```sh
git clone https://github.com/jesperpedersen/pgprtdbg.git
cd pgprtdbg
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## Compiling the documentation

[**pgprtdbg**][pgprtdbg]'s documentation requires

* [pandoc](https://pandoc.org/)
* [texlive](https://www.tug.org/texlive/)

```sh
dnf install pandoc texlive-scheme-basic \
            'tex(footnote.sty)' 'tex(footnotebackref.sty)' \
            'tex(pagecolor.sty)' 'tex(hardwrap.sty)' \
            'tex(mdframed.sty)' 'tex(sourcesanspro.sty)' \
            'tex(ly1enc.def)' 'tex(sourcecodepro.sty)' \
            'tex(titling.sty)' 'tex(csquotes.sty)' \
            'tex(zref-abspage.sty)' 'tex(needspace.sty)'

```

You will need the `Eisvogel` template as well which you can install through

```
wget https://github.com/Wandmalfarbe/pandoc-latex-template/releases/download/2.4.2/Eisvogel-2.4.2.tar.gz
tar -xzf Eisvogel-2.4.2.tar.gz
mkdir -p $HOME/.local/share/pandoc/templates
mv eisvogel.latex $HOME/.local/share/pandoc/templates
```

where `$HOME` is your home directory.

### Build

These packages will be detected during `cmake` and built as part of the main build.
