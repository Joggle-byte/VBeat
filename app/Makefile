CXX      = clang++
CXXFLAGS = -std=c++17 -Wall -Iinclude
LDFLAGS  = -Llib -L. -Wl,-rpath,@executable_path -framework AudioToolbox -framework CoreAudio -framework CoreFoundation
LDLIBS   = -lbass -lftxui-component -lftxui-dom -lftxui-screen

# Directories
SRC_DIR  = src
OBJ_DIR  = obj
TARGET   = vbeat

SRCS     := main.cpp $(wildcard $(SRC_DIR)/*.cpp)


OBJS     := $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.cpp=.o)))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@


$(OBJ_DIR)/main.o: main.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)

.PHONY: all clean