CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(fdupves C CXX)

SET(PROJECT_MAJOR "3")
SET(PROJECT_MINOR "0")
SET(PROJECT_PATCH "0")
SET(PROJECT_ICON "abella.danger.png")

ADD_DEFINITIONS(
        -DPACKAGE="${PROJECT_NAME}"
        -DPACKAGE_STRING="${PROJECT_NAME} ${PROJECT_MAJOR}.${PROJECT_MINOR}.${PROJECT_PATCH}"
        -DPROJECT_MAJOR="${PROJECT_MAJOR}"
        -DPROJECT_MINOR="${PROJECT_MINOR}"
        -DPROJECT_PATCH="${PROJECT_PATCH}"
        -DPROJECT_ICON="${PROJECT_ICON}"
        -DCMAKE_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}"
)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(GTK gtk+-3.0 REQUIRED)
IF (WIN32)
    SET(VCPKG_INSTALLED_DIR "C:/vcpkg/installed"
            CACHE PATH
            "vcpkg installed dir")
    SET(VCPKG_INSTALLED_X64_WINDOWS ${VCPKG_INSTALLED_DIR}/x64-windows)

    SET(FFMPEG_INCLUDE_DIRS ${VCPKG_INSTALLED_X64_WINDOWS}/include)
    SET(FFMPEG_LIBRARY_DIRS ${VCPKG_INSTALLED_X64_WINDOWS}/lib)
    SET(FFMPEG_LIBRARIES avformat.lib avcodec.lib avutil.lib swscale.lib swresample.lib)
    SET(OPENCV_INCLUDE_DIRS ${VCPKG_INSTALLED_X64_WINDOWS}/include/opencv4)
    SET(OPENCV_LIBRARY_DIRS ${VCPKG_INSTALLED_X64_WINDOWS}/lib)
    SET(OPENCV_LIBRARIES opencv_imgproc4.lib opencv_core4.lib)

    SET(MUPDF_LIBRARIES libmupdf.lib openjp2.lib jpeg.lib jbig2dec.lib gumbo.lib)
ELSE (WIN32)
    PKG_CHECK_MODULES(FFMPEG libavformat libavcodec libavutil libswscale libswresample REQUIRED)
    PKG_CHECK_MODULES(OPENCV opencv4 REQUIRED)

    SET(MUPDF_INCLUDE_DIRS "/usr/include/mupdf")
    SET(MUPDF_LIBRARIES "-lmupdf")

ENDIF (WIN32)

INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS}
        ${FFMPEG_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/sqlite3
	${OPENCV_INCLUDE_DIRS}
        )
LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}
        ${GTK_LIBRARY_DIRS}
        ${FFMPEG_LIBRARY_DIRS}
	${OPENCV_LIBRARY_DIRS}
        )

IF (WIN32)
    SET(REQ_LIBRARIES windowscodecs.lib)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ELSE (WIN32)
    SET(CMAKE_CXX_FLAGS "-Wall -D_FILE_OFFSET_BITS=64 -Wno-deprecated-declarations")

    OPTION(FDUPVES_ENABLE_DEBUG "If build binary with debug infomations." OFF)
    IF (FDUPVES_ENABLE_DEBUG)
        SET(CMAKE_CXX_FLAGS "-D_DEBUG -g ${CMAKE_C_FLAGS}")
    ELSE (FDUPVES_ENABLE_DEBUG)
        SET(CMAKE_CXX_FLAGS "-O2 -fno-strict-aliasing ${CMAKE_C_FLAGS}")
    ENDIF (FDUPVES_ENABLE_DEBUG)

    OPTION(FDUPVES_ENABLE_MUDFLAP "If build binary with mudflap infomations." OFF)
    IF (FDUPVES_ENABLE_MUDFLAP)
        SET(CMAKE_CXX_FLAGS "-D_MUDFLAP -fmudflap -fmudflapth -funwind-tables -lmudflapth -rdynamic ${CMAKE_C_FLAGS}")
    ENDIF (FDUPVES_ENABLE_MUDFLAP)

    OPTION(FDUPVES_ENABLE_PROFILER "If build binary with google profiler infomations." OFF)
    IF (FDUPVES_ENABLE_PROFILER)
        SET(CMAKE_CXX_FLAGS "-DFDUPVES_ENABLE_PROFILER -lprofiler ${CMAKE_C_FLAGS}")
    ENDIF (FDUPVES_ENABLE_PROFILER)

    OPTION(FDUPVES_ENABLE_GPROF "If build binary with gprof infomations." OFF)
    IF (FDUPVES_ENABLE_GPROF)
        SET(CMAKE_CXX_FLAGS "-pg ${CMAKE_C_FLAGS}")
    ENDIF (FDUPVES_ENABLE_GPROF)
