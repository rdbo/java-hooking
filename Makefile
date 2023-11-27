all:
	mkdir -p build
	c++ -o build/libtest.so -shared -fPIC src/main.cpp -llibmem
	javac target/main/Main.java
