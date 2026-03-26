CXX = g++
CXXFLAGS = -std=c++17 -DUNIT_TEST -I../../test/mocks/ -I../../src/ -I../../include/

TEST_SRC = test_main.cpp test_support.cpp ../../src/ObdManager.cpp ../../src/WebConsole.cpp
TEST_EXE = test_obd_manager

all: run

$(TEST_EXE): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

run: $(TEST_EXE)
	./$(TEST_EXE)

clean:
	rm -f $(TEST_EXE)

.PHONY: all run clean
