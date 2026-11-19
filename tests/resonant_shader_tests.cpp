#define NOMINMAX
#include "../source/rendering/LegacySamplerBindings.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>

using Microsoft::WRL::ComPtr;

namespace {
ComPtr<ID3DBlob> compile(const std::string &source, const char *entry,
                        const char *profile) {
  ComPtr<ID3DBlob> code, errors;
  const auto result = D3DCompile(source.data(), source.size(), "resonant-test",
      nullptr, nullptr, entry, profile,
      D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 0, &code, &errors);
  INFO((errors ? static_cast<const char *>(errors->GetBufferPointer()) : ""));
  REQUIRE(SUCCEEDED(result));
  return code;
}
}

TEST_CASE("RunResonant renders scene colours and follows bypass on D3D11",
          "[compositor][shader][regression]") {
  const auto path = std::filesystem::path(RUN3_TEST_SOURCE_DIR) /
      "Games/The Long Way/TheLongWay/run3/shaders/Effect8.cg";
  if (!std::filesystem::exists(path)) SKIP("TLW content absent");
  std::ifstream input(path, std::ios::binary);
  const std::string authored{std::istreambuf_iterator<char>(input), {}};
  auto source = run3::rendering::detail::separateD3D11SamplerBindings(authored);
  // Match the fullscreen position input added by translateCgSource. D3D11
  // links the quad's position and UV by register, even when position is unused.
  const auto entry = source.find("float4 main_ps(");
  REQUIRE(entry != std::string::npos);
  source.insert(entry + std::string("float4 main_ps(").size(),
                "float4 position : POSITION, ");
  const auto ps = compile(source, "main_ps", "ps_4_0");
  const auto vs = compile(R"(
struct Out { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
Out main(uint id : SV_VertexID) {
  Out o;
  o.pos = float4(id == 2 ? 3 : -1, id == 1 ? 3 : -1, 0, 1);
  o.uv = float2(0.25, 0.25);
  return o;
})", "main", "vs_4_0");

  ComPtr<ID3D11ShaderReflection> reflection;
  REQUIRE(SUCCEEDED(D3DReflect(ps->GetBufferPointer(), ps->GetBufferSize(),
      __uuidof(ID3D11ShaderReflection), reinterpret_cast<void **>(reflection.GetAddressOf()))));
  for (const auto &[name, slot] : {std::pair{"RTTexture", 0u},
                                   std::pair{"patTexture", 1u}}) {
    D3D11_SHADER_INPUT_BIND_DESC binding{};
    REQUIRE(SUCCEEDED(reflection->GetResourceBindingDescByName(name, &binding)));
    CHECK(binding.BindPoint == slot);
  }
  D3D11_SHADER_VARIABLE_DESC bypass{};
  REQUIRE(SUCCEEDED(reflection->GetVariableByName("bypass")->GetDesc(&bypass)));
  D3D11_SHADER_BUFFER_DESC buffer{};
  REQUIRE(SUCCEEDED(reflection->GetConstantBufferByIndex(0)->GetDesc(&buffer)));

  // WARP renders offscreen without opening a game window or depending on a GPU.
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;
  REQUIRE(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
      0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context)));
  ComPtr<ID3D11VertexShader> vertex;
  ComPtr<ID3D11PixelShader> pixel;
  REQUIRE(SUCCEEDED(device->CreateVertexShader(vs->GetBufferPointer(),
      vs->GetBufferSize(), nullptr, &vertex)));
  REQUIRE(SUCCEEDED(device->CreatePixelShader(ps->GetBufferPointer(),
      ps->GetBufferSize(), nullptr, &pixel)));
  context->VSSetShader(vertex.Get(), nullptr, 0);
  context->PSSetShader(pixel.Get(), nullptr, 0);
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  D3D11_VIEWPORT viewport{0, 0, 1, 1, 0, 1};
  context->RSSetViewports(1, &viewport);

  std::array<std::array<float, 4>, 16> scene{};
  for (unsigned i = 0; i < scene.size(); ++i)
    scene[i] = {static_cast<float>(i) / 15, 0.3f, 0.8f, 1};
  const std::array<float, 4> pattern{0.25f, 0.25f, 0.25f, 1};
  std::array<ComPtr<ID3D11ShaderResourceView>, 2> views;
  for (unsigned i = 0; i < views.size(); ++i) {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = 1;
    desc.Height = i == 0 ? 16 : 1;
    desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{i == 0 ? static_cast<const void *>(scene.data()) :
        static_cast<const void *>(pattern.data()), sizeof(pattern), 0};
    ComPtr<ID3D11Texture2D> texture;
    REQUIRE(SUCCEEDED(device->CreateTexture2D(&desc, &data, &texture)));
    REQUIRE(SUCCEEDED(device->CreateShaderResourceView(texture.Get(), nullptr, &views[i])));
  }
  ID3D11ShaderResourceView *inputs[]{views[0].Get(), views[1].Get()};
  context->PSSetShaderResources(0, 2, inputs);
  D3D11_SAMPLER_DESC sampling{};
  sampling.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  sampling.AddressU = sampling.AddressV = sampling.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampling.MaxLOD = D3D11_FLOAT32_MAX;
  ComPtr<ID3D11SamplerState> sampler;
  REQUIRE(SUCCEEDED(device->CreateSamplerState(&sampling, &sampler)));
  ID3D11SamplerState *samplers[]{sampler.Get(), sampler.Get()};
  context->PSSetSamplers(0, 2, samplers);

  D3D11_TEXTURE2D_DESC outputDesc{};
  outputDesc.Width = outputDesc.Height = outputDesc.MipLevels =
      outputDesc.ArraySize = outputDesc.SampleDesc.Count = 1;
  outputDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  outputDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
  ComPtr<ID3D11Texture2D> output, staging;
  REQUIRE(SUCCEEDED(device->CreateTexture2D(&outputDesc, nullptr, &output)));
  ComPtr<ID3D11RenderTargetView> target;
  REQUIRE(SUCCEEDED(device->CreateRenderTargetView(output.Get(), nullptr, &target)));
  context->OMSetRenderTargets(1, target.GetAddressOf(), nullptr);
  outputDesc.BindFlags = 0;
  outputDesc.Usage = D3D11_USAGE_STAGING;
  outputDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  REQUIRE(SUCCEEDED(device->CreateTexture2D(&outputDesc, nullptr, &staging)));
  D3D11_BUFFER_DESC paramsDesc{};
  paramsDesc.ByteWidth = buffer.Size;
  paramsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  ComPtr<ID3D11Buffer> params;
  REQUIRE(SUCCEEDED(device->CreateBuffer(&paramsDesc, nullptr, &params)));
  context->PSSetConstantBuffers(0, 1, params.GetAddressOf());
  std::vector<float> values(buffer.Size / sizeof(float), 0);
  for (const float value : {2.f, 30.f, 120.f, 240.f}) {
    CAPTURE(value);
    values[bypass.StartOffset / sizeof(float)] = value;
    context->UpdateSubresource(params.Get(), 0, nullptr, values.data(), 0, 0);
    context->Draw(3, 0);
    context->CopyResource(staging.Get(), output.Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};
    REQUIRE(SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
    const auto colour = *static_cast<const std::array<float, 4> *>(mapped.pData);
    context->Unmap(staging.Get(), 0);
    // Compare the GPU output with the original shader's displacement formula.
    const float y = 0.25f + std::cos(0.25f * 3.14f) * (1 - pattern[1] + 0.01f) / (value / 10);
    const auto row = std::min(15, std::max(0, static_cast<int>(y * 16)));
    CHECK(colour[0] == Catch::Approx(scene[row][0]));
    CHECK(colour[1] == Catch::Approx(0.3f));
    CHECK(colour[2] == Catch::Approx(0.8f));
  }
}

TEST_CASE("Glow function samplers retain independent D3D11 bindings",
          "[compositor][shader][regression]") {
  const auto code = compile(run3::rendering::detail::separateD3D11SamplerBindings(R"(
float4 main(float2 uv : TEXCOORD0,
    uniform sampler2D scene : register(s0),
    uniform sampler2D blur : register(s1)) : COLOR {
  return tex2D(blur, uv) + tex2D(scene, uv);
})"), "main", "ps_4_0");
  REQUIRE(code->GetBufferSize() > 0);
}
