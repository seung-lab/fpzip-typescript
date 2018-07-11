all:
	node-gyp configure build

asm:
	node-gyp configure build --asmjs=1