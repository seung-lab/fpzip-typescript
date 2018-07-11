all: clean node types

clean:
	touch build/Release/nbind.js
	rm build/Release/nbind*

node: 
	node-gyp configure build

asm: clean
	node-gyp configure build --asmjs=1 

types: asm
	npm run -s -- ndts . > lib-types.d.ts