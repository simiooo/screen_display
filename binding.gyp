{
  "targets": [
    {
      "target_name": "render2display",
      "sources": [ "src/render2display.cpp" ],
      "conditions": [
        ["target_arch == 'ia32'", { "defines": ["ARCH_32BIT"] }],
        ["target_arch == 'x64'", { "defines": ["ARCH_64BIT"] }],
        ["target_arch == 'arm64'", { "defines": ["ARCH_ARM64"] }]
      ],
      "libraries": ["d2d1.lib", "dwrite.lib"],
      "include_dirs": [ "<!@(node -p \"require('node-addon-api').include\")" ],
      "defines": [
        "NODE_ADDON_API_CPP_EXCEPTIONS"  
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1 
        }
      }
    }
  ]
}