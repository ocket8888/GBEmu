all: myemu

myemu: myemu.o
	g++ -o /home/ocket8888/bin/myemu myemu.o; rm myemu.o

myemu.o: 
	g++ -Wall -o myemu.o -c myemu.cpp

# myemu.pre:
# 	g++ -P -E myemu.cpp -o myemu.pre

pre: myemu.pre

obj: myemu.o

.PHONY: clean

clean:
	rm ./myemu.o /home/ocket8888/bin/myemu