workspace "TemplateProject"
    configurations { "Debug", "Release", "Distribution" }
    platforms { "Windows" }
    
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
        
    include "VulkanHelperPremake5.lua"
    include "TemplateProject"
