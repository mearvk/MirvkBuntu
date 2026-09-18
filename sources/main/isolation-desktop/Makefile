.PHONY: all bootstrap compile run clean

all: compile

bootstrap:
	./bootstrap-gradle

compile: bootstrap
	./compile-desktop

run: compile
	./launch-desktop

clean:
	rm -rf build
