#add_subdirectory(tests)

add_library(skeldal_sdl 
            sdl_context.cpp 
            BGraph2.cpp 
            input.cpp 
            sound.cpp
            sound_filter.cpp
            pixel_scaler.cpp
            post_process.cpp)
set_property(TARGET skeldal_sdl PROPERTY CXX_STANDARD 20)
