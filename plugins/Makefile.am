DIST_SUBDIRS = \
	changecase	\
	checkupdate	\
	docinfo		\
	filebrowser	\
	modelines	\
	sort		\
	spell 		\
	time		\
	zeitgeist

if ENABLE_PYTHON
DIST_SUBDIRS += \
	externaltools	\
	pythonconsole	\
	quickopen	\
	snippets
endif

SUBDIRS = \
	changecase	\
	docinfo		\
	filebrowser	\
	modelines	\
	sort		\
	time

if ENABLE_PYTHON
SUBDIRS += \
	pythonconsole	\
	quickopen	\
	snippets
endif

if !OS_WIN32
if ENABLE_PYTHON
SUBDIRS      += externaltools
endif
endif

if ENABLE_ENCHANT
SUBDIRS      += spell
endif

if ENABLE_UPDATER
SUBDIRS      += checkupdate
endif

if ENABLE_ZEITGEIST
SUBDIRS      += zeitgeist
endif

-include $(top_srcdir)/git.mk
