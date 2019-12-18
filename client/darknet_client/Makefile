DEBUG = 1

CPP = g++
COMMON = -DOPENCV
CXXFLAGS = -g -Wall -O2 -std=c++11 -DOPENCV
LDFLAGS = -lstdc++ -lpthread -lzmq -lrt -ltbb

CXXFLAGS += `pkg-config --cflags json-c`
CXXFLAGS += `pkg-config --cflags opencv`

LDFLAGS += `pkg-config --libs json-c`
LDFLAGS += `pkg-config --libs opencv`

ifeq ($(DEBUG), 1)
COMMON += -DDEBUG
endif

VPATH = ./src/
OBJDIR = ./obj/
DEPS = $(wildcard src/*.h*)

EXEC1 = darknet_client
EXEC1_OBJ = main.o frame.o mem_pool.o base64.o args.o util.o
EXEC1_OBJS = $(addprefix $(OBJDIR), $(EXEC1_OBJ))

OBJS = $(EXEC1_OBJS) 
EXECS = $(EXEC1) 

all: $(OBJDIR) $(EXECS)

$(EXEC1): $(EXEC1_OBJS)
	$(CPP) $(COMMON) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJDIR)%.o: %.cpp $(DEPS)
	$(CPP) $(COMMON) $(CXXFLAGS) -c $< -o $@ 

$(OBJDIR):
	mkdir -p $(OBJDIR) res

clean:
	rm -rf $(OBJS) $(EXECS) 
