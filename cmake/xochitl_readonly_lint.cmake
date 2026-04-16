
find_program(BASH_COMMAND bash)
find_program(GREP_COMMAND grep)

if(BASH_COMMAND AND GREP_COMMAND)
    set(XOCHITL_LINT_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/xochitl_readonly_lint.sh")

    execute_process(
        COMMAND chmod +x "${XOCHITL_LINT_SCRIPT}"
        RESULT_VARIABLE _chmod_result
        ERROR_QUIET
    )

    add_custom_target(xochitl_readonly_lint
        COMMAND "${BASH_COMMAND}" "${XOCHITL_LINT_SCRIPT}" "${CMAKE_SOURCE_DIR}/src"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "xochitl_readonly_lint, enforce XOCH-10 read-only invariant (see cmake/xochitl_readonly_lint.cmake)"
        VERBATIM
    )

    add_dependencies(readmarkable xochitl_readonly_lint)
else()
    if(NOT BASH_COMMAND)
        message(WARNING "bash not found, xochitl_readonly_lint (XOCH-10) disabled")
    endif()
    if(NOT GREP_COMMAND)
        message(WARNING "grep not found, xochitl_readonly_lint (XOCH-10) disabled")
    endif()
endif()
