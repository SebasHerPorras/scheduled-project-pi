# Compilador
CXX := g++
CXXFLAGS := -g -c -std=c++11

# Directorios
SRC := .
OBJDIR := obj
BINDIR := exec

VPATH := $(SRC)

$(shell mkdir -p $(OBJDIR) $(BINDIR))

# Archivos fuente
SRCS_COMMON := Socket.cc VSocket.cc
SRCS_CLIENT := cliente.cpp
SRCS_FIGURA := figura_discovery.cpp file_system.cpp
SRCS_TENEDOR := tenedor_discovery.cpp file_system.cpp

# Objetos comunes
OBJS_COMMON := $(SRCS_COMMON:%.cc=$(OBJDIR)/%.o)

# Objetos por ejecutable
OBJS_CLIENT := $(OBJDIR)/cliente.o $(OBJS_COMMON)
OBJS_FIGURA := $(OBJDIR)/figura_discovery.o $(OBJDIR)/file_system.o $(OBJS_COMMON)
OBJS_TENEDOR := $(OBJDIR)/tenedor_discovery.o $(OBJDIR)/file_system.o $(OBJS_COMMON)

# Ejecutables
TARGET_CLIENT := $(BINDIR)/cliente.out
TARGET_FIGURA := $(BINDIR)/figura_discovery.out
TARGET_TENEDOR := $(BINDIR)/tenedor_discovery.out

TARGETS := $(TARGET_CLIENT) $(TARGET_FIGURA) $(TARGET_TENEDOR)

.PHONY: all clean run client figuras tenedor listar extraer

# Regla por defecto
all: $(TARGETS)

# Compilación genérica para .cpp
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Compilación genérica para .cc
$(OBJDIR)/%.o: %.cc | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Enlace de ejecutables
$(TARGET_CLIENT): $(OBJS_CLIENT) | $(BINDIR)
	$(CXX) $^ -o $@

$(TARGET_FIGURA): $(OBJS_FIGURA) | $(BINDIR)
	$(CXX) $^ -o $@

$(TARGET_TENEDOR): $(OBJS_TENEDOR) | $(BINDIR)
	$(CXX) $^ -o $@

# Crear directorios si no existen
$(OBJDIR) $(BINDIR):
	mkdir -p $@

# Targets individuales
client: $(TARGET_CLIENT)
figuras: $(TARGET_FIGURA)
tenedor: $(TARGET_TENEDOR)

# Limpieza
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Ejecución simple
run:
	@echo "Uso: make run_[client|figuras|tenedor|listar|extraer]"

run_client: client
	./$(TARGET_CLIENT)

run_figuras: figuras
	./$(TARGET_FIGURA)

run_tenedor: tenedor
	./$(TARGET_TENEDOR)
