Name:          pgprtdbg
Version:       0.3.1
Release:       1%{dist}
Summary:       Trace file for PostgreSQL protocol interaction
License:       BSD
URL:           https://github.com/jesperpedersen/pgprtdbg
BuildRequires: gcc
BuildRequires: cmake
BuildRequires: make
BuildRequires: libev
BuildRequires: libev-devel
Requires:      libev
Source:        https://github.com/jesperpedersen/pgprtdbg/releases/%{name}-%{version}.tar.gz

%description
Trace file for PostgreSQL protocol interaction

%prep
%setup -q

%build

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

%install

mkdir -p %{buildroot}%{_sysconfdir}
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_docdir}/%{name}

/usr/bin/install -m 644 %{_builddir}/%{name}-%{version}/LICENSE %{buildroot}%{_docdir}/%{name}/LICENSE
/usr/bin/install -m 644 %{_builddir}/%{name}-%{version}/README.md %{buildroot}%{_docdir}/%{name}/README.md
/usr/bin/install -m 644 %{_builddir}/%{name}-%{version}/doc/CONFIGURATION.md %{buildroot}%{_docdir}/%{name}/CONFIGURATION.md
/usr/bin/install -m 644 %{_builddir}/%{name}-%{version}/doc/GETTING_STARTED.md %{buildroot}%{_docdir}/%{name}/GETTING_STARTED.md
/usr/bin/install -m 644 %{_builddir}/%{name}-%{version}/doc/RPM.md %{buildroot}%{_docdir}/%{name}/RPM.md

/usr/bin/install -m 644 %{_builddir}/%{name}-%{version}/doc/etc/pgprtdbg.conf %{buildroot}%{_sysconfdir}/pgprtdbg.conf

/usr/bin/install -m 755 %{_builddir}/%{name}-%{version}/build/src/pgprtdbg %{buildroot}%{_bindir}/pgprtdbg

/usr/bin/install -m 755 %{_builddir}/%{name}-%{version}/build/src/libpgprtdbg.so.%{version} %{buildroot}%{_libdir}/libpgprtdbg.so.%{version}

chrpath -r %{_libdir} %{buildroot}%{_bindir}/pgprtdbg

cd %{buildroot}%{_libdir}/
ln -s -f libpgprtdbg.so.%{version} libpgprtdbg.so.0
ln -s -f libpgprtdbg.so.0 libpgprtdbg.so

%files
%license %{_docdir}/%{name}/LICENSE
%{_docdir}/%{name}/CONFIGURATION.md
%{_docdir}/%{name}/GETTING_STARTED.md
%{_docdir}/%{name}/README.md
%{_docdir}/%{name}/RPM.md
%config %{_sysconfdir}/pgprtdbg.conf
%{_bindir}/pgprtdbg
%{_libdir}/libpgprtdbg.so
%{_libdir}/libpgprtdbg.so.0
%{_libdir}/libpgprtdbg.so.%{version}

%changelog
