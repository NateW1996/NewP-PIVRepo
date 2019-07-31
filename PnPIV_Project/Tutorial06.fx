//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);
TextureCube gCubeMap : register(t1);
SamplerState samTriLinear : register(s1)
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

cbuffer ConstantBuffer2 : register(b1)
{
    float4 vLightDir[3];
    float4 vLightColor[3];
    float4 vLightPos[3];
    float4 vLightType[3];
    float4 vLightRange[3];

    float4 vInnerAngle[3];
    float4 vOuterAngle[3];
    float4 vOutputColor;
};

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 WorldPos : POSITION;
    float4 LocalPos : POSITION1;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD1;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.LocalPos = input.Pos;

    output.Pos = mul(float4(input.Pos.xyz, 1), World);
    output.WorldPos = output.Pos;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Norm = mul(float4(input.Norm, 0), World).xyz;
    output.Tex = input.Tex;
    return output;
}

//--------------------------------------------------------------------------------------
// CubeMap Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VSCubeMap(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.WorldPos = input.Pos;
    output.Tex = output.LocalPos;
    output.LocalPos = input.Pos;
    output.Pos = mul(float4(input.Pos.xyz, 1), World);
    output.WorldPos = output.Pos;
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Norm = mul(float4(input.Norm, 0), World).xyz;
    output.Tex = output.LocalPos;
    return output;
}

//--------------------------------------------------------------------------------------
// CubeMap Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSCubeMap(PS_INPUT input) : SV_Target
{
    float4 finalColor = (0.0f, 0.0f, 0.0f, 1.0f);
    finalColor *= gCubeMap.Sample(samTriLinear, input.LocalPos.xyz);
    return finalColor;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
    float4 finalColor = (0.0f, 0.0f, 0.0f, 0.0f);
    //Light Calculations
    for (int i = 0; i < 3; i++)
    {
        if (int(vLightType[i].x) == 0)
        {
            finalColor += saturate(dot(-normalize(vLightDir[i].xyz), normalize(input.Norm))) * vLightColor[i];
        }
        if (int(vLightType[i].x) == 1)
        {
            float3 lightDirection = (vLightPos[i].xyz - input.WorldPos.xyz);
            float distance = length(lightDirection);
            lightDirection = lightDirection / distance;
            float AngularAtt = saturate(dot(lightDirection, normalize(input.Norm)));
            float RangeAtt = 1.0f - saturate(distance / vLightRange[i]);
            RangeAtt = pow(RangeAtt, 2);
            //RangeAtt = 1;
            finalColor += vLightColor[i] * AngularAtt * RangeAtt;
        }
        if (int(vLightType[i].x) == 2)
        {
            float3 lightDirection = (input.WorldPos.xyz - vLightPos[i].xyz);
            float distance = length(lightDirection);
            lightDirection = lightDirection / distance;
            float AngularAtt = saturate(dot(-lightDirection, normalize(input.Norm)));
            //float RangeAtt = 1.0f - saturate(distance / vLightRange[i]);
            float SpotDot = dot(lightDirection, vLightDir[i].xyz);

            float SpotAtt = saturate((SpotDot - vOuterAngle[i].x) / (vInnerAngle[i].x - vOuterAngle[i].x));
            finalColor += vLightColor[i] * AngularAtt * SpotAtt;
        }
    }   
    finalColor *= txDiffuse.Sample(samLinear, input.Tex);
    finalColor.a = 1;
    return finalColor;
}

//--------------------------------------------------------------------------------------
// PSSolid - render a solid color
//--------------------------------------------------------------------------------------
float4 PSSolid(PS_INPUT input) : SV_Target
{
    return vOutputColor;
}
