# time plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 				\
	$(GEDIT_CFLAGS) 				\
	$(WARN_CFLAGS)					\
	$(DISABLE_DEPRECATED_CFLAGS)			

plugin_LTLIBRARIES = libtime.la

BUILT_SOURCES = \
	gedit-time-resources.c

libtime_la_SOURCES = \
	gedit-time-plugin.h	\
	gedit-time-plugin.c	\
	$(BUILT_SOURCES)

libtime_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libtime_la_LIBADD  = $(GEDIT_LIBS)

gedit-time-resources.c: gedit-time.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies $(srcdir)/gedit-time.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $(srcdir)/gedit-time.gresource.xml

plugin_in_files = time.plugin.desktop.in

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.plugin.desktop.in=.plugin)

gsettings_ENUM_NAMESPACE = org.gnome.gedit.plugins.time
gsettings_ENUM_FILES = $(top_srcdir)/plugins/time/gedit-time-plugin.c
gsettings_SCHEMAS = org.gnome.gedit.plugins.time.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST =					\
	$(plugin_in_files)			\
	org.gnome.gedit.plugins.time.gschema.xml.in.in \
	gedit-time.gresource.xml		\
	gedit-time-dialog.ui			\
	gedit-time-setup-dialog.ui

CLEANFILES = $(plugin_DATA) $(gsettings_SCHEMAS) $(BUILT_SOURCES)
DISTCLEANFILES = $(plugin_DATA) $(gsettings_SCHEMAS) $(BUILT_SOURCES)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

-include $(top_srcdir)/git.mk
