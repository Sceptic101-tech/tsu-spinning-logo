#include<iostream>
#include<math.h>
#include<windows.h>

//Все комментарии на английском

# define M_PI 3.14159265358979323846

using namespace std;
HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

//If the image is "twitching", then reduce the height and width of the terminal.
const int window_high = 50;
const int window_width = 50;

const float half_cube_side = 2;

const float theta_step = 0.09f;
const float phi_step = 0.03f;

const float x_step = 0.03f;
const float y_step = 0.03f;
char dictionary[12] = { '.', ',', '-', '~', ':', ';', '=', '!', '*', '#', '$', '@' };

const float K2 = 20.0f;//The distance from the observer to the figure
const float K1 = window_width * K2 * 6 / (20 * (half_cube_side));//The zoom level of the perspective projection to 2D

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
float ooz;
int xp;
int yp;

void clearScreen()
{
	std::cout << "\x1B[2J\x1B[H"; // ANSI escape-code
}

void RotateAroundX(float* xyz, const float& cos_alpha, const float& sin_alpha)
{
	float old_y = xyz[1];
	//float old_z = xyz[2];
	xyz[1] = old_y * cos_alpha + xyz[2] * sin_alpha;
	xyz[2] = xyz[2] * cos_alpha - old_y * sin_alpha;
}

void RotateAroundY(float* xyz, const float& cos_phi, const float& sin_phi)
{
	float old_x = xyz[0];
	//float old_z = xyz[2];
	xyz[0] = old_x * cos_phi - xyz[2] * sin_phi;
	xyz[2] = old_x * sin_phi + xyz[2] * cos_phi;
}

void RotateAroundZ(float* xyz, const float& cos_beta, const float& sin_beta)
{
	float old_x = xyz[0];
	//float old_y = xyz[1];
	xyz[0] = old_x * cos_beta + xyz[1] * sin_beta;
	xyz[1] = xyz[1] * cos_beta - old_x * sin_beta;
}

template<typename T>
void Print(T** arr, int& size)
{
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			cout << arr[i][j] << " ";
		}
		cout << endl;
	}
}

template<typename T>
void Print(T** arr, const int& size)
{
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			cout << arr[i][j] << " ";
		}
		cout << endl;
	}
}

template<typename T>
void SetZeros(T** arr, int& size)
{
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			arr[i][j] = 0;
		}
	}
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
	// Устанавливаем размер буфера экрана
	COORD bufferSize = { static_cast<SHORT>(width), static_cast<SHORT>(height) };
	SetConsoleScreenBufferSize(handle, bufferSize);

	// Меняем размер окна
	SMALL_RECT rect = { 0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height - 1) };
	SetConsoleWindowInfo(handle, TRUE, &rect);
}

void RenderFrame(float alpha, float beta, float gamma)//Каждый фрейм будет повернут на alpha и beta соответственно по оси ОX и ОY.
{
	//Прерасчет sin cos поворота
	sin_alpha = sin(alpha);
	cos_alpha = cos(alpha);
	sin_beta = sin(beta);
	cos_beta = cos(beta);
	sin_gamma = sin(gamma);
	cos_gamma = cos(gamma);

	for (float y = -half_cube_side; y <= half_cube_side; y += y_step)
	{
		for (float x = -half_cube_side; x <= half_cube_side; x += x_step)
		{
			xyz[0] = x;
			xyz[1] = y;
			xyz[2] = 0;
			RotateAroundY(xyz, cos_gamma, sin_gamma);
			RotateAroundX(xyz, cos_alpha, sin_alpha);
			RotateAroundZ(xyz, cos_beta, sin_beta);
			xyz[2] += K2;
			ooz = 1.0f / xyz[2]; //"one over z". Выгоднее один раз разделить, затем дважды умножить, чем дважды делить
			xp = static_cast<int>(window_width / 2 + xyz[0] * ooz * K1);
			yp = static_cast<int>(window_high / 2 - xyz[1] * ooz * K1);

			//Вектор нормали к поверхности
			Lxyz[0] = 1;
			Lxyz[1] = 1;
			Lxyz[2] = 0;
			RotateAroundY(Lxyz, cos_gamma, sin_gamma);
			RotateAroundX(Lxyz, cos_alpha, sin_alpha);
			RotateAroundZ(Lxyz, cos_beta, sin_beta);

			float light_intensity = Lxyz[1] - Lxyz[2];//Домножаем на вектор света (0,1,-1). Вектор света не нормализован, имеет длину 2 -> изменяется в диапазоне (2, 2)
			if (xp < 0 || xp >= window_width || yp < 0 || yp >= window_high) continue;
			output[xp][yp] = '#';

			if (light_intensity > 0)//light_intensity - косинус угла между вектором света и нормали
			{
				if (ooz > z_buffer[xp][yp])
				{
					z_buffer[xp][yp] = ooz;
					int luminance_index = static_cast<int>(light_intensity * 5.5f);//Теперь в диапазоне (0, 11)
					//output[xp][yp] = dictionary[luminance_index];
					//output[xp][yp] = '#';
				}
			}
		}
	}

	bool upside_down = false;
	for (int i = 0; i < window_high; i++)
	{
		for (int j = 0; j < window_width; j++)
		{
			putchar(output[i][j]);
		}
		putchar('\n');
	}
}

int main()
{
	output = new char* [window_high];
	z_buffer = new float* [window_high];
	xyz = new float[3];
	Lxyz = new float[3];
	for (int i = 0; i < window_high; i++)
	{
		output[i] = new char[window_width];
		z_buffer[i] = new float[window_width];
	}

	int size = 3;
	int counter = 0;

	SetConsoleWindowSize(window_high, window_width); // Изменить размер окна

	float alpha = 0;
	float beta = 0;
	float gamma = 0;
	counter = 0;

	while (counter < 10000)
	{
		clearScreen();
		M_FillWithValue<char>(output, window_high, window_width, ' ');
		M_FillWithValue<float>(z_buffer, window_high, window_width, NULL);
		printf("\x1b[H"); //Переводим каретку в самое начало окна терминала
		RenderFrame(alpha, beta, gamma);
		//Sleep(10);
		alpha += 0.03f;//при бОльших занчениях получается бОльшая разница между кадрами, тк бОльший угол поворота фигуры
		beta += 0.03f;
		gamma += 0.03f;
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