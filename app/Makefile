CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude
LDFLAGS  = -Llib
LDLIBS   = -lbass -lftxui-component -lftxui-dom -lftxui-screen -pthread

# Directories
SRC_DIR  = src
OBJ_DIR  = obj
TARGET   = vbeat.exe

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
	cmd /c if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

clean:
	cmd /c if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)
	cmd /c if exist $(TARGET) del /f /q $(TARGET)