# filebrowser

plugindir = $(GEDIT_PLUGINS_LIBS_DIR)

INCLUDES = \
	-I$(top_srcdir) 						\
	$(GEDIT_CFLAGS) 						\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED_CFLAGS)

BUILT_SOURCES = \
	gedit-file-browser-resources.c		\
	gedit-file-browser-enum-types.h		\
	gedit-file-browser-enum-types.c		\
	gedit-file-browser-marshal.h 		\
	gedit-file-browser-marshal.c

plugin_LTLIBRARIES = libfilebrowser.la

NOINST_H_FILES = \
	gedit-file-bookmarks-store.h 		\
	gedit-file-browser-store.h 		\
	gedit-file-browser-view.h 		\
	gedit-file-browser-widget.h 		\
	gedit-file-browser-error.h		\
	gedit-file-browser-utils.h		\
	gedit-file-browser-plugin.h		\
	gedit-file-browser-messages.h		\
	messages/gedit-file-browser-message-activation.h \
	messages/gedit-file-browser-message-add-context-item.h \
	messages/gedit-file-browser-message-add-filter.h \
	messages/gedit-file-browser-message-get-root.h \
	messages/gedit-file-browser-message-get-view.h \
	messages/gedit-file-browser-message-id.h \
	messages/gedit-file-browser-message-id-location.h \
	messages/gedit-file-browser-message-set-emblem.h \
	messages/gedit-file-browser-message-set-root.h \
	messages/messages.h

libfilebrowser_la_SOURCES = \
	$(BUILT_SOURCES) 			\
	gedit-file-bookmarks-store.c 		\
	gedit-file-browser-store.c 		\
	gedit-file-browser-view.c 		\
	gedit-file-browser-widget.c 		\
	gedit-file-browser-utils.c 		\
	gedit-file-browser-plugin.c		\
	gedit-file-browser-messages.c		\
	messages/gedit-file-browser-message-activation.c \
	messages/gedit-file-browser-message-add-context-item.c \
	messages/gedit-file-browser-message-add-filter.c \
	messages/gedit-file-browser-message-get-root.c \
	messages/gedit-file-browser-message-get-view.c \
	messages/gedit-file-browser-message-id.c \
	messages/gedit-file-browser-message-id-location.c \
	messages/gedit-file-browser-message-set-emblem.c \
	messages/gedit-file-browser-message-set-root.c \
	$(NOINST_H_FILES)

libfilebrowser_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)
libfilebrowser_la_LIBADD = $(GEDIT_LIBS)

plugin_in_files = filebrowser.plugin.desktop.in

%.plugin: %.plugin.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

gedit-file-browser-resources.c: gedit-file-browser.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies gedit-file-browser.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source gedit-file-browser.gresource.xml

gedit-file-browser-enum-types.h: gedit-file-browser-enum-types.h.template $(NOINST_H_FILES) $(GLIB_MKENUMS)
	(cd $(srcdir) && $(GLIB_MKENUMS) --template gedit-file-browser-enum-types.h.template $(NOINST_H_FILES)) > $@

gedit-file-browser-enum-types.c: gedit-file-browser-enum-types.c.template gedit-file-browser-enum-register.c.template $(NOINST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) && \
	$(GLIB_MKENUMS) --template gedit-file-browser-enum-types.c.template $(NOINST_H_FILES) && \
	$(GLIB_MKENUMS) --template gedit-file-browser-enum-register.c.template $(NOINST_H_FILES)) > $@

gedit-file-browser-marshal.h: gedit-file-browser-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $< --header --prefix=gedit_file_browser_marshal > $@

gedit-file-browser-marshal.c: gedit-file-browser-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) echo "#include \"gedit-file-browser-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_file_browser_marshal >> $@

plugin_DATA = $(plugin_in_files:.plugin.desktop.in=.plugin)

gsettings_ENUM_NAMESPACE = org.gnome.gedit.plugins.filebrowser
gsettings_ENUM_FILES = $(top_srcdir)/plugins/filebrowser/*.h
gsettings_SCHEMAS = org.gnome.gedit.plugins.filebrowser.gschema.xml

@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

update-messages:
	$(top_srcdir)/tools/generate-message.py --output-directory $(builddir)/messages/  --gpl --program gedit $(srcdir)/messages.xml

EXTRA_DIST = \
	$(plugin_in_files)				\
	gedit-file-browser.gresource.xml		\
	gedit-file-browser-enum-types.h.template	\
	gedit-file-browser-enum-types.c.template	\
	gedit-file-browser-enum-register.c.template	\
	gedit-file-browser-marshal.list			\
	org.gnome.gedit.plugins.filebrowser.gschema.xml.in.in

CLEANFILES = \
	$(plugin_DATA)		\
	$(BUILT_SOURCES)	\
	$(gsettings_SCHEMAS)

DISTCLEANFILES = \
	$(plugin_DATA)		\
	$(BUILT_SOURCES)	\
	$(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

-include $(top_srcdir)/git.mk
