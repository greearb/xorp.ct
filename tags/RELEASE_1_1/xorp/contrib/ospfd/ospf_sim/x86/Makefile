
vpath %.C ../../src:../../linux:..
vpath %.c ../../src/contrib

INSTALL_DIR = .
CGI_DIR = c:/cgi-bin
CC = g++
CPPFLAGS = -I. -I../../src -I../../linux -I..
CFLAGS = -O -g -Wall -Woverloaded-virtual -Wcast-qual -Wuninitialized
CXXFLAGS = -O -g -Wall -Woverloaded-virtual -Wcast-qual -Wuninitialized
LDFLAGS = 

OBJS	= asbrlsa.o \
	  asexlsa.o \
	  avl.o \
	  cksum.o \
	  config.o \
	  dbage.o \
	  grplsa.o \
	  helper.o \
	  hostmode.o \
	  ifcfsm.o \
	  lsa.o \
	  lsalist.o \
	  lsdb.o \
	  monitor.o \
	  mospf.o \
	  mtrace.o \
	  nbrfsm.o \
	  netlsa.o \
	  opqlsa.o \
	  ospf.o \
	  pat.o \
	  phyint.o \
	  priq.o \
	  restart.o \
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
	  md5c.o \
	  linux.o \
	  ospfd_sim.o \
	  tcppkt.o \
	  tlv.o \
	  sim_system.o


install:  ospf_sim ospfd_sim ospfd_mon ospfd_browser
	install ospfd_browser.exe ${CGI_DIR}
	cp -f ../ospf_sim.tcl .

ospfd_sim: ${OBJS}

ospfd_mon: tcppkt.o lsa_prn.o

ospf_sim: tcppkt.o avl.o pat.o sim_x86.o ../sim.C
	g++ tcppkt.o avl.o pat.o sim_x86.o ../sim.C -ltcl80 -ltk80 \
	${CPPFLAGS} ${CXXFLAGS} -DINSTALL_DIR=\"${INSTALL_DIR}\" -o ospf_sim

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
