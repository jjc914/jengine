# wulkan
[ ] add wrappers around vulkan enums (just use a `using ...` statement)
[ ] fix type conversion warnings with `static_cast<...>`
[ ] !! decouple physical device from surface

# simple ecs
[ ] 

# jengine
[ ] render targets for multi-stage rendering
[ ] !! fix renderer, pipeline, viewport, render target to each do their correct jobs
[ ] create create pipeline function in device (auto pipeline = device.create_pipeline(viewport.render_pass(), layout, vertex_shader, fragment_shader);)
[ ] refactor: make sure everything uses logger library