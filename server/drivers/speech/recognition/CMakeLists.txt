PLAYERDRIVER_OPTION (sphinx2 build_sphinx2 OFF "Disabled by default")
PLAYERDRIVER_REQUIRE_HEADER (sphinx2 build_sphinx2 sphinx2/ckd_alloc.h)
PLAYERDRIVER_ADD_DRIVER (sphinx2 build_sphinx2 LINKFLAGS -lsphinx2 -lsphinx2fe -lsphinx2ad
                         SOURCES sphinx2.cc)
