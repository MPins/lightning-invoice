# Compiler
CXX = g++
# Compiler flags
CXXFLAGS = -g -Wall -Wextra -std=c++11
# Source files
SRCS = tests.cpp bech32.cpp payment_request.cpp
# Object files
OBJS = $(SRCS:.cpp=.o)
# Executable name
TARGET = tests

# Default target
all: $(TARGET)

# Linking rule
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compilation rule for C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean rule
clean:
	rm -f $(TARGET) $(OBJS)

