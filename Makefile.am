SUBDIRS = man src

EXTRA_DIST = debian doc include utils

dist-hook:
	for cvsdir in `find $(distdir) -name CVS`; do \
	  [ -d "$$cvsdir" ] && rm -rf "$$cvsdir" ; \
	done
