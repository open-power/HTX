include ../../htx.mk

SUBDIRS= utils
TARGET= hxecd 
all: ${TARGET} ${SUBDIRS}

LDFLAGS +=

OBJ_SUFF=.o
OBJECTS = hxecd${OBJ_SUFF} \
                        get_rule${OBJ_SUFF} \
                        proc_rule${OBJ_SUFF} \
                        buf_oper${OBJ_SUFF} \
                        io_oper${OBJ_SUFF} \
                        prt_msg${OBJ_SUFF} \
                        rnd_oper${OBJ_SUFF} \
                        seq_oper${OBJ_SUFF}
 
SRCS = $(patsubst %.o, %.c, $(OBJECTS))
SUBDIRS_CLEAN = $(patsubst %,%.clean,$(SUBDIRS))

CFLAGS += -D_LARGEFILE64_SOURCE -D__DEFINE_PAGE_MACROS__  -DFALSE=0 -DTRUE=1
 
LIBS = -lpthread -lc -lhtx64 

.PHONY: all clean ${SUBDIRS} ${SUBDIRS_CLEAN}

${TARGET}: ${OBJECTS}
	${CC} ${LDFLAGS} ${LIBPATH} ${OBJECTS} ${LIBS} -o ${TARGET} 
	${CP} ${TARGET} ${SHIPBIN}

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} -c $< -o $@

${SUBDIRS}:
	$(MAKE) -C $@

clean: ${SUBDIRS_CLEAN}
	${RM} -f *.o ${TARGET} ${SHIPBIN}/${TARGET}

${SUBDIRS_CLEAN}:
	@$(MAKE) -C $(@:.clean=) clean

%.clean: %
	@$(MAKE) -C $< clean
