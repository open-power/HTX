include ../../htx.mk

TARGET= hxediag

LDFLAGS +=

OBJ_SUFF=.o
hxediag_OBJECTS = hxediag.o

hxediag_SRCS = $(patsubst %.o, %.c, $(hxediag_OBJECTS))

all: hxediag

CFLAGS += -D__RELGR2600__
LIBS = -lpthread -lhtx64
 
.PHONY: all clean

hxediag: ${hxediag_OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${hxediag_OBJECTS} ${LIBS} -o hxediag 
	${CP} hxediag ${SHIPBIN}

move_scripts:
	mkdir -p ${SCRIPTS}
	${CP} ib_info.pl ${SCRIPTS}/

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

clean:
	${RM} -f *.o ${TARGET}
	${RM} -f ${SHIPBIN}/hxediag
