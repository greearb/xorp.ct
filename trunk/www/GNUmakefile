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

install-www: all
	rsync -avz ./ www.xorp.org:/var/www/xorp.org/www.xorp.org/

install-test: all
	rsync -avz ./ www.xorp.org:/var/www/xorp.org/test.xorp.org/

clean:
	rm -rf $(TARGETS)
