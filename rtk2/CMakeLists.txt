IF (WITH_GTK AND INCLUDE_RTK)
    SET (rtkSrcs rtk.c
            rtk_canvas.c
            rtk_canvas_movie.c
            rtk_fig.c
            rtk_region.c
            rtk_menu.c
            rtk_table.c)

    ADD_LIBRARY (rtk STATIC ${rtkSrcs})
    SET_SOURCE_FILES_PROPERTIES (${rtkSrcs} PROPERTIES
        COMPILE_FLAGS "${GNOMECANVAS_CFLAGS} ${GTK_CFLAGS} ${GDKPIXBUF_CFLAGS}")
    SET_TARGET_PROPERTIES (rtk PROPERTIES
        LINK_FLAGS "${GNOMECANVAS_LINKFLAGS} ${GTK_LINKFLAGS} ${GDKPIXBUF_LINKFLAGS}")
ENDIF (WITH_GTK AND INCLUDE_RTK)