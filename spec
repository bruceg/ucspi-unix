Name: ucspi-unix
Summary: UNIX-domain socket client-server command-line tools
Version: @VERSION@
Release: 1
License: GPL
Group: Utilities/System
Source: http://untroubled.org/ucspi-unix/archive/ucspi-unix-%{version}.tar.gz
BuildRoot: %{_tmppath}/ucspi-unix-root
URL: http://untroubled.org/ucspi-unix/
Packager: Bruce Guenter <bruceg@untroubled.org>

%description
unixclient and unixserver are command-line tools for building UNIX
domain client-server applications.  unixclient connects to a UNIX domain
socket and runs a program of your choice.  unixserver creates a UNIX
domain socket, waits for incoming connections and, for each connection,
runs a program of your choice.

unixclient and unixserver conform to UCSPI, the UNIX Client-Server
Program Interface, using UNIX domain sockets.  UCSPI tools are available
for several different networks.

%prep
%setup

%build
make

%install
rm -fr %{buildroot}
make install prefix=%{buildroot} bindir=%{_bindir} mandir=%{_mandir}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc COPYING NEWS PROTOCOL README TODO VERSION
%{_bindir}/*
%{_mandir}/*/*
