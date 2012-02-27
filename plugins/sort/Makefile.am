# sort plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)	

plugin_LTLIBRARIES = libsort.la

libsort_la_SOURCES = \
	gedit-sort-plugin.h	\
	gedit-sort-plugin.c	\
	gedit-sort-resources.c

libsort_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libsort_la_LIBADD  = $(GEDIT_LIBS)

gedit-sort-resources.c: gedit-sort.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies gedit-sort.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source gedit-sort.gresource.xml

plugin_in_files = sort.plugin.desktop.in

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.plugin.desktop.in=.plugin)

EXTRA_DIST = $(plugin_in_files)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)


-include $(top_srcdir)/git.mk
