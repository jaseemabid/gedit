# Spell checker plugin
plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = 							\
	-I$(top_srcdir)						\
	$(GEDIT_CFLAGS) 					\
	$(ENCHANT_CFLAGS)					\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	$(GTK_MAC_CFLAGS)

BUILT_SOURCES = 					\
	gedit-spell-marshal.c				\
	gedit-spell-marshal.h				\
	gedit-spell-resources.c

plugin_LTLIBRARIES = libspell.la

libspell_la_SOURCES = \
	gedit-spell-plugin.c 				\
	gedit-spell-plugin.h 				\
	gedit-spell-checker.c 				\
	gedit-spell-checker.h				\
	gedit-spell-checker-dialog.c			\
	gedit-spell-checker-dialog.h			\
	gedit-spell-checker-language.c			\
	gedit-spell-checker-language.h			\
	gedit-spell-language-dialog.c			\
	gedit-spell-language-dialog.h			\
	gedit-automatic-spell-checker.c			\
	gedit-automatic-spell-checker.h			\
	gedit-spell-utils.c				\
	gedit-spell-utils.h				\
	$(BUILT_SOURCES)			

libspell_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS) 
libspell_la_LIBADD  = $(GEDIT_LIBS) $(ENCHANT_LIBS)

# Mac OSX convenience library and ldflags
if OS_OSX
noinst_LTLIBRARIES = libosx.la

libosx_la_LDFLAGS = -framework Cocoa
libosx_la_LIBADD = -lobjc
libosx_la_CFLAGS = -xobjective-c

libosx_la_SOURCES = 		\
	gedit-spell-osx.c		\
	gedit-spell-osx.h

libspell_la_LIBADD += libosx.la  $(GTK_MAC_LIBS)
endif

gedit-spell-marshal.h: gedit-spell-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $< --header --prefix=gedit_marshal > $@

gedit-spell-marshal.c: gedit-spell-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) echo "#include \"gedit-spell-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_marshal >> $@

gedit-spell-resources.c: gedit-spell.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies gedit-spell.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source gedit-spell.gresource.xml

plugin_in_files = spell.plugin.desktop.in

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.plugin.desktop.in=.plugin)

EXTRA_DIST = 					\
	$(plugin_in_files)			\
	gedit-spell-marshal.list		\
	gedit-spell.gresource.xml

CLEANFILES = $(BUILT_SOURCES) $(plugin_DATA)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
