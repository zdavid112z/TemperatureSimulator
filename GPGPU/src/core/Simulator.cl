#define IMAGE_WIDTH 0
#define IMAGE_HEIGHT 1
#define OFFSET_X 2
#define OFFSET_Y 3
#define START_TEMP 4
#define MAX_ROW 5

#define SIZE 0
#define HEATMAP_SCALE_FACTOR 2

#define BLUR_SIZE 1

kernel void Sum(global int* params, global float* inData, global float* outData, local float* partial)
{
	int globalId = get_global_id(0);
	int localId = get_local_id(0);
	int localSize = get_local_size(0);
	if (globalId < params[SIZE])
	{
		partial[localId] = inData[globalId];
	}
	else
	{
		partial[localId] = 0;
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	for (int i = localSize / 2; i > 0; i /= 2)
	{
		if(localId < i)
			partial[localId] += partial[localId + i];
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	if(localId == 0)
		outData[get_group_id(0)] = partial[0];
}

kernel void Simulate(global int* params, global float* input, global float* output)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	float sum = 0.f;

	for (int i = -BLUR_SIZE; i <= BLUR_SIZE; i++)
	{
#if 1
		int nx = x + params[OFFSET_X] * i;
		int ny = y + params[OFFSET_Y] * i;
		if (nx < 0 || ny < 0 || nx >= params[IMAGE_WIDTH] || ny >= params[IMAGE_HEIGHT])
			sum += params[START_TEMP];
		else sum += input[nx + ny * params[IMAGE_WIDTH]];
#else	
		int nx = clamp(x + params[OFFSET_X] * i, 0, params[IMAGE_WIDTH] - 1);
		int ny = clamp(y + params[OFFSET_Y] * i, 0, params[IMAGE_HEIGHT] - 1);
		sum += input[nx + ny * params[IMAGE_WIDTH]];
#endif
	}

	sum /= BLUR_SIZE * 2 + 1;
	if(x < params[IMAGE_WIDTH] && y < params[MAX_ROW])
		output[x + y * ((int)params[IMAGE_WIDTH])] = sum;
}

kernel void ComputeDiff(global int* params, global float* original, global float* altered, global float* output)
{
	int pos = get_global_id(0);
	if(pos < params[SIZE])
		output[pos] = fabs(original[pos] - altered[pos]);
}

kernel void Heatmap(global int* params, global float* input, __write_only image2d_t output)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	if (x < params[IMAGE_WIDTH] && y < params[IMAGE_HEIGHT])
	{ 
		float temp = input[x + y * params[IMAGE_WIDTH]] / ((float)params[HEATMAP_SCALE_FACTOR] / 1000.f);
		float red, green, blue;
		if (temp <= 66.f) {
			red = 255.f;
			green = temp;
			green = 99.4708025861f * log(green) - 161.1195681661f;
			if (temp <= 19) {
				blue = 0;
			}
			else {
				blue = temp - 10;
				blue = 138.5177312231f * log(blue) - 305.0447927307f;
			}
		}
		else {
			red = temp - 60.f;
			red = 329.698727446f * pow(red, -0.1332047592f);
			green = temp - 60.f;
			green = 288.1221695283f * pow(green, -0.0755148492f);
			blue = 255.f;
		}

		write_imagef(output, (int2)(x, y), (float4)(
			clamp(red / 255.f, 0.f, 1.f),
			clamp(green / 255.f, 0.f, 1.f),
			clamp(blue / 255.f, 0.f, 1.f),
			1.f));
	}
}
