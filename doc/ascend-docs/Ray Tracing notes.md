Specific Raytracing Hello World.sln Notes
- D3D12RayTracingHelloWorld Inherits from DXSample
- WindowsApplication calls OnInit (which also calls D3D12RayTracingHelloWorld OnInit cus virtual function table)
- DeviceResources (Controls all DirectXResources)

Unique to DeviceResources:
- Handles GPU hardware adapter
- Window Resizing
- swapchain
- buffer
- debug layer
- fence synchronisation (fences)
- Is a unique_ptr member variable within DXSample
- Literally acts as an interface to most non RT related obejcts

Unique to the RayTracingPipeline
**RayGenConstantBuffer**
- Includes a viewport size (for generating rays?)
- Includes stencil sizes too

Use query interface to create
ID3D12Device5 (Ray tracing device)
ID3D12CommandList4 (Ray tracing commandlist)

**Create Root Signatures**
	GLOBAL UAV shader is created  (This is shared across ALL raytracing shaders)
	OutputViewSLot
	AccelerationStructureSlot
	Count
	
	 **LOCAL root signature shader that allows unique arguments from shader tables**

Create Raytracing PSO
Sub Object creation
`CD3DX12_DXIL_LIBRARY_SUBOBJECT` - Helper class for creating a DXIL library state subobject
Next is to define shader exports from the dxil library

hitGroup shader is created
`CD3DX12_HIT_GROUP_SUBOBJECT`
(A hit group specifies closest hit, any hit and intersection shaders that need to be created)

create global root signature sub object
`CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT`

Raytracing shader Config is created
`CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT`

(for better per set max recursion depth as low as needed)
hello world has this set to 1

Create state object

Create descriptor heap
 Does what it says on the tin
 creates a singular descriptor heap (potentially DeviceResources has a triple buffered one??)

Build Geo
Does the tin thing again
allocates using an upload buffer (isn't this bad practice??) - obviously this is a sample
This is done through the use of DirectXRaytracing helper/.

Build Acceleration structures
reset command list to create acceleration structures
set geometry type for index buffer
`D3D12_RAYTRACING_GEOMETRY_DESC`
`D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS`
`D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS`
(insert woo yung woo "woah woaah woaahh")

device5 commands 
`GetRaytracingAccelerationStructurePrebuildInfo`

Scratch resource created (unordered access resources, potentially temporary for ray tracing operations but that's just what copilot said nothing actually official.)

Allocate resources for accel structures
accel structures have to be created on the default heap 

`D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE `

Allocate UAV buffer (using raytracing helper)

create instance desc
`D3D12_RAYTRACING_INSTANCE_DESC`
describes an acceleration structure used in GPU memory during the acceleration structure build phase

Create bottom level and top level acceleration structures

build acceleration structures from lambda

sync

Build shader tables (encapsulates all shader records -)
`# D3D12StateObjectProperties::GetShaderIdentifier`
getss root sig for a shader so that it can be used in a 

this is for aformentioned Ray gen shader tables
miss shader tables
hit gorup shader table

CreateRayTracingOutputResource
(creates the 2d output texture from the ray traced scene)

window size dependant resources do the same but creates the output resource followed by building the shader tables.

Device resources prepare (resets commandlist and command allocator)

DoRaytracing (Actually does the the raytracing)
create dispatch desc bind heaps, acceleration structure and dispatch rays
at the end run the dispatch rays lambda, this is a lambda because????
I'm not sure honestly


That's all!!
