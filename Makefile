TARGET=test
CPPFLAGS:=`pkg-config eigen3 --cflags` -Wall -Werror

all: $(TARGET).stl

$(TARGET).stl: $(TARGET)
	./$^ > $@

install-fc:
	sudo yum install eigen3-devel:

clean:
	rm -f $(TARGET) $(TARGET).stl

