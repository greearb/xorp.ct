
vpath %.C .:../src
vpath %.c ../src/contrib

INSTALL_DIR = ../bin
CGI_DIR = /home/httpd/cgi-bin
CPPFLAGS = -I. -I../src -I/usr/local/include
CXXFLAGS = -O -g -Wall -Woverloaded-virtual -Wcast-qual -Wuninitialized
CFLAGS = -O -g -Wall -Woverloaded-virtual -Wcast-qual -Wuninitialized

OBJS	= asbrlsa.o \
	  asexlsa.o \
	  avl.o \
	  cksum.o \
	  config.o \
	  dbage.o \
	  grplsa.o \
	  hostmode.o \
	  ifcfsm.o \
	  lsa.o \
	  lsalist.o \
	  lsdb.o \
	  monitor.o \
	  mospf.o \
	  nbrfsm.o \
	  netlsa.o \
	  opqlsa.o \
	  ospf.o \
	  pat.o \
	  phyint.o \
	  priq.o \
	  rte.o \
	  rtrlsa.o \
	  spfack.o \
	  spfarea.o \
	  spfcalc.o \
	  spfdd.o \
	  spfhello.o \
	  spfifc.o \
	  spflood.o \
	  spfnbr.o \
	  spforig.o \
	  spfutil.o \
	  spfvl.o \
	  summlsa.o \
	  timer.o \
	  md5c.o

install: ospfd ospfd_mon ospfd_browser
	install ospfd ${INSTALL_DIR}
	install ospfd_mon ${INSTALL_DIR}
	install ospfd_browser ${CGI_DIR}
	cp ospfd.tcl ${INSTALL_DIR}

ospfd:	ospfd_freebsd.C freebsd.o system.o tcppkt.o ${OBJS}
	g++ $(CXXFLAGS) $(CPPFLAGS) ospfd_freebsd.C freebsd.o system.o \
	 tcppkt.o ${OBJS} \
	-DINSTALL_DIR=\"${INSTALL_DIR}\" -L/usr/local/lib -ltcl83 -lm \
	-o ospfd

ospfd_mon: tcppkt.o lsa_prn.o

ospfd_browser:	tcppkt.o pat.o lsa_prn.o

clean:
	rm -rf .depfiles
	rm -f *.o ospf_sim ospfd_sim ospfd_mon ospfd_browser

# Stuff to automatically maintain dependency files

.c.o:
	gcc -MD $(CFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

.C.o:
	g++ -MD $(CXXFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

-include $(OBJS:%.o=.depfiles/%.d)

