include ../../htx.mk

TARGET= hxerng 

LDFLAGS +=

OBJ_SUFF=.o
OBJECTS = test_utils${OBJ_SUFF} p9_darn${OBJ_SUFF} hxerng${OBJ_SUFF} 
 
SRCS = $(patsubst %.o, %.c, $(OBJECTS))

CFLAGS += -g -D__HTX_LINUX__ -D__64_LINUX__ -D_GNU_SOURCE -DKERNEL_2_6 -D__HTX_OPEN_SOURCE__
 
LIBS = -lhtx64 -lm -lpthread

.PHONY: all clean

all: ${OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${OBJECTS} ${LIBS} -o ${TARGET}
	${CP} ${TARGET} ${SHIPBIN}

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

clean:
	${RM} -f *.o ${TARGET} ${SHIPBIN}/${TARGET}
