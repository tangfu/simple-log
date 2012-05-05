ALL: log.c queue.c log.h
	@-astyle -n --style=linux --mode=c --pad-oper --pad-paren-in --unpad-paren --break-blocks --delete-empty-lines --min-conditional-indent=0 --max-instatement-indent=80 --indent-col1-comments --indent-switches --lineend=linux *.{c,h} 1>/dev/null
	@gcc -c log.c queue.c -lpthread -g3 -Wall -Wextra -fstack-protector
	@ar rc	liblog.a log.o queue.o
	@rm *.o
	@make -C example
#	@gcc log.c queue.c -fPIC -shared -o libtaskpool.so	
#	produce document
	@doxygen

release:	log.c queue.c log.h
	@gcc -c log.c queue.c -lpthread -Wall -Wextra -fstack-protector
	@ar rc	liblog.a log.o queue.o
	@rm *.o
		
clean:
	@if [ -f liblog.a ];then \
		rm liblog.a example/example example/debug.log example/test.log 2>/dev/null; \
		rm -rf doc/html/* 2>/dev/null; \
	fi
		
