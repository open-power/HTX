include ../../htx.mk

TARGET=hxefabricbus

LDFLAGS +=

OBJ_SUFF=.o

vpath = %.% ../hxemem64/

OBJECTS =  \
    nest_framework${OBJ_SUFF} \
    nest_read_rules${OBJ_SUFF} \
    parse_filters${OBJ_SUFF} \
    mem${OBJ_SUFF} \
    tlbie_irritator${OBJ_SUFF} \
    fabricbus_setup${OBJ_SUFF} \
    memory_operations${OBJ_SUFF} \
    mem_pattern_file_operations${OBJ_SUFF} \
    mem_random_pat_operations${OBJ_SUFF} \
    stride_operation${OBJ_SUFF}


SRCS = $(patsubst %.o, %.c, $(OBJECTS))

CFLAGS += -D__64_LINUX__ -D_GNU_SOURCE -DKERNEL_2_6 -D_REENTRANT -g -O2 -D__HTX_LINUX_ -DTRUE=1 -DFALSE=0 -I./../hxemem64 -std=c99

LIBS = -lhtx64 -lpthread

.PHONY: all clean

%.o: ../hxemem64/%.c 
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

all: ${OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${OBJECTS} ${LIBS} -o ${TARGET}
	${CP} ${TARGET} ${SHIPBIN}


%.o: ../hxemem64/%.s
	${AS} -c $< -o $@

clean:
	${RM} -f *.o ${TARGET} ${SHIPBIN}/${TARGET}
