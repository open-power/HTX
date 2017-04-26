include ../../htx.mk

TARGET=hxetlbie

LDFLAGS +=

OBJ_SUFF=.o

vpath= ../hxemem64/

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

CFLAGS += -D__64_LINUX__ -D_GNU_SOURCE -DKERNEL_2_6 -D_REENTRANT -g -D__HTX_LINUX__ -I./../hxemem64 -std=c99
 
LIBS += -lpthread -lhtx64

.PHONY: all clean

%.o: ../hxemem64/%.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

%.o: ../hxemem64/%.s
	${AS} -c $< -o $@

all: ${OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${OBJECTS} ${LIBS} -o ${TARGET}
	${CP} ${TARGET} ${SHIPBIN}/${TARGET}

clean:
	${RM} -rf *.o ${TARGET} ${SHIPBIN}/${TARGET}

