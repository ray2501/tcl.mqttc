%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}

Name:          tcl.mqttc
Summary:       MQTT Client for Tcl
Version:       0.4
Release:       0
License:       BSD 3-Clause License
Group:         Development/Libraries/Tcl
Source:        %{name}-%{version}.tar.gz
URL:           https://github.com/ray2501/tcl.mqttc
BuildRequires: autoconf
BuildRequires: make
BuildRequires: tcl-devel >= 8.4
BuildRequires: openssl-devel
BuildRoot:     %{buildroot}

%description
MQTT Client for Tcl, based on Paho MQTT C Client for Posix and Windows.

%prep
%setup -q -n %{name}-%{version}

%build
./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}
make 

%install
make DESTDIR=%{buildroot} pkglibdir=%{tcl_archdir}/%{name}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%{tcl_archdir}
