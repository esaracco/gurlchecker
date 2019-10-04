> ***IMPORTANT:***
> _This is a dead, no longer maintained, project!_
> 

# gURLChecker

gURLChecker is a graphical websites checker for GNU/Linux.

## Installation

```bash
$ ./autogen.sh --prefix=/usr
$ make
$ su -c "make install"
$ /usr/bin/gurlchecker
```
## FAQ

1. When I try to compile CVS files I have a error from autogen.sh telling me that my automake/autoconf/libtool/gtkdoc version if not ok!
> If you have some problems with "autogen.sh", just delete the following last lines:
```
  REQUIRED_AUTOMAKE_VERSION=1.8 \
  REQUIRED_AUTOCONF_VERSION=2.50 \
  REQUIRED_LIBTOOL_VERSION=1.5 \
  REQUIRED_GTK_DOC_VERSION=1.1 \
```
> and try again.
2. How can I build gurlchecker with/without libsqlite3 support?
> Just pass `--disable-sqlite3` to `./configure`.
3. How can I build gurlchecker without gtk-doc support?
> Just pass `--disable-gtk-doc` to `./configure`.
4. How can I build gurlchecker with/without libclamav support?
> Just pass `--disable-clamav` to `./configure`.
5. How can I build gurlchecker without libcroco support?
> Just pass `--disable-croco` to `./configure`.
6. How can I build gurlchecker without libtidy support?
> Just pass `--disable-tidy` to `./configure`.
7. How can I build gurlchecker without SSL support?
> Just pass `--disable-gnutls` to `./configure`.
8. Why gurlchecker does not correctly handle the RECTANGLE attribute of IMG tag?
> The RECTANGLE attribute is not in HTML 4 specification.
> Response from xml@gnome.org (gurlchecker use libxml2 to parse HTML):
>
> . from **Malcolm Tredinnick**:
> > Reading the HTML specification (for HTML 4.01), it looks to me like img elements can take precisely zero rectangle attributes (i.e. it is not a valid attribute). So anything you get back in the above situation would be purely a bonus.
>
> . from **Morus Walter**:
> > depends what you call HTML. If you look at the html 4.01 spec you won't find any rectangle attribute on img at all. And, since HTML is based on SGML you can never have any element hanging more than one instance of any attribute. On the other hand, HTML as it is found on the web often is just tag soup and I wouldn't be surprised if some browsers recognice such constructs or some browser vendors suggested such syntax in the past. OTOH I didn't find a rectangle attribute for any element in the rather long list of html dtds on my system. The only occurences of rectangle is found in ie-2.0.dtd and ie-3.0.dtd where it is used as an attribute value. You cannot expect libxml2 to parse that.
9. I have installed libxml2 I still have errors at compile time!
> You must have a libxml2 >= 2.6.


















