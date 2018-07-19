include ../../htx.mk

TARGET=hxesctu 

LDFLAGS +=

OBJ_SUFF=.o

vpath = %.% ../hxefpu64/

OBJECTS = framework${OBJ_SUFF} \
                  bfp_global${OBJ_SUFF} \
                  dfp_global${OBJ_SUFF} \
                  vsx_global${OBJ_SUFF} \
                  vmx_global${OBJ_SUFF} \
                  linux_sync_func${OBJ_SUFF} \
                  cpu_global${OBJ_SUFF} \
				  miscomp_debug${OBJ_SUFF} 

SRCS = $(patsubst %.o, %.c, $(OBJECTS))

CFLAGS +=  -O1 -g -D__HTX_LINUX__ -D__64_LINUX__ -D_GNU_SOURCE -DKERNEL_2_6 -DSCTU -fno-stack-protector

LIBS =-lpthread -lc -lhtx64 -lsimstub

.PHONY: all clean

%.o: ../hxefpu64/%.c 
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

all: ${OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${OBJECTS} ${LIBS} -o ${TARGET}
	${CP} ${TARGET} ${SHIPBIN}


%.o: ../hxefpu64/%.s
	${AS} -c $< -o $@

clean:
	${RM} -f *.o ${TARGET} ${SHIPBIN}/${TARGET}
