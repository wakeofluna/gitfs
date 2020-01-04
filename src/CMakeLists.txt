set(SOURCE_FILES
	command_line.cpp
	fs_branch.cpp
	fs_commit_link.cpp
	fs_entry.cpp
	fs_pseudo_directory.cpp
	fs_pseudo_entry.cpp
	fs_root.cpp
	git_context.cpp
	git_wrappers.cpp
	logger.cpp
	main.cpp
	mount_context.cpp
	mount.cpp
	umount.cpp
)

find_package(PkgConfig)
pkg_check_modules(FUSE REQUIRED fuse3)
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
	PUBLIC -DFUSE_USE_VERSION=30
)
add_custom_command(TARGET gitfs POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink gitfs ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mount.gitfs
	BYPRODUCTS mount.gitfs
	VERBATIM
)
