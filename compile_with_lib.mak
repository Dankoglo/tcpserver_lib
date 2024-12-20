CXX=g++
CXXFLAGS=-c
LDFLAGS=-pthread

COMPILE=$(CXX) $(CXXFLAGS) $<
LINK=$(CXX) $(LDFLAGS) $*.o lib/objects/lib.o -o $*
REMOVE_OBJECT=rm $*.o

%: %.cpp
	@$(COMPILE)
	@$(LINK)
	@$(REMOVE_OBJECT)