Analysis of dear-imgui d3d12 implementation


| Likes                                                                                                                      | Dislikes                                                                              |
| -------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| CreateDeviceD3D to initialize                                                                                              | ImGui Business just casually lives in main <- could this be moved to a UserInterface? |
| ImGui frontend gets to live away from d3d12 code (init functions for d3d12 could be called via the renderer)               | Main loop has too many shenanigans in it                                              |
| ZeroingMemory before using something is good :)                                                                            | `ImGui::Render();` followed by d3d12 looks bad                                        |
| Ifdef for debug layer                                                                                                      | Create render Target could be more specific?                                          |
| basic info queue actually exists                                                                                           |                                                                                       |
| Releasing bunch of temp objects is pretty good                                                                             |                                                                                       |
| CleanupD3D                                                                                                                 |                                                                                       |
| WaitForNextFrameResources                                                                                                  |                                                                                       |
| WndProc is also in main.cpp. Having "Windows application is a pain in the arse" - no point of putting it inside of a class |                                                                                       |

Design Pattern overview for render-main
[command](https://gameprogrammingpatterns.com/command.html) - no use but could be nice for shaders/rendercommands??
[flyweight](https://gameprogrammingpatterns.com/flyweight.html) - might be good to use for models and [pooling](https://gameprogrammingpatterns.com/object-pool.html) later on
