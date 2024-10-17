#Module-Specific definitions
%define mod_name mod_form
%define mod_conf A71_%{mod_name}.conf
%define mod_so %{mod_name}.so

Summary:	DSO module for the apache web server
Name:		apache-%{mod_name}
Version:	0
Release:	18
Group:		System/Servers
License:	GPL
URL:		https://apache.webthing.com/mod_form/
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
%{_bindir}/apxs -c %{mod_name}.c

%install

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

%files
%doc README
%attr(0644,root,root) %config(noreplace) %{_sysconfdir}/httpd/modules.d/%{mod_conf}
%attr(0755,root,root) %{_libdir}/apache-extramodules/%{mod_so}

%files devel
%attr(0644,root,root) %{_includedir}/mod_form.h


%changelog
* Sat Feb 11 2012 Oden Eriksson <oeriksson@mandriva.com> 0-17mdv2012.0
+ Revision: 772655
- rebuild

* Tue May 24 2011 Oden Eriksson <oeriksson@mandriva.com> 0-16
+ Revision: 678314
- mass rebuild

* Sun Oct 24 2010 Oden Eriksson <oeriksson@mandriva.com> 0-15mdv2011.0
+ Revision: 587988
- rebuild

* Mon Mar 08 2010 Oden Eriksson <oeriksson@mandriva.com> 0-14mdv2010.1
+ Revision: 516101
- rebuilt for apache-2.2.15

* Fri Nov 06 2009 Oden Eriksson <oeriksson@mandriva.com> 0-13mdv2010.1
+ Revision: 461218
- added one patch from opensuse

* Sat Aug 01 2009 Oden Eriksson <oeriksson@mandriva.com> 0-12mdv2010.0
+ Revision: 406585
- rebuild

* Tue Jan 06 2009 Oden Eriksson <oeriksson@mandriva.com> 0-11mdv2009.1
+ Revision: 325763
- rebuild

* Mon Jul 14 2008 Oden Eriksson <oeriksson@mandriva.com> 0-10mdv2009.0
+ Revision: 234948
- rebuild

* Thu Jun 05 2008 Oden Eriksson <oeriksson@mandriva.com> 0-9mdv2009.0
+ Revision: 215580
- fix rebuild

* Fri Mar 07 2008 Oden Eriksson <oeriksson@mandriva.com> 0-8mdv2008.1
+ Revision: 181737
- rebuild

* Mon Feb 18 2008 Thierry Vignaud <tv@mandriva.org> 0-7mdv2008.1
+ Revision: 170722
- rebuild
- fix "foobar is blabla" summary (=> "blabla") so that it looks nice in rpmdrake
- kill re-definition of %%buildroot on Pixel's request

  + Olivier Blin <blino@mandriva.org>
    - restore BuildRoot

* Sat Sep 08 2007 Oden Eriksson <oeriksson@mandriva.com> 0-6mdv2008.0
+ Revision: 82583
- rebuild

* Sat Aug 18 2007 Oden Eriksson <oeriksson@mandriva.com> 0-5mdv2008.0
+ Revision: 65640
- rebuild


* Wed Mar 14 2007 Oden Eriksson <oeriksson@mandriva.com> 0-4mdv2007.1
+ Revision: 143752
- bunzip sources

* Sat Mar 10 2007 Oden Eriksson <oeriksson@mandriva.com> 0-3mdv2007.1
+ Revision: 140677
- rebuild

* Thu Nov 09 2006 Oden Eriksson <oeriksson@mandriva.com> 0-2mdv2007.0
+ Revision: 79426
- Import apache-mod_form

* Tue Jul 18 2006 Oden Eriksson <oeriksson@mandriva.com> 0-2mdv2007.0
- added the devel subpackage

* Tue Jul 18 2006 Oden Eriksson <oeriksson@mandriva.com> 0-1mdv2007.0
- initial Mandriva package

