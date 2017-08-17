include ../../htx.mk


TARGET = hxecache_old  

vpath= ../hxemem64

LDFLAGS +=

OBJ_SUFF=.o
OBJECTS = hxecache${OBJ_SUFF} \
        prefetch_irritator${OBJ_SUFF} \
        prefetch${OBJ_SUFF} \
        prefetch_dcbtt${OBJ_SUFF} \
        prefetch_dcbtp${OBJ_SUFF} \
        prefetch_n_stride${OBJ_SUFF} \
        prefetch_dcbtna${OBJ_SUFF}
 
hxecache_new_OBJECTS = \
    nest_framework${OBJ_SUFF} \
    nest_read_rules${OBJ_SUFF} \
    parse_filters${OBJ_SUFF} \
    fabricbus_setup${OBJ_SUFF} \
    mem${OBJ_SUFF} \
    tlbie_irritator${OBJ_SUFF} \
    exer_cache${OBJ_SUFF} \
    prefetch_operation${OBJ_SUFF} \
    prefetch${OBJ_SUFF} \
    prefetch_dcbtna${OBJ_SUFF} \
    prefetch_dcbtp${OBJ_SUFF} \
    prefetch_dcbtt${OBJ_SUFF} \
    prefetch_n_stride${OBJ_SUFF} \
    memory_operations${OBJ_SUFF} \
    mem_pattern_file_operations${OBJ_SUFF} \
    mem_random_pat_operations${OBJ_SUFF} \
    stride_operation${OBJ_SUFF} 

SRCS = $(patsubst %.o, %.c, $(OBJECTS))
#hxecache_new_SRCS = $(patsubst %.o, %.c, $(hxecache_new_OBJECTS))

CFLAGS += -D__64_LINUX__ -D_GNU_SOURCE -DKERNEL_2_6 -D_REENTRANT -D__HTX_LINUX__ -O2

LIBS = -lhtx64 -lpthread -lm

.PHONY: all clean

all: ${OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${OBJECTS} ${LIBS} -o ${TARGET}
	${CP} ${TARGET} ${SHIPBIN}

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

clean:
	${RM} -f *.o ${TARGET} ${SHIPBIN}/${TARGET}