ENDIF (WIN32)

ADD_SUBDIRECTORY(src)

FIND_PACKAGE(Gettext)
IF (GETTEXT_FOUND)
    ADD_SUBDIRECTORY(po)
    IF (!WIN32)
        ADD_DEPENDENCIES(fdupves translate)
    ENDIF ()
ELSE (GETTEXT_FOUND)
    INSTALL(FILES po/zh_CN.gmo
            DESTINATION share/locale/zh_CN/LC_MESSAGES
            RENAME fdupves.mo)
ENDIF (GETTEXT_FOUND)

ADD_EXECUTABLE(sqlite3 sqlite3/sqlite3.h sqlite3/sqlite3.c sqlite3/shell.c)

INSTALL(DIRECTORY icons
        DESTINATION share/fdupves)
INSTALL(FILES
        fdupvesrc
        DESTINATION etc)
INSTALL(FILES
        COPYING THANKS AUTHORS
        TODO INSTALL NEWS
        DESTINATION share/fdupves)
IF (WIN32)
    #INSTALL(FILES ${VCPKG_INSTALLED_X64_WINDOWS}/LICENSE.txt
    #        DESTINATION .)
ENDIF (WIN32)

SET(CPACK_PACKAGE_VENDOR "Alf, Inc.")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "fdupves - find duplicate video/image files")
SET(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_PATCH})

IF (UNIX)
    SET(CPACK_SET_DESTDIR ON)
    IF (EXISTS /etc/debian_version)
        SET(CPACK_GENERATOR DEB)
        SET(CPACK_PACKAGE_CONTACT "Alf <nhpub@sina.com>")
    ENDIF (EXISTS /etc/debian_version)
    IF (EXISTS /etc/redhat-release)
        SET(CPACK_GENERATOR RPM)
        SET(CPACK_RPM_PACKAGE_REQUIRES "gtk2 >= 2.10.4 ffmpeg")
    ENDIF (EXISTS /etc/redhat-release)

    SET(CPACK_SOURCE_PACKAGE_FILE_NAME
            ${CMAKE_PROJECT_NAME}-${PROJECT_MAJOR}.${PROJECT_MINOR}.${PROJECT_PATCH})
    SET(CPACK_SOURCE_GENERATOR TGZ)
    SET(CPACK_SOURCE_IGNORE_FILES
            ${CMAKE_BINARY_DIR}
            ".git"
            ".gitmodules"
            ".gitignore"
            "~$"
    )
ELSE (UNIX)
    SET(CPACK_GENERATOR NSIS)
    #  SET (CPACK_PACKAGE_ICON ${NSISDIR}/share/icons/fdupves.ico)
    #  SET (PACK_NSIS_INSTALLED_ICON_NAME ${NSISDIR}/share/icons/fdupves.ico)
    SET(CPACK_NSIS_MUI_ICON ${CMAKE_SOURCE_DIR}/icons/fdupves.ico)
    SET(CPACK_NSIS_CONTACT "Alf <naihe2010@126.com>")
    SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '\$DESKTOP\\\\fdupves.lnk' '\$INSTDIR\\\\bin\\\\fdupves.exe'")
    SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '\$DESKTOP\\\\fdupves.lnk'")
ENDIF (UNIX)

INCLUDE(CPack)
