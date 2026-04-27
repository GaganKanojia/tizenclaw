Name:       rvc-tidl-test
Summary:    Diagnostic tool for com.samsung.vr.robot-main TIDL interface
Version:    1.0.0
Release:    1
Group:      Development/Tools
License:    Apache-2.0
%undefine _debugsource_packages
Source0:    %{name}-%{version}.tar.gz
Source1001: %{name}.manifest

BuildRequires:  cmake
BuildRequires:  pkgconfig(capi-rpc-port)
BuildRequires:  pkgconfig(bundle)
BuildRequires:  pkgconfig(dlog)

%description
Standalone diagnostic utility that validates the RVCTizenClawService
TIDL RpcPort interface on com.samsung.vr.robot-main.
Sends vc_command N and reports the full roundtrip result with timing.

%prep
%setup -q -n %{name}-%{version}
cp %{SOURCE1001} .

%build
%cmake .
%__make %{?_smp_mflags}

%install
%make_install

%files
%defattr(-,root,root,-)
/usr/bin/rvc-tidl-test
