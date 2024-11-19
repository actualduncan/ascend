# includes from https://github.com/KStocky/ShaderTestFramework
include(nuget)
include(AssetDependencyManagement)
add_executable(ascend WIN32)
asset_dependency_init(ascend)

fetch_nuget_package(
    Microsoft.Direct3D.D3D12
    1.613.3
	AGILITY_BINARY
)
fetch_nuget_package(
    Microsoft.Direct3D.DXC
    1.8.2403.18
	DXC_BINARY
)
fetch_nuget_package(
    Microsoft.Direct3D.WARP
    1.0.13
	WARP_BINARY
)

set(SOURCES ascend/main.cpp 
			ascend/ascendHelpers.h
			ascend/Renderer.cpp 
			ascend/Renderer.h 
			ascend/RayTracingRHI.cpp
			ascend/RayTracingRHI.h 
			ascend/WindowsApplication.cpp 
			ascend/WindowsApplication.h
			ascend/imgui/imconfig.h
			ascend/imgui/imgui.cpp
			ascend/imgui/imgui.h
			ascend/imgui/imgui_internal.h
			ascend/imgui/imgui_demo.cpp
			ascend/imgui/imgui_draw.cpp
			ascend/imgui/imgui_tables.cpp
			ascend/imgui/imgui_widgets.cpp
			ascend/imgui/imstb_rectpack.h
			ascend/imgui/imstb_textedit.h
			ascend/imgui/imstb_truetype.h
			ascend/imgui/imgui_impl_dx12.cpp
			ascend/imgui/imgui_impl_dx12.h
			ascend/imgui/imgui_impl_win32.cpp
			ascend/imgui/imgui_impl_win32.h
			
)

# TODO: Move to .cmake file for shader compiling - Reduce code duplication.
set(HLSL_LIB_SHADER_FILES ascend/Shader/Raytracing.hlsl
						  ascend/Shader/WorkGraphRaytracing.hlsl)
set(HLSL_CS_SHADER_FILES  ascend/Shader/ComputeRaytracing.hlsl)
set(SHADER_FILES ${HLSL_LIB_SHADER_FILES} 					  
				 ${HLSL_CS_SHADER_FILES}
				 ascend/Shader/HlslCompat.h
				 ascend/Shader/RaytracingHlslCompat.h)  
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ascend/Shader)

set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_TYPE lib)
set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_MODEL 6_8)
set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_ENTRY_POINT CSMain)
set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_VARIABLE_NAME "g_p%(Filename)")
set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_OUTPUT_HEADER_FILE "$(IntDir)CompiledShaders\%(Filename).hlsl.h")

set_source_files_properties(${HLSL_CS_SHADER_FILES} PROPERTIES VS_SHADER_TYPE cs)
set_source_files_properties(${HLSL_CS_SHADER_FILES} PROPERTIES VS_SHADER_MODEL 6_5)
set_source_files_properties(${HLSL_CS_SHADER_FILES} PROPERTIES VS_SHADER_VARIABLE_NAME "g_p%(Filename)")
set_source_files_properties(${HLSL_CS_SHADER_FILES} PROPERTIES VS_SHADER_OUTPUT_HEADER_FILE "$(IntDir)CompiledShaders\%(Filename).hlsl.h")

set(SHADER_SOURCE_REL_DIR "Shaders")

# Compile shaders using dxc
add_custom_target(CompileShaders)

file(MAKE_DIRECTORY ${SHADER_SOURCE_DIR}/CompiledShaders)

foreach(FILE ${HLSL_LIB_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  list(APPEND CSO_SHADER_FILES ${CMAKE_BINARY_DIR}/${FILE_WE}.cso)
	add_custom_command(TARGET CompileShaders
		COMMAND "${DXC_BINARY}/dxc.exe" /Vd /Zi /Od /Vn g_p${FILE_WE} /Fo ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.cso /T lib_6_8 /Fh ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.hlsl.h /nologo ${FILE}
		             MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)		
endforeach(FILE)

foreach(FILE ${HLSL_CS_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  list(APPEND CSO_SHADER_FILES ${CMAKE_BINARY_DIR}/${FILE_WE}.cso)
	add_custom_command(TARGET CompileShaders
		COMMAND "${DXC_BINARY}/dxc.exe" /E CSMain /Zi /Od /Vn g_p${FILE_WE} /Fo ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.cso /T cs_6_5 /Fh ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.hlsl.h /nologo ${FILE}
		             MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)		
endforeach(FILE)

add_dependencies(ascend CompileShaders)

file( COPY ${SHADER_SOURCE_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug )

target_sources(ascend PRIVATE ${SOURCES})
target_sources(ascend PRIVATE ${SHADER_FILES})

set_source_files_properties(${SHADER_FILES}  PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source" FILES ${SOURCES})
source_group(TREE ${SHADER_SOURCE_DIR} PREFIX "ShaderSources" FILES ${SHADER_FILES})

target_compile_definitions(ascend PRIVATE SHADER_SRC="${SHADER_SOURCE_REL_DIR}", ImTextureID=ImU64)

target_include_directories(ascend PUBLIC . ${SHADER_SOURCE_DIR}/CompiledShaders/)
target_link_libraries(ascend PRIVATE
    Microsoft.Direct3D.D3D12    
    Microsoft.Direct3D.DXC
    Microsoft.Direct3D.WARP
    d3d12
    dxcompiler
    dxgi
    dxguid)

add_definitions(-DUNICODE -D_UNICODE)
add_compile_definitions(DEBUG)

