all:
	mkdir -p build
	c++ -o build/libtest.so -shared -fPIC -g -ggdb src/main.cpp -llibmem
	javac target/main/Main.java target/main/MyClass.java
