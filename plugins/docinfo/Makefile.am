# docinfo plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)

plugin_LTLIBRARIES = libdocinfo.la

BUILT_SOURCES = \
	gedit-docinfo-resources.c

libdocinfo_la_SOURCES = \
	gedit-docinfo-plugin.h	\
	gedit-docinfo-plugin.c	\
	$(BUILT_SOURCES)

libdocinfo_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libdocinfo_la_LIBADD  = $(GEDIT_LIBS)

gedit-docinfo-resources.c: gedit-docinfo.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies gedit-docinfo.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source gedit-docinfo.gresource.xml

plugin_in_files = docinfo.plugin.desktop.in

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.plugin.desktop.in=.plugin)

EXTRA_DIST = $(plugin_in_files)
CLEANFILES = $(BUILT_SOURCES) $(plugin_DATA)
DISTCLEANFILES = $(BUILT_SOURCES) $(plugin_DATA)

-include $(top_srcdir)/git.mk
