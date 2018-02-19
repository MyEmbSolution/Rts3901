include_directories(after .)

set(target_t ft2transfer)
set(src_t transfer_net.c tr_buffer.c)
add_library(${target_t} STATIC ${src_t})

set(target_p ft2protocol)
set(src_p protocol.c)
add_library(${target_p} STATIC ${src_p})
target_link_libraries(${target_p} ${target_t})

set(target_tester tester)
set(src_tester tester.c)
add_library(${target_tester} STATIC ${src_tester})
target_link_libraries(${target_tester} ${target_p})

install(TARGETS ${target_tester} ${target_p} ${target_t}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
