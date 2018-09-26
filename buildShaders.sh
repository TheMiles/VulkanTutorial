#!/bin/bash

shaderfiles=(
    "triangle.frag"
    "triangle.vert"
)

shaderpath="shaders"

buildpath=(
    "build"
)

for shader in "${shaderfiles[@]}"
do
    shaderInFile=$shader
    if [[ -e $shaderpath/$shaderInFile ]]; then
        shaderOutFile=${shaderInFile%.*}_${shaderInFile##*.}.spv
        echo Compiling "$shaderInFile" to "$shaderOutFile"
        glslangValidator -V "$shaderpath/$shaderInFile" -o "$shaderpath/$shaderOutFile"

        for builddir in "${buildpath[@]}"
        do
            if [[ -d $builddir ]]; then
                mkdir -p $builddir/$shaderpath
                cp $shaderpath/$shaderOutFile $builddir/$shaderpath/$shaderOutFile
            fi
        done
    fi
done