TARGET := translator.dll
SOURCE := main.cpp
#GCC := mingw32-g++
GCC := i686-w64-mingw32-g++
STD := c++98
FLAGS := -std=$(STD) -lpsapi -s -shared -O3
STATICFLAGS := -static-libstdc++ -static-libgcc


all:
	$(GCC) $(SOURCE) $(FLAGS) -o $(TARGET)

static:
	$(GCC) $(SOURCE) $(FLAGS) $(STATICFLAGS) -o $(TARGET)


clean:
	rm -f $(TARGET)

