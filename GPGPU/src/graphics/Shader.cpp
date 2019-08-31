#include "Pch.h"
#include "Shader.h"
#include "MeshFactory.h"

using namespace DirectX;

Shader::Shader(Graphics* g, const std::wstring& vertShaderPath, const std::wstring& pixelShaderPath)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;

	// Compile the vertex shader code.
	result = D3DCompileFromFile(vertShaderPath.c_str(), NULL, NULL, "VertexMain", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&vertexShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		OutputShaderError(errorMessage, vertShaderPath);
		assert(false);
	}

	// Compile the pixel shader code.
	result = D3DCompileFromFile(pixelShaderPath.c_str(), NULL, NULL, "PixelMain", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		OutputShaderError(errorMessage, pixelShaderPath);
		assert(false);
	}

	// Create the vertex shader from the buffer.
	result = g->GetDevice()->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_VertexShader);
	assert(!FAILED(result));

	// Create the pixel shader from the buffer.
	result = g->GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_PixelShader);
	assert(!FAILED(result));

	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = offsetof(Vertex, position);
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "UV";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = offsetof(Vertex, uv);;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	result = g->GetDevice()->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
		vertexShaderBuffer->GetBufferSize(), &m_InputLayout);
	assert(!FAILED(result));

	vertexShaderBuffer->Release();
	pixelShaderBuffer->Release();

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(ShaderBuffer);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = g->GetDevice()->CreateBuffer(&matrixBufferDesc, NULL, &m_InputBuffer);
	assert(!FAILED(result));
}

Shader::~Shader()
{
	m_InputBuffer->Release();
	m_InputLayout->Release();
	m_PixelShader->Release();
	m_VertexShader->Release();
}

void Shader::OutputShaderError(ID3D10Blob* errorMessage, const std::wstring& shaderPath)
{
	std::cerr << "Shader compilation failed!\n";
	if (errorMessage == nullptr)
	{
		std::cerr << "Could not open file " << std::string(shaderPath.begin(), shaderPath.end()) << std::endl;
		return;
	}

	char* compileErrors;
	unsigned long long bufferSize, i;

	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Write out the error message.
	for (i = 0; i < bufferSize; i++)
	{
		std::cerr << compileErrors[i];
	}

	// Release the error message.
	errorMessage->Release();
	assert(false);
}

void Shader::UpdateBuffer(Graphics* g, Camera* camera, const DirectX::XMMATRIX& modelMatrix)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ShaderBuffer* dataPtr;
	unsigned int bufferNumber;

	XMMATRIX viewProjection = XMMatrixTranspose(camera->GetViewProjection());
	XMMATRIX model = XMMatrixTranspose(modelMatrix);

	// Lock the constant buffer so it can be written to.
	result = g->GetDeviceContext()->Map(m_InputBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	assert(!FAILED(result));

	// Get a pointer to the data in the constant buffer.
	dataPtr = (ShaderBuffer*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	dataPtr->modelMatrix = model;
	dataPtr->viewProjectionMatrix = viewProjection;

	// Unlock the constant buffer.
	g->GetDeviceContext()->Unmap(m_InputBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 0;

	// Finanly set the constant buffer in the vertex shader with the updated values.
	g->GetDeviceContext()->VSSetConstantBuffers(bufferNumber, 1, &m_InputBuffer);
}

void Shader::Bind(Graphics* g)
{
	// Set the vertex input layout.
	g->GetDeviceContext()->IASetInputLayout(m_InputLayout);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	g->GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
	g->GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);
}