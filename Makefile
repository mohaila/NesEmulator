.PHONY: clean build test coverage
.ONESHELL:

BUILDIR:=buildir
GEN:=Ninja

clean:
	@rm -rf $(BUILDIR)

build:
	@cmake -S. -G$(GEN) -B$(BUILDIR)
	@cmake --build $(BUILDIR)

test:
	@cmake -DENABLE_TESTS=ON -S. -G$(GEN) -B$(BUILDIR)
	@cmake --build $(BUILDIR)
	@ctest --verbose --timeout 10 --test-dir $(BUILDIR)	

coverage:
	@cmake -DENABLE_COVERAGE=ON -DENABLE_TESTS=ON -S. -G$(GEN) -B$(BUILDIR)
	@cmake --build $(BUILDIR)
	@ctest --verbose --timeout 10 --test-dir $(BUILDIR)
	@cd $(BUILDIR) && ninja coverage