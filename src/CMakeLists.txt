# includes from https://github.com/KStocky/ShaderTestFramework
include(NugetPackages)
include(AssetDependencyManagement)

add_executable(ascend WIN32)
asset_dependency_init(ascend)


#set(ASSIMP_BUILD_LIB ${CMAKE_BINARY_DIR}/_deps/assimp-build/lib/debug/assimp-vc143-mtd.lib)
#set(ASSIMP_BUILD_DLL ${CMAKE_BINARY_DIR}/_deps/assimp-build/bin/Debug/assimp-vc143-mtd.dll)
#set(ASSIMP_CONFIG_DIR ${CMAKE_BINARY_DIR}/_deps/assimp-build/include/)
#set(ASSIMP_INCLUDE_DIR ${assimp_SOURCE_DIR}/include/)


nuget_initialize(ascend)
nuget_get_agility_sdk(ascend AGILITY_FOUND AGILITY_INCLUDE AGILITY_DLL SDK_VER)
nuget_get_dxc(ascend DXC_FOUND DXC_INCLUDE DXC_DLL DXC_LIB)
nuget_get_assimp(ascend ASSIMP_FOUND ASSIMP_INCLUDE ASSIMP_DLL ASSIMP_LIB)
#nuget_get_warp(ShaderTestFramework WARP_FOUND WARP_DLL)
#nuget_get_WinPixEventRuntime(ShaderTestFramework WINPIX_FOUND WINPIX_INCLUDE WINPIX_BIN)
add_library(DXC SHARED IMPORTED)

set_target_properties(DXC PROPERTIES
    IMPORTED_IMPLIB "${DXC_LIB}/dxcompiler.lib"
)

add_library(ASSIMP SHARED IMPORTED)
set_target_properties(ASSIMP PROPERTIES
    IMPORTED_IMPLIB "${ASSIMP_LIB}/assimp-vc142-mtd.lib"
)


set(SOURCES ascend/main.cpp 
			ascend/Camera.h
			ascend/Camera.cpp
			ascend/GBuffer.h
			ascend/GBuffer.cpp
			ascend/InputCommands.h
			ascend/Model.h
			ascend/Model.cpp
			ascend/WorkGraphsDXR.h
			ascend/WorkGraphsDXR.cpp
			ascend/IMGuiHelper.h
			ascend/IMGuiHelper.cpp
			ascend/DX12/DDSTextureLoader12.cpp
			ascend/DX12/DDSTextureLoader12.h
			ascend/DX12/DX12.h
			ascend/DX12/DX12.cpp
			ascend/DX12/DX12_Helpers.h
			ascend/DX12/DX12_Helpers.cpp
			ascend/DX12/GraphicsTypes.h
			ascend/DX12/GraphicsTypes.cpp
			ascend/DX12/PCH.h
			ascend/DX12/SwapChain.h
			ascend/DX12/SwapChain.cpp
			ascend/DX12/Textures.cpp
			ascend/DX12/Textures.h
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
set(HLSL_LIB_SHADER_FILES ascend/Shader/WorkGraphRaytracing.hlsl)
set(HLSL_CS_SHADER_FILES  ascend/Shader/ComputeRaytracing.hlsl)
set(HLSL_VS_SHADER_FILES ascend/Shader/RasterVS.hlsl ascend/Shader/IMGuiVS.hlsl)
set(HLSL_PS_SHADER_FILES ascend/Shader/RasterPS.hlsl ascend/Shader/IMGuiPS.hlsl)
set(SHADER_FILES ${HLSL_LIB_SHADER_FILES} 					  
				 ${HLSL_CS_SHADER_FILES}
				 ${HLSL_VS_SHADER_FILES}
				 ${HLSL_PS_SHADER_FILES})  
				 
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ascend/Shader)
set(RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ascend/res)

set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_TYPE lib)
set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_MODEL 6_3)
set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_ENTRY_POINT CSMain)
set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_VARIABLE_NAME "g_p%(Filename)")
set_source_files_properties(${HLSL_LIB_SHADER_FILES} PROPERTIES VS_SHADER_OUTPUT_HEADER_FILE "$(IntDir)CompiledShaders\%(Filename).hlsl.h")

set_source_files_properties(${HLSL_CS_SHADER_FILES} PROPERTIES VS_SHADER_TYPE cs)
set_source_files_properties(${HLSL_CS_SHADER_FILES} PROPERTIES VS_SHADER_MODEL 6_5)
set_source_files_properties(${HLSL_CS_SHADER_FILES} PROPERTIES VS_SHADER_VARIABLE_NAME "g_p%(Filename)")
set_source_files_properties(${HLSL_CS_SHADER_FILES} PROPERTIES VS_SHADER_OUTPUT_HEADER_FILE "$(IntDir)CompiledShaders\%(Filename).hlsl.h")

set_source_files_properties(${HLSL_VS_SHADER_FILES} PROPERTIES VS_SHADER_TYPE vs)
set_source_files_properties(${HLSL_VS_SHADER_FILES} PROPERTIES VS_SHADER_MODEL 6_5)
set_source_files_properties(${HLSL_VS_SHADER_FILES} PROPERTIES VS_SHADER_VARIABLE_NAME "g_p%(Filename)")
set_source_files_properties(${HLSL_VS_SHADER_FILES} PROPERTIES VS_SHADER_OUTPUT_HEADER_FILE "$(IntDir)CompiledShaders\%(Filename).hlsl.h")

