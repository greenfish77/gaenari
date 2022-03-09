#######
# c++17
#######
macro(check_cpp17_gaenari)
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED True)
    set(CMAKE_CXX_EXTENSIONS OFF)
    if(MSVC_VERSION GREATER_EQUAL 1910)
        # for using `and`, `or` keyword.
        add_compile_options(/permissive-)
    endif()
endmacro()

#######################
# to build with gaenari
#######################
macro(add_gaenari target)
    # add include and link, ...
    target_include_directories(${target} PRIVATE ${GAENARI_INCLUDE_DIR_FOR_BUILD})
    set_property(TARGET ${target} APPEND PROPERTY COMPILE_DEFINITIONS ${GAEANRI_DEFINITION_FOR_BUILD})
    set_property(TARGET ${target} APPEND PROPERTY LINK_LIBRARIES ${GAENARI_LINK_FOR_BUILD})
endmacro()
