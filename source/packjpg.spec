Name:		packjpg
Version:	2.5j
Release:	1%{?dist}
Summary:	Lossless JPEG re-compression
URL:		http://www.elektronik.htw-aalen.de/packjpg/
License:	LGPL
Group:		Applications/File
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Source0:	%{name}-%{version}.tar.xz
Patch0:		%{name}-build-fix.patch

%description
packJPG is a compression program specially designed for further
compression of JPEG images without causing any further loss. Typically
it reduces the file size of a JPEG file by 20%.

%prep
%setup -q
%patch0 -p1


%build
cd source
CFLAGS="%{optflags}" LDFLAGS="%{?__global_ldflags}" make -f Makefile_linux %{?_smp_mflags}


%install
rm -rf %{buildroot}
install -D -m755 source/%{name} %{buildroot}%{_bindir}/%{name}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%{_bindir}/%{name}


%changelog
* Fri Jul 18 2014 Bryan Stillwell <bstillwell@photobucket.com> - 2.5j-1
- Initial packaging
