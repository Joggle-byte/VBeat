CXX      = g++
CXXFLAGS = -Wall -Iinclude
LDFLAGS  = -Llib
LDLIBS   = -lbass

SRCS     = main.cpp src/audiobus.cpp src/audioplayer.cpp src/song.cpp src/playlist.cpp
OBJS     = $(SRCS:.cpp=.o)
TARGET   = liveplay.exe

all: $(TARGET)

# $(LDLIBS) DEVE stare dopo $(OBJS)
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	del /f /q src\*.o $(TARGET)