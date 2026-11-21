Name:		packjpg
Version:	2.5k
Release:	1%{?dist}
Summary:	Lossless JPEG re-compression
URL:		https://github.com/packjpg/packJPG
License:	LGPLv3
Group:		Applications/File
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Source0:	%{name}-%{version}.tar.xz

%description
packJPG is a compression program specially designed for further
compression of JPEG images without causing any further loss. Typically
it reduces the file size of a JPEG file by 20%.

%prep
%setup -q


%build
CXXFLAGS="%{optflags}" LDFLAGS="%{?__global_ldflags}" make -C source %{?_smp_mflags} STRIP=


%install
rm -rf %{buildroot}
make -C source install DESTDIR=%{buildroot} PREFIX=%{_prefix} BINDIR=%{_bindir}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root)
%{_bindir}/packJPG


%changelog
* Fri Jul 18 2014 Bryan Stillwell <bstillwell@photobucket.com> - 2.5j-1
- Initial packaging
