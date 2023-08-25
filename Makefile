STB_INCLUDE_PATH = ./libraries/stb
CFLAGS = -std=c++17 -O2 -I$(STB_INCLUDE_PATH)
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
EXEC = renderer # Executable name

$(EXEC): src/main.cpp
	@echo Compiling...
	g++ $(CFLAGS) -o $(EXEC) src/main.cpp $(LDFLAGS)

.PHONY: test clean

test: $(EXEC)
	./$(EXEC)

clean:
	rm -f $(EXEC)
