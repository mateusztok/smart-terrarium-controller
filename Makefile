CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDLIBS = -lpistache -lsqlite3 -pthread

BACKEND_SOURCES = backend/main.cpp backend/database.cpp \
	backend/measurement_handler.cpp backend/form_data.cpp
BACKEND_BINARY = build/backend
FRONTEND_DIR = frontend

.PHONY: all backend frontend frontend-install run-backend run-frontend clean

all: backend

backend: $(BACKEND_BINARY)

$(BACKEND_BINARY): $(BACKEND_SOURCES) backend/database.h \
	backend/form_data.h backend/measurement_handler.h backend/models.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(BACKEND_SOURCES) -o $(BACKEND_BINARY) $(LDLIBS)

frontend:
	npm --prefix $(FRONTEND_DIR) run build

frontend-install:
	npm --prefix $(FRONTEND_DIR) ci

run-backend: $(BACKEND_BINARY)
	./$(BACKEND_BINARY)

run-frontend:
	npm --prefix $(FRONTEND_DIR) run dev -- --host 0.0.0.0

clean:
	rm -rf build
