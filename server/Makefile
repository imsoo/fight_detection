DEBUG = 1

CPP = g++
COMMON = -DOPENCV
CXXFLAGS = -g -Wall -O2 -std=c++11 -DOPENCV
LDFLAGS = -lstdc++ -lpthread -lzmq -lrt -ltbb -ldarknet -lboost_serialization

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

EXEC1 = ventilator
EXEC1_OBJ = ventilator.o frame.o mem_pool.o base64.o
EXEC1_OBJS = $(addprefix $(OBJDIR), $(EXEC1_OBJ))

EXEC2 = worker
EXEC2_OBJ = worker.o people.o pose_detector.o frame.o mem_pool.o base64.o args.o
EXEC2_OBJS = $(addprefix $(OBJDIR), $(EXEC2_OBJ))

EXEC3 = sink
EXEC3_OBJ = sink.o people.o Tracker.o Hungarian.o KalmanTracker.o frame.o mem_pool.o base64.o
EXEC3_OBJS = $(addprefix $(OBJDIR), $(EXEC3_OBJ))

OBJS = $(EXEC1_OBJS) $(EXEC2_OBJS) $(EXEC3_OBJS)
EXECS = $(EXEC1) $(EXEC2) $(EXEC3)
INPROCS = processed unprocessed action

all: $(OBJDIR) $(EXECS)

$(EXEC1): $(EXEC1_OBJS)
	$(CPP) $(COMMON) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(EXEC2): $(EXEC2_OBJS)
	$(CPP) $(COMMON) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(EXEC3): $(EXEC3_OBJS)
	$(CPP) $(COMMON) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJDIR)%.o: %.cpp $(DEPS)
	$(CPP) $(COMMON) $(CXXFLAGS) -c $< -o $@ 

$(OBJDIR):
	mkdir -p $(OBJDIR) cfg weights names train

clean:
	rm -rf $(OBJS) $(EXECS) $(INPROCS)
