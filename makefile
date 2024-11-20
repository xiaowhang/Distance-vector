# 设置编译器和编译选项
CXX = g++
CXXFLAGS = -O2 -Wall -std=c++17

# 项目目录设置
SRC_DIR = src
INCLUDE_DIR = include
OUT_DIR = out

# 头文件和源文件
HEADERS = $(INCLUDE_DIR)/common.h $(INCLUDE_DIR)/router.h $(INCLUDE_DIR)/utils.h
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/router.cpp $(SRC_DIR)/utils.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# 可执行文件名称
EXEC = main

# 默认目标：构建可执行文件
all: $(EXEC)

# 编译源文件并生成目标文件
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# 链接所有目标文件生成可执行文件
$(EXEC): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXEC)

# 清理编译产生的文件
clean:
	rm -f $(OBJECTS) $(EXEC)

# 运行可执行文件
run: $(EXEC)
	./$(EXEC)

# 安装目标（如果需要的话）
install: $(EXEC)
	cp $(EXEC) /usr/local/bin/

