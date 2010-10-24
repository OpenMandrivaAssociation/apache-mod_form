#Module-Specific definitions
%define mod_name mod_form
%define mod_conf A71_%{mod_name}.conf
%define mod_so %{mod_name}.so

Summary:	DSO module for the apache web server
Name:		apache-%{mod_name}
Version:	0
Release:	%mkrel 15
Group:		System/Servers
License:	GPL
URL:		http://apache.webthing.com/mod_form/
# there is no official tar ball
# http://apache.webthing.com/svn/apache/forms/mod_form.c
Source0:	http://apache.webthing.com/svn/apache/filters/xmlns/mod_form.c
Source1:	http://apache.webthing.com/svn/apache/filters/xmlns/mod_form.h
Source2:	README.mod_form
Source3:	%{mod_conf}
# preserve r->args (apr_strtok is
# destructive in this regard). Makes mod_autoindex work again in
# conjunction with directories where FormGET is enabled.
Patch0:         mod_form.c.preserve_args.patch
Requires(pre): rpm-helper
Requires(postun): rpm-helper
Requires(pre):	apache-conf >= 2.2.0
Requires(pre):	apache >= 2.2.0
Requires:	apache-conf >= 2.2.0
Requires:	apache >= 2.2.0
BuildRequires:	apache-devel >= 2.2.0
BuildRequires:	file
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
mod_form is a utility to decode data submitted from Web forms. It deals with
both GET and POST methods where the data are encoded using the default content
type application/x-www-form-urlencoded. It does not decode multipart/form-data
(file upload) forms: for those you should use mod_upload.

%package	devel
Summary:	Development API for the mod_form apache module
Group:		Development/C

%description	devel
mod_form is a utility to decode data submitted from Web forms. It deals with
both GET and POST methods where the data are encoded using the default content
type application/x-www-form-urlencoded. It does not decode multipart/form-data
(file upload) forms: for those you should use mod_upload.

This package contains the development API for the mod_form apache module.

%prep

%setup -q -c -T -n %{mod_name}-%{version}

cp %{SOURCE0} %{mod_name}.c
cp %{SOURCE1} %{mod_name}.h
%patch0
cp %{SOURCE2} README
cp %{SOURCE3} %{mod_conf}

# strip away annoying ^M
find . -type f|xargs file|grep 'CRLF'|cut -d: -f1|xargs perl -p -i -e 's/\r//'
find . -type f|xargs file|grep 'text'|cut -d: -f1|xargs perl -p -i -e 's/\r//'

%build
%{_sbindir}/apxs -c %{mod_name}.c

%install
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

install -d %{buildroot}%{_sysconfdir}/httpd/modules.d
install -d %{buildroot}%{_libdir}/apache-extramodules
install -d %{buildroot}%{_includedir}

install -m0755 .libs/*.so %{buildroot}%{_libdir}/apache-extramodules/
install -m0644 mod_form.h %{buildroot}%{_includedir}/
install -m0644 %{mod_conf} %{buildroot}%{_sysconfdir}/httpd/modules.d/%{mod_conf}

%post
if [ -f %{_var}/lock/subsys/httpd ]; then
 %{_initrddir}/httpd restart 1>&2;
fi

%postun
if [ "$1" = "0" ]; then
 if [ -f %{_var}/lock/subsys/httpd ]; then
	%{_initrddir}/httpd restart 1>&2
 fi
fi

%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc README
%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/httpd/modules.d/%{mod_conf}
%attr(0755,root,root) %{_libdir}/apache-extramodules/%{mod_so}

%files devel
%defattr(-,root,root)
%attr(0644,root,root) %{_includedir}/mod_form.h
