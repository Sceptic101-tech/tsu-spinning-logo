#include<iostream>
#include<math.h>
#include<windows.h>

# define M_PI 3.14159265358979323846

using namespace std;
HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

//If the image is "twitching", then reduce the height and width of the terminal.
const int window_high = 50;
const int window_width = 50;

//steps
const float theta_step = 0.1f;
const float radius_step_for_front = 0.03f;
float radius_step;
const float x_step = 0.1f;
const float y_step = 0.1f;
const float z_step = 0.1f;

//lightning and visualisation section
const bool is_lighting = 1; //if '1' then the lighting will be calculated.
const bool is_TSU_letters = 0;// if '1' then the image will be filled with TSU_letters, no lighting calculation
char symbol = '1';// if is_lighting = is_TSU_letters = 0 then the image will be filled with symbol
char dictionary[12] = { '.', ',', '-', '~', ':', ';', '=', '!', '*', '#', '$', '@' };
char TSU_letters[3] = { 'T', 'S', 'U' };

//logo features
//logo consists of two semicircles(R1 and R2) and 4 planes(inner and outer border).
const float logo_high = 1.5f;
const float half_pi = M_PI / 2;
const float half_logo_thickness = 0.3f;
const float R1 = 1.0f;//The radius of the inner circle
const float R2 = 2.0f;//The radius of the outer circle
const float K2 = 10.0f;//The distance from the observer to the figure

const float K1 = window_width * K2 * 8 / (20 * (logo_high + R2));//The zoom level of the perspective projection to 2D

//pre-defined
char** output;
float** z_buffer;
float* xyz;
float* Lxyz;
float sin_alpha;
float cos_alpha;
float sin_beta;
float cos_beta;
float cos_theta;
float sin_theta;
float cos_phi;
float sin_phi;
float sin_gamma;
float cos_gamma;
float z;
float ooz;
float light_intensity;
int xp;
int yp;

void clearScreen()
{
	std::cout << "\x1B[2J\x1B[H"; // ANSI escape-code
}

void RotateAroundX(float* xyz, const float& cos_alpha, const float& sin_alpha)
{
	float old_y = xyz[1];
	xyz[1] = old_y * cos_alpha + xyz[2] * sin_alpha;
	xyz[2] = xyz[2] * cos_alpha - old_y * sin_alpha;
}

void RotateAroundY(float* xyz, const float& cos_phi, const float& sin_phi)
{
	float old_x = xyz[0];
	xyz[0] = old_x * cos_phi - xyz[2] * sin_phi;
	xyz[2] = old_x * sin_phi + xyz[2] * cos_phi;
}

void RotateAroundZ(float* xyz, const float& cos_beta, const float& sin_beta)
{
	float old_x = xyz[0];
	xyz[0] = old_x * cos_beta + xyz[1] * sin_beta;
	xyz[1] = xyz[1] * cos_beta - old_x * sin_beta;
}

template<typename T>
void M_FillWithValue(T** arr, const int& rows, const int& columns, char value)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			arr[i][j] = static_cast<T>(value);
		}
	}
}

void SetConsoleWindowSize(int width, int height) {
	//                                   
	COORD bufferSize = { static_cast<SHORT>(width), static_cast<SHORT>(height) };
	SetConsoleScreenBufferSize(handle, bufferSize);

	//                   
	SMALL_RECT rect = { 0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height - 1) };
	SetConsoleWindowInfo(handle, TRUE, &rect);
}

