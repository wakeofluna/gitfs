set(SOURCE_FILES
	gitfs.c
	mount.c
	umount.c
)

find_package(PkgConfig)
pkg_check_modules(FUSE REQUIRED fuse)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)

add_executable(gitfs ${SOURCE_FILES})
target_link_libraries(gitfs
	${LIBGIT2_LIBRARIES}
	${FUSE_LIBRARIES}
)
target_include_directories(gitfs
	PUBLIC ${LIBGIT2_INCLUDE_DIRS}
	PUBLIC ${FUSE_INCLUDE_DIRS}
)
target_compile_options(gitfs
	PUBLIC ${LIBGIT2_CFLAGS_OTHER}
	PUBLIC ${FUSE_CFLAGS_OTHER}
)
add_custom_command(TARGET gitfs POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink gitfs mount.gitfs
	BYPRODUCTS mount.gitfs
	VERBATIM
)
