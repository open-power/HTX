include ../../htx.mk

.PHONY: all clean

all: hxemem64_new hxemem64_old  

vpath = %.% ../hxecache/

LDFLAGS +=

OBJ_SUFF=.o
hxemem64_new_OBJECTS =  \
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

hxemem64_old_OBJECTS =  \
        hxemem${OBJ_SUFF} \
        mem_operation${OBJ_SUFF} \
        rand_operation${OBJ_SUFF} \
        pat_operation${OBJ_SUFF} \
        gen_tlbie${OBJ_SUFF} \
        test_L4${OBJ_SUFF}


 
#SRCS = $(patsubst %.o, %.c, ../hxecache/%.s, $(OBJECTS))

CFLAGS += -D__64_LINUX__ -D_GNU_SOURCE -DKERNEL_2_6 -D_REENTRANT -g  -D__HTX_LINUX_ -DTRUE=1 -DFALSE=0 -std=c99
 
LIBS += -Wl,--start-group
LIBS += -lhtx64 -lpthread
LIBS += -Wl,--end-group

.PHONY: all clean


hxemem64_old: ${hxemem64_old_OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${hxemem64_old_OBJECTS} ${LIBS} -o hxemem64_old
	${CP} hxemem64_old ${SHIPBIN}

hxemem64_new: ${hxemem64_new_OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${hxemem64_new_OBJECTS} ${LIBS} -o hxemem64_new
	${CP} hxemem64_new ${SHIPBIN}
	${CP} hxemem64_new ${SHIPBIN}/hxecache_new

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

%.o: ../hxecache/%.s
	${AS} -c $< -o $@

clean:
	#${RM} -f *.o ${TARGET} 
	${RM} -f *.o hxemem64_new hxemem64_old 
	${RM} -f ${SHIPBIN}/hxemem64_new ${SHIPBIN}/hxemem64_old ${SHIPBIN}/hxecache_new
