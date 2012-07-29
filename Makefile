CFLAGS=-g -O2 -Wall -Wextra -Isrc -rdynamic -DNDEBUG $(OPTFLAGS)
LIBS=/usr/lib/libevent.a
PREFIX?=/usr/local 
 
SOURCES=$(wildcard src/**/*.c src/*.c) 
OBJECTS=$(patsubst %.c,%.o,$(SOURCES)) 
 
TEST_SRC=$(wildcard tests/*_tests.c) 
TESTS=$(patsubst %.c,%,$(TEST_SRC)) 

PROGRAMS_SRC=$(wildcard bin/*.c)
PROGRAMS=$(patsubst %.c,%,$(PROGRAMS_SRC))
 
TARGET=build/lib_zeus.a 
SO_TARGET=$(patsubst %.a,%.so,$(TARGET)) 

 
# The Target Build 
all: clean $(TARGET) $(SO_TARGET) tests $(PROGRAMS) 

dev: CFLAGS=-g -Wall -Isrc -Wall -Wextra $(OPTFLAGS) 
dev: clean all 
 
$(TARGET): CFLAGS += -fPIC 
$(TARGET): build $(OBJECTS) 
	ar rcs $@ $(OBJECTS) 
	ranlib $@ 
 
$(SO_TARGET): $(TARGET) $(OBJECTS) 
	$(CC) -shared -o $@ $(OBJECTS)

$(PROGRAMS):	 
	$(CC) $(CFLAGS) $(PROGRAMS_SRC) $(TARGET) $(LIBS) -o $@ -lrt

$(TESTS): 
	$(foreach file, $(TEST_SRC), \
		$(CC) $(CFLAGS) $(file) $(TARGET) $(LIBS) -o $(subst .c,,$(file)) -lrt; \
	)

build: 
	@mkdir -p build 
	@mkdir -p bin 

# The Unit Tests 
.PHONY: tests 
#tests: CFLAGS += $(TARGET)
tests: $(TESTS)
	sh ./tests/runtests.sh 
 
valgrind: 
	VALGRIND="valgrind --log-file=/tmp/valgrind-%p.log" $(MAKE) 
 
# The Cleaner 
clean: 
	rm -rf build $(OBJECTS) $(TESTS) $(PROGRAMS) 
	rm -f tests/tests.log 
	find . -name "*.gc*" -exec rm {} \; 
	rm -rf `find . -name "*.dSYM" -print` 
 
# The Install 
install: all 
	install -d $(DESTDIR)/$(PREFIX)/lib/ 
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib/ 

# The Checker 
BADFUNCS='[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)' 
check: 
	@echo Files with potentially dangerous functions. 
	@egrep $(BADFUNCS) $(SOURCES) || true 