void RenderFrame(float alpha, float beta, float gamma)//Each frame will be rotated to alpha, beta, gamma, respectively, along the OX, OZ, OY axis.
{
	//Pre-calculation of the sin cos
	sin_alpha = sin(alpha);
	cos_alpha = cos(alpha);
	sin_beta = sin(beta);
	cos_beta = cos(beta);
	sin_gamma = sin(gamma);
	cos_gamma = cos(gamma);

	if (sin_alpha == 0) z = half_logo_thickness - z_step;
	for (z = -half_logo_thickness; z < half_logo_thickness; z += z_step)
	{
		if (z == -half_logo_thickness || z + z_step >= half_logo_thickness) radius_step = radius_step_for_front;
		else radius_step = R2 - R1 - radius_step_for_front;
		//Drawing circles
		for (float theta = -half_pi; theta < half_pi; theta += theta_step)//Circle on XOY
		{
			cos_theta = cos(theta);
			sin_theta = sin(theta);

			for (float radius = R1; radius < R2; radius += radius_step)
			{
				float circlex = radius * cos_theta;//The initial coordinates of the circle
				float circley = radius * sin_theta;

				xyz[0] = circlex;
				xyz[1] = circley;
				xyz[2] = z;

				RotateAroundX(xyz, cos_alpha, sin_alpha);
				RotateAroundY(xyz, cos_gamma, sin_gamma);
				RotateAroundZ(xyz, cos_beta, sin_beta);
				xyz[2] += K2;

				ooz = 1.0f / xyz[2]; //"one over z". It is more profitable to divide once and then multiply twice than to divide twice.
				xp = static_cast<int>(window_width / 2 + xyz[0] * ooz * K1);
				yp = static_cast<int>(window_high / 2 - xyz[1] * ooz * K1);

				if (xp < 0 || xp >= window_width || yp < 0 || yp >= window_high)
					continue;

				//The vector of the normal to the surface
				Lxyz[0] = cos_theta;
				Lxyz[1] = sin_theta;
				Lxyz[2] = 0;
				RotateAroundY(Lxyz, cos_gamma, sin_gamma);
				RotateAroundX(Lxyz, cos_alpha, sin_alpha);
				RotateAroundZ(Lxyz, cos_beta, sin_beta);

				light_intensity = (Lxyz[0] - Lxyz[2]) * is_lighting;//if not is_lighting -> fills by only one symbol

				if (light_intensity > 0)
				{
					if (ooz > z_buffer[xp][yp])
					{
						z_buffer[xp][yp] = ooz;
						int luminance_index = static_cast<int>(light_intensity * 7);//normalizing to fit dictionary size
						output[xp][yp] = dictionary[luminance_index];
					}
				}
				else
				{
					if (ooz > z_buffer[xp][yp])
					{
						z_buffer[xp][yp] = ooz;
						if (is_lighting) output[xp][yp] = dictionary[0];
						else output[xp][yp] = symbol;
					}
				}
			}
		}

		for (float radius = R1; radius < R2; radius += radius_step)
		{
			//Drawing lines
			//Left and right parts of logo
			for (int i = -1; i <= 1; i += 2)
			{
				for (float x = 0; abs(x) < logo_high; x -= x_step)
				{
					xyz[0] = x;
					xyz[1] = i * radius;
					xyz[2] = z;

					RotateAroundY(xyz, cos_gamma, sin_gamma);
					RotateAroundX(xyz, cos_alpha, sin_alpha);
					RotateAroundZ(xyz, cos_beta, sin_beta);
					xyz[2] += K2;

					ooz = 1.0f / xyz[2];
					xp = static_cast<int>(window_width / 2 + xyz[0] * ooz * K1);
					yp = static_cast<int>(window_high / 2 - xyz[1] * ooz * K1);

					if (xp < 0 || xp >= window_width || yp < 0 || yp >= window_high)
						continue;

					light_intensity = (sin_alpha * i / (abs(xyz[0]) + 1)) * is_lighting;//if not is_lighting -> fills by only one symbol

					if (light_intensity > 0)
					{
						if (ooz > z_buffer[xp][yp])
						{
							z_buffer[xp][yp] = ooz;
							int luminance_index = static_cast<int>(light_intensity * logo_high*1.8f);
							output[xp][yp] = dictionary[luminance_index];
						}
					}
					else
					{
						if (ooz > z_buffer[xp][yp])
						{
							z_buffer[xp][yp] = ooz;
							if (is_lighting) output[xp][yp] = dictionary[0];
							else output[xp][yp] = symbol;
						}
					}
				}
			}
		}
	}
	//To the screen
	if (is_TSU_letters)//special output for tsu letters
	{
		for (int i = 0; i < window_high; i++)
		{
			for (int j = 0; j < window_width; j++)
			{
				if (output[i][j] != ' ') putchar(TSU_letters[(i + j) % 3]);
				else putchar(' ');
			}
			putchar('\n');
		}
	}
	else//common output
	{
		for (int i = 0; i < window_high; i++)
		{
			for (int j = 0; j < window_width; j++)
			{
				putchar(output[i][j]);
			}
			putchar('\n');
		}
	}
}

int main()
{
	output = new char* [window_high];
	z_buffer = new float* [window_high];
	xyz = new float[3];
	Lxyz = new float[3];//Lightning xyz
	for (int i = 0; i < window_high; i++)
	{
		output[i] = new char[window_width];
		z_buffer[i] = new float[window_width];
	}

	int counter = 0;

	SetConsoleWindowSize(window_high, window_width);

	float alpha = 0;
	float beta = 0;
	float gamma = 0;

	while (counter < 10000)
	{
		clearScreen();
		M_FillWithValue<char>(output, window_high, window_width, ' ');
		M_FillWithValue<float>(z_buffer, window_high, window_width, NULL);
		printf("\x1b[H"); //We move the carriage to the beginning of the terminal window
		RenderFrame(alpha, beta, gamma);
		//Sleep(10);
		// 
		//angles of rotation of the figure
		alpha += 0.05f;
		//beta += 0.03f;
		//gamma += 0.03f;
		counter++;
	}
	for (int i = 0; i < window_high; i++)
	{
		delete[] output[i];
		delete[] z_buffer[i];
	}
	delete[] output;
	delete[] z_buffer;
	delete[] xyz;
	delete[] Lxyz;
	return 0;
}