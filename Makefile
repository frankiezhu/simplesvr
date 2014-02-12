CXX    := g++
LD     := g++
INC := -I ../libs/libtinyxml
LDFLAGS+= -L ../libs/libtinyxml -Wl,-rpath,../libs/libtinyxml/ -ltinyxml -ldl -rdynamic
CXXFLAGS+= -Wall -Wstrict-aliasing -g $(INC) 
SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(SRCS))
TARGET=simplesvr

$(OBJ):%o:%.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TARGET):$(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

all:$(TARGET)

clean:
	rm -f *.o $(TARGET)

.PHONY:clean
