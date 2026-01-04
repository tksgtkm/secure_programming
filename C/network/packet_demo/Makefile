OUTDIR = ./build
TARGET = $(OUTDIR)/packet
SOURCES = $(wildcard *.cc)
OBJECTS = $(addprefix $(OUTDIR)/, $(SOURCES:.cc=.o))

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(TARGET)

.PHONY: run
run: $(TARGET)
	./build/packet

$(TARGET): $(OBJECTS) Makefile
	$(CXX) -o $(TARGET) $(OBJECTS)

$(OUTDIR)/%.o: %.cc Makefile
	mkdir -p build
	$(CXX) -o $@ -c $<