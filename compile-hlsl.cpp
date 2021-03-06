//
//  D3D4Linux — access Direct3D DLLs from Linux programs
//
//  Copyright © 2016 Sam Hocevar <sam@hocevar.net>
//
//  This library is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#if __linux__
#  include "d3d4linux.h"
#else
#  include <d3d11.h>
#  include <d3dcompiler.h>
#endif

#include <string>
#include <fstream>
#include <streambuf>

#include <cstdio>
#include <cstdint>

int main(int argc, char *argv[])
{
    HRESULT ret = 0;

    if (argc <= 3)
    {
        fprintf(stderr, "Usage: %s shader.hlsl entry_point type\n", argv[0]);
        return -1;
    }

    std::string file = argv[1];
    std::string entry = argv[2];
    std::string type = argv[3];

    std::ifstream t(file);
    std::string source((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());

    ID3DBlob* shader_blob = nullptr;
    ID3DBlob* error_blob = nullptr;

    HMODULE lib = LoadLibrary("d3dcompiler_43.dll");
    pD3DCompile compile = (pD3DCompile)GetProcAddress(lib, "D3DCompile");

    printf("Calling: D3DCompile\n");

    ret = compile(source.c_str(), source.size(),
                  file.c_str(),
                  nullptr, /* unimplemented */
                  nullptr, /* unimplemented */
                  entry.c_str(),
                  type.c_str(),
                  D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY
                   | D3DCOMPILE_OPTIMIZATION_LEVEL0
                   | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR,
                  0, &shader_blob, &error_blob);

    printf("Result: 0x%08x\n", (int)ret);

    if (FAILED(ret))
    {
        if (error_blob)
        {
            printf("%s\n", (char const *)error_blob->GetBufferPointer());
            error_blob->Release();
        }
    }
    else
    {
        uint8_t *buf = (uint8_t *)shader_blob->GetBufferPointer();
        int len = shader_blob->GetBufferSize();
        for (int i = 0; i < len; ++i )
        {
            if (buf[i] <= 9)
                printf("\\%d", buf[i]);
            else if (buf[i] >= ' ' && buf[i] < 0x7f)
                printf("%c", buf[i]);
            else
                printf("\\x%02x", buf[i]);
        }
        printf("\n");

        printf("Calling: D3DReflect\n");

        ID3D11ShaderReflection *reflector = nullptr;
        ret = D3DReflect(shader_blob->GetBufferPointer(),
                         shader_blob->GetBufferSize(),
                         IID_ID3D11ShaderReflection,
                         (void **)&reflector);

        printf("Result: 0x%08x\n", (int)ret);

        if (SUCCEEDED(ret))
        {
            D3D11_SHADER_DESC shader_desc;
            reflector->GetDesc(&shader_desc);
            printf("Creator: %s\n", shader_desc.Creator);

            printf("Input Parameters (%d):\n", shader_desc.InputParameters);
            for (size_t i = 0; i < shader_desc.InputParameters; ++i)
            {
                D3D11_SIGNATURE_PARAMETER_DESC param_desc;
                reflector->GetInputParameterDesc(i, &param_desc);
                printf("  %d: %s\n", param_desc.SemanticIndex, param_desc.SemanticName);
            }

            printf("Output Parameters (%d):\n", shader_desc.OutputParameters);
            for (size_t i = 0; i < shader_desc.OutputParameters; ++i)
            {
                D3D11_SIGNATURE_PARAMETER_DESC param_desc;
                reflector->GetOutputParameterDesc(i, &param_desc);
                printf("  %d: %s\n", param_desc.SemanticIndex, param_desc.SemanticName);
            }

            printf("Bound Resources (%d):\n", shader_desc.BoundResources);
            for (size_t i = 0; i < shader_desc.BoundResources; ++i)
            {
                D3D11_SHADER_INPUT_BIND_DESC bind_desc;
                reflector->GetResourceBindingDesc(i, &bind_desc);
                printf("  %s\n", bind_desc.Name);
            }

            printf("Constant Buffers (%d):\n", shader_desc.ConstantBuffers);
            for (uint32_t i = 0; i < shader_desc.ConstantBuffers; ++i)
            {
                ID3D11ShaderReflectionConstantBuffer *cbuffer
                        = reflector->GetConstantBufferByIndex(i);

                D3D11_SHADER_BUFFER_DESC buffer_desc;
                cbuffer->GetDesc(&buffer_desc);
                printf("  %s (%d variables):\n", buffer_desc.Name, buffer_desc.Variables);

                for (uint32_t j = 0; j < buffer_desc.Variables; ++j)
                {
                    ID3D11ShaderReflectionVariable *var
                            = cbuffer->GetVariableByIndex(j);

                    D3D11_SHADER_VARIABLE_DESC variable_desc;
                    var->GetDesc(&variable_desc);
                    printf("    %s (%d bytes)\n", variable_desc.Name, variable_desc.Size);
                }
            }
        }

        printf("Calling: D3DStripShader\n");

        ID3DBlob* strip_blob = nullptr;
        ret = D3DStripShader(shader_blob->GetBufferPointer(),
                             shader_blob->GetBufferSize(),
                             0,
                             &strip_blob);

        printf("Result: 0x%08x\n", (int)ret);

        printf("Calling: D3DDisassemble\n");

        ID3DBlob* disas_blob = nullptr;
        ret = D3DDisassemble(shader_blob->GetBufferPointer(),
                             shader_blob->GetBufferSize(),
                             0, "",
                             &disas_blob);

        if (SUCCEEDED(ret))
            fwrite(disas_blob->GetBufferPointer(),
                   disas_blob->GetBufferSize(),
                   1, stdout);

        printf("Result: 0x%08x\n", (int)ret);
    }

    if (shader_blob)
        shader_blob->Release();
}

