cbuffer ShaderBuffer
{
	matrix modelMatrix;
	matrix viewProjectionMatrix;
};

struct VertexInput
{
	float4 position : POSITION;
	float4 uv : UV;
};

struct PixelInput
{
	float4 position : SV_POSITION;
	float4 uv : UV;
};

PixelInput VertexMain(VertexInput input)
{
	PixelInput output;

	// Change the position vector to be 4 units for proper matrix calculations.
	input.position.w = 1.0f;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(input.position, modelMatrix);
	output.position = mul(output.position, viewProjectionMatrix);
	
	// Store the input color for the pixel shader to use.
	output.uv = input.uv;

	return output;
}

Texture2D imageTexture;
SamplerState textureSampler;

float4 PixelMain(PixelInput input) : SV_TARGET
{
	return imageTexture.Sample(textureSampler, input.uv);
}