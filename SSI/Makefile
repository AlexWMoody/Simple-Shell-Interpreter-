.phony all:
all: ssi inf args

ssi: ssi.c
	gcc ssi.c -lreadline -lhistory -ltermcap -o ssi

inf: inf.c
	gcc inf.c -o inf

args: args.c
	gcc args.c -o args

.PHONY clean:
clean:
	-rm -rf *.o *.exe
