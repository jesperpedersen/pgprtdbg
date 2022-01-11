Name:          pgprtdbg
Version:       0.4.0
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

%{__mkdir} build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
%{__make}

%install

%{__mkdir} -p %{buildroot}%{_sysconfdir}
%{__mkdir} -p %{buildroot}%{_bindir}
%{__mkdir} -p %{buildroot}%{_libdir}
%{__mkdir} -p %{buildroot}%{_docdir}/%{name}

%{__install} -m 644 %{_builddir}/%{name}-%{version}/LICENSE %{buildroot}%{_docdir}/%{name}/LICENSE
%{__install} -m 644 %{_builddir}/%{name}-%{version}/README.md %{buildroot}%{_docdir}/%{name}/README.md
%{__install} -m 644 %{_builddir}/%{name}-%{version}/doc/CONFIGURATION.md %{buildroot}%{_docdir}/%{name}/CONFIGURATION.md
%{__install} -m 644 %{_builddir}/%{name}-%{version}/doc/GETTING_STARTED.md %{buildroot}%{_docdir}/%{name}/GETTING_STARTED.md
%{__install} -m 644 %{_builddir}/%{name}-%{version}/doc/RPM.md %{buildroot}%{_docdir}/%{name}/RPM.md

%{__install} -m 644 %{_builddir}/%{name}-%{version}/doc/etc/pgprtdbg.conf %{buildroot}%{_sysconfdir}/pgprtdbg.conf

%{__install} -m 755 %{_builddir}/%{name}-%{version}/build/src/pgprtdbg %{buildroot}%{_bindir}/pgprtdbg

%{__install} -m 755 %{_builddir}/%{name}-%{version}/build/src/libpgprtdbg.so.%{version} %{buildroot}%{_libdir}/libpgprtdbg.so.%{version}

chrpath -r %{_libdir} %{buildroot}%{_bindir}/pgprtdbg

cd %{buildroot}%{_libdir}/
%{__ln_s} libpgprtdbg.so.%{version} libpgprtdbg.so.0
%{__ln_s} libpgprtdbg.so.0 libpgprtdbg.so

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
