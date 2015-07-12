TARGETS:=vase.stl marble-run.stl
CPPFLAGS:=`pkg-config eigen3 --cflags` -Wall -Werror
TARGET_BINS:=$(TARGETS:.stl=.run)

all: $(TARGET_BINS) $(TARGETS)

%.stl: %.run
	./$^ > $@

%.run: %.cpp
	$(CXX) $^ $(CPPFLAGS) -o $@

install-fc:
	sudo yum install eigen3-devel:

clean:
	rm -f $(TARGET_BINS) $(TARGETS)

