all:
	gcc -Wall -Werror wish.c -o wish -fsanitize=address & ./tester/run-tests.sh
clean:
	rm wish
	rm -rf tests-out/