set_source_files_properties(${HLSL_PS_SHADER_FILES} PROPERTIES VS_SHADER_TYPE ps)
set_source_files_properties(${HLSL_PS_SHADER_FILES} PROPERTIES VS_SHADER_MODEL 6_5)
set_source_files_properties(${HLSL_PS_SHADER_FILES} PROPERTIES VS_SHADER_VARIABLE_NAME "g_p%(Filename)")
set_source_files_properties(${HLSL_PS_SHADER_FILES} PROPERTIES VS_SHADER_OUTPUT_HEADER_FILE "$(IntDir)CompiledShaders\%(Filename).hlsl.h")


set(SHADER_SOURCE_REL_DIR "Shaders")

# Compile shaders using dxc
add_custom_target(CompileShaders)

file(MAKE_DIRECTORY ${SHADER_SOURCE_DIR}/CompiledShaders)

foreach(FILE ${HLSL_LIB_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  list(APPEND CSO_SHADER_FILES ${CMAKE_BINARY_DIR}/${FILE_WE}.cso)
	add_custom_command(TARGET CompileShaders
		COMMAND "${DXC_DLL}/dxc.exe" /Zi /Od /Vn g_p${FILE_WE} /Fo ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.cso /T lib_6_8 /Fh ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.hlsl.h /nologo ${FILE}
		             MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)		
endforeach(FILE)

foreach(FILE ${HLSL_CS_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  list(APPEND CSO_SHADER_FILES ${CMAKE_BINARY_DIR}/${FILE_WE}.cso)
	add_custom_command(TARGET CompileShaders
		COMMAND "${DXC_DLL}/dxc.exe" /E CSMain /Zi /Od /Vn g_p${FILE_WE} /Fo ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.cso /T cs_6_8 /Fh ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.hlsl.h /nologo ${FILE}
		             MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)		
endforeach(FILE)

foreach(FILE ${HLSL_VS_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  list(APPEND CSO_SHADER_FILES ${CMAKE_BINARY_DIR}/${FILE_WE}.cso)
	add_custom_command(TARGET CompileShaders
		COMMAND "${DXC_DLL}/dxc.exe" /E VSMain /Zi /Od /Vn g_p${FILE_WE} /Fo ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.cso /T vs_6_8 /Fh ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.hlsl.h /nologo ${FILE}
		             MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)		
endforeach(FILE)

foreach(FILE ${HLSL_PS_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  list(APPEND CSO_SHADER_FILES ${CMAKE_BINARY_DIR}/${FILE_WE}.cso)
	add_custom_command(TARGET CompileShaders
		COMMAND "${DXC_DLL}/dxc.exe" /E PSMain /Zi /Od /Vn g_p${FILE_WE} /Fo ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.cso /T ps_6_8 /Fh ${SHADER_SOURCE_DIR}/CompiledShaders/${FILE_WE}.hlsl.h /nologo ${FILE}
		             MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)		
endforeach(FILE)

add_dependencies(ascend CompileShaders)

file(GLOB_RECURSE AGILITY_HEADERS "${AGILITY_INCLUDE}/*.h*" )
file(GLOB_RECURSE DXC_HEADERS "${DXC_INCLUDE}/*.h*" )
file(GLOB_RECURSE ASSIMP_HEADERS "${ASSIMP_INCLUDE}/*.h*" )
file( COPY ${SHADER_SOURCE_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug )
file( COPY ${RESOURCE_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug )
file( COPY "${AGILITY_DLL}/" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug )
file( COPY "${ASSIMP_DLL}/" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug )

target_sources(ascend PRIVATE ${SOURCES})
target_sources(ascend PRIVATE ${AGILITY_HEADERS})
target_sources(ascend PRIVATE ${DXC_HEADERS})
target_sources(ascend PRIVATE ${SHADER_FILES})
target_sources(ascend PRIVATE ${ASSIMP_HEADERS})

set_source_files_properties(${SHADER_FILES}  PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source" FILES ${SOURCES})
source_group(TREE ${AGILITY_INCLUDE} PREFIX "ThirdParty/AgilitySDK" FILES ${AGILITY_HEADERS})
source_group(TREE ${DXC_INCLUDE} PREFIX "ThirdParty/DXC" FILES ${DXC_HEADERS})
source_group(TREE ${ASSIMP_INCLUDE} PREFIX "ThirdParty/assimp" FILES ${ASSIMP_HEADERS})
source_group(TREE ${SHADER_SOURCE_DIR} PREFIX "ShaderSources" FILES ${SHADER_FILES})

target_compile_definitions(ascend PRIVATE D3D12_SDK_VERSION_ID=${SDK_VER} SHADER_SRC="${SHADER_SOURCE_REL_DIR}", ImTextureID=ImU64)
target_include_directories(ascend PUBLIC . ${CMAKE_CURRENT_LIST_DIR}/Public/ ${AGILITY_INCLUDE} ${DXC_INCLUDE} ${ASSIMP_INCLUDE})
target_include_directories(ascend PUBLIC . ${SHADER_SOURCE_DIR}/CompiledShaders/)
target_link_libraries(ascend PRIVATE ASSIMP d3d12.lib dxgi.lib d3dcompiler.lib DXC dxguid.lib ASSIMP)

target_add_asset_directory(ascend ${AGILITY_DLL} "/D3D12")
target_add_asset_directory(ascend ${DXC_DLL} "/")
target_add_asset_directory(ascend ${ASSIMP_DLL} "/")
add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
add_compile_definitions(DEBUG)

