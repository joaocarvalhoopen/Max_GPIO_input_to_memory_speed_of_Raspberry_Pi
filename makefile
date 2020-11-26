

clean:
	rm gpio_in_active_poll

all:
	gcc -O3 -Wall -o gpio_in_active_poll gpio_in_active_poll.c

run:
	sudo ./gpio_in_active_poll


