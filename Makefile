SHELL=/bin/bash
CC=gcc
CPP=g++
CCSTD=-std=gnu11
CXXSTD=-std=gnu++11
CFLAGS=-O3 -fdiagnostics-color=auto -pthread -g $(CCSTD)
CXXFLAGS=$(filter-out $(CCSTD), $(CFLAGS)) $(CXXSTD) -fno-exceptions -Wno-write-strings -Wno-pointer-arith
MKDIRS=lib bin tst/bin .pass .pass/tst/bin .pass/tst/scripts .make .make/bin .make/tst/bin .make/lib
INCLUDE=$(addprefix -I,include)
EXECS=$(addprefix bin/,iuclient iuserver iuctl)
BINTESTS=$(addprefix tst/bin/,capitalC concat stream mpsc msg datacenters net todo file node messenger relay iuctl)
SCRIPTTESTS=$(addprefix tst/scripts/, relay)
TESTS=$(BINTESTS) $(SCRIPTTESTS)
SRC=$(wildcard src/*.cpp)
LIBS=$(patsubst src/%.cpp, lib/%.o, $(SRC))


.PHONY: default all clean again check distcheck dist-check
.SECONDARY:
default: all
all: $(EXECS) $(TESTS)
$(addprefix .pass/,$(SCRIPTTESTS)): $(EXECS)
clean:
	rm -rf $(MKDIRS)
again: clean all
check: $(addprefix .pass/,$(TESTS))

FNM=\([a-z_A-Z/]*\)
.make/%.d: %.c
	@mkdir -p $(@D)
	@$(CC) -MM $(CCSTD) $(INCLUDE) $< -o $@
.make/%.d: %.cpp
	@mkdir -p $(@D)
	$(CPP) -MM $(CXXSTD) $(INCLUDE) $< -o $@
.make/lib/%.o.d: .make/src/%.d | .make/lib
	@sed 's/$(FNM)\.o/lib\/\1.o/g' $< > $@
.make/bin/%.d: .make/%.d | .make/bin
	@sed 's/include\/$(FNM).h/lib\/\1.o/g' $< > $@
	@sed -i 's/$(FNM).o:/bin\/\1:/g' $@
	@perl make/depend.pl $@ > $@.bak
	@mv $@.bak $@
.make/tst/bin/%.d: .make/tst/%.d | .make/tst/bin
	@sed 's/include\/$(FNM).h/lib\/\1.o/g' $< > $@
	@sed -i 's/$(FNM).o:/tst\/bin\/\1:/g' $@
	@perl make/depend.pl $@ > $@.bak
	@mv $@.bak $@
MAKES=$(addsuffix .d,$(addprefix .make/, $(EXECS) $(TESTS) $(LIBS)))
-include $(MAKES)
distcheck dist-check:
	@rm -rf .pass
	@make --no-print-directory check
.pass/tst/bin/%: tst/bin/% | .pass/tst/bin
	@printf "\033[0;33mRUNNING\033[0m| $< "
	@$<\
		&& echo -e "\r\033[0;32mPASS\033[0m   " && touch $@\
		|| echo -e "\r\033[0;31mFAIL\033[0m   "
.pass/tst/scripts/%: tst/scripts/% | .pass/tst/scripts
	@printf "\033[0;33mRUNNING\033[0m| $< "
	@$<\
		&& echo -e "\r\033[0;32mPASS\033[0m   " && touch $@\
		|| echo -e "\r\033[0;31mFAIL\033[0m   "
.pass/tst/scripts/%: tst/scripts/%.sh | .pass/tst/scripts
$(MKDIRS):
	@mkdir -p $@
$(EXECS): | bin
bin/%: %.cpp
	$(CPP) $(CXXFLAGS) $(INCLUDE) $^ -o $@
bin/%: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@
lib/%.o: src/%.cpp include/%.h | lib
	$(CPP) -c $(CXXFLAGS) $(INCLUDE) $< -o $@
lib/%.o: src/%.c include/%.h | lib
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@
tst/bin/%: tst/%.cpp | tst/bin
	$(CPP) $(CXXFLAGS) $(INCLUDE) $^ -o $@
tst/bin/%: tst/%.c | tst/bin
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@
