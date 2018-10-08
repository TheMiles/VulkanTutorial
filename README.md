# Doing a Vulkan tutorial

I'm currently working on this [Vulkan tutorial](https://vulkan-tutorial.com/).
This repository shows the current progress of my work so far.

This is not a tutorial by itself. It's just keeping track of my drafts, while I'm trying to understand everything. So don't expect any helpful insights (though the history hopefully explains a little bit the progress), or anythin which could be called a framework.

### TODO

* Create a shader module
    * Use GLSL files and compile to SPIR-V in the executable
    * Watch the source files, recompile on changes (use default shader on errors)
* Texture loading shall use its own command buffer `setupCommandBuffer`, to handle loading async of the regular commands
