all:
	g++ -g -o alarm action.cpp main.cpp panel.cpp state.cpp parser.cpp connection.cpp log.cpp ui.cpp -lpthread

clean:
	rm -f alarm
