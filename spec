Name: ucspi-unix
Summary: UNIX-domain socket client-server command-line tools
Version: @VERSION@
Release: 1
Copyright: GPL
Group: Utilities/System
Source: http://em.ca/~bruceg/ucspi-unix/%{version}/ucspi-unix-%{version}.tar.gz
BuildRoot: /tmp/ucspi-unix-root
URL: http://em.ca/~bruceg/ucspi-unix/
Packager: Bruce Guenter <bruceg@em.ca>

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
make programs

%install
rm -fr $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_bindir}
echo $RPM_BUILD_ROOT%{_bindir} >conf-bin
make installer instcheck
./installer
./instcheck

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYING ChangeLog NEWS PROTOCOL README TODO
%{_bindir}/*
