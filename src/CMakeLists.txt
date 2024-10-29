# includes from https://github.com/KStocky/ShaderTestFramework
include(NugetPackages)
include(AssetDependencyManagement)

add_executable(ascend WIN32)
asset_dependency_init(ascend)

nuget_initialize(ascend)
nuget_get_agility_sdk(ShaderTestFramework AGILITY_FOUND AGILITY_INCLUDE AGILITY_DLL SDK_VER)
nuget_get_dxc(ascend DXC_FOUND DXC_INCLUDE DXC_DLL DXC_LIB)
#nuget_get_warp(ShaderTestFramework WARP_FOUND WARP_DLL)
#nuget_get_WinPixEventRuntime(ShaderTestFramework WINPIX_FOUND WINPIX_INCLUDE WINPIX_BIN)
add_library(DXC SHARED IMPORTED)
set_target_properties(DXC PROPERTIES
    IMPORTED_IMPLIB "${DXC_LIB}/dxcompiler.lib"
)


set(SOURCES ascend/main.cpp 
			ascend/ascendHelpers.h
			ascend/Renderer.cpp 
			ascend/Renderer.h 
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

set(SHADER_SOURCES ascend/Shader/shaders.hlsl
		   ascend/Shader/HlslCompat.h
		   ascend/Shader/RaytracingHlslCompat.h
	 	   ascend/Shader/Raytacing.hlsl)

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ascend/Shader)
set(SHADER_SOURCE_REL_DIR "Shaders")

file(GLOB_RECURSE AGILITY_HEADERS "${AGILITY_INCLUDE}/*.h*" )
file(GLOB_RECURSE DXC_HEADERS "${DXC_INCLUDE}/*.h*" )
file( COPY ${SHADER_SOURCE_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug )

target_sources(ascend PRIVATE ${SOURCES})
target_sources(ascend PRIVATE ${AGILITY_HEADERS})
target_sources(ascend PRIVATE ${DXC_HEADERS})
target_sources(ascend PRIVATE ${SHADER_SOURCES})

set_source_files_properties(${SHADER_SOURCES} PROPERTIES VS_SETTINGS "ExcludedFromBuild=true")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source" FILES ${SOURCES})
source_group(TREE ${AGILITY_INCLUDE} PREFIX "ThirdParty/AgilitySDK" FILES ${AGILITY_HEADERS})
source_group(TREE ${DXC_INCLUDE} PREFIX "ThirdParty/DXC" FILES ${DXC_HEADERS})
source_group(TREE ${SHADER_SOURCE_DIR} PREFIX "ShaderSources" FILES ${SHADER_SOURCES})

target_compile_definitions(ascend PRIVATE D3D12_SDK_VERSION_ID=${SDK_VER} SHADER_SRC="${SHADER_SOURCE_REL_DIR}", ImTextureID=ImU64)
target_include_directories(ascend PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Public/ ${AGILITY_INCLUDE} ${DXC_INCLUDE})

target_link_libraries(ascend PRIVATE d3d12.lib dxgi.lib d3dcompiler.lib DXC dxguid.lib)

target_add_asset_directory(ascend ${AGILITY_DLL} "/D3D12")
target_add_asset_directory(ascend ${DXC_DLL} "/")

add_definitions(-DUNICODE -D_UNICODE)
add_compile_definitions(DEBUG)