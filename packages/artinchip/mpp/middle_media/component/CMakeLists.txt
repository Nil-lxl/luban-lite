set(MM_DEMUXER
    component/mm_demuxer_component.c
)

set(MM_VDEC
    component/mm_vdec_component.c
)
set(MM_VIDEO_RENDER
    component/mm_video_render_component.c
)
set(MM_ADEC
    component/mm_adec_component.c
)

set(MM_AUDIO_RENDER
    component/mm_audio_render_component.c
)

set(MM_CLOCK
    component/mm_clock_component.c
)

add_library(mm SHARED
    ${MM_DEMUXER}
    ${MM_VDEC}
    ${MM_VIDEO_RENDER}
    ${MM_ADEC}
    ${MM_AUDIO_RENDER}
    ${MM_CLOCK}
	OMX_core.c
)

if (AAC_DECODER)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAAC_DECODER")
endif()
target_link_libraries(mm mpp_audio_decoder mpp_decoder mm_base base)

target_include_directories(MM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/component"
	"${CMAKE_CURRENT_SOURCE_DIR}/../audio_decoder/include"
)

if(DEFINED CMAKE_INSTALL_FULL_LIBDIR)
    install(TARGETS MM RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
endif() # CMAKE_INSTALL_FULL_LIBDIR

