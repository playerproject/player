SET (replaceSrcs dummy.c)
# SET (replaceSrcs dirname.c poll.c replace.h cfmakeraw.c round.c compressBound.c)

ADD_LIBRARY (replace STATIC ${replaceSrcs})