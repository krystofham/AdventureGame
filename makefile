all:
	gcc src/main.c -Wall -Werror -o test
test:
	python tests/*.py
clean:
	rm -f test
