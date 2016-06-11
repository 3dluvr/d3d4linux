
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

