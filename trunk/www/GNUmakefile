SRCDIR  = html_src
DSTDIR  = .
SOURCES = $(wildcard $(SRCDIR)/*html)
TARGETS = $(subst $(SRCDIR),$(DSTDIR),$(SOURCES))
SCRIPTDIR = scripts
SCRIPTS = $(shell echo scripts/*py)

all: $(TARGETS)

$(DSTDIR)/%.html: $(SRCDIR)/%.html $(SCRIPTS) xorp.css
	$(SCRIPTDIR)/XorpOrgGenerator.py -d $(SRCDIR) $@

check: $(TARGETS)
	@for i in $(TARGETS) ; do xmllint --valid --html --noout $$i && echo $$i passed. ; done

clean:
	rm -rf $(TARGETS)
