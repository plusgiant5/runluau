all: build

build: $(patsubst %.cpp, build/%.o, $(wildcard *.cpp))

build/%.o: %.cpp
	gcc -c -o $@ $< -I$(LUAUSRC)/CodeGen/include \
		-I$(LUAUSRC)/Common/include \
		-I$(LUAUSRC)/VM/include \
		-I$(LUAUSRC)/Compiler/include \
		-I$(LUAUSRC)/Ast/include \
		-Ishared

clean:
	rm -rf build