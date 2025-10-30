CC := g++
CFLAGS := -Wall -g

SRCS := main.cpp Server.cpp Client.cpp Database.cpp tinyxml2.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := myprogram
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $@ from $^"
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.cpp
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning..."
	rm -f $(OBJS) $(TARGET)
