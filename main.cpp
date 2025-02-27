#include<iostream>
#include<math.h>
#include<windows.h>
//#include<conio.h>

# define M_PI 3.14159265358979323846

using namespace std;
HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

//Если изображение "дергается", то уменьшить высоту и ширину терминала
const int window_high = 50;
const int window_width = 50;

const float theta_step = 0.09f;
const float phi_step = 0.03f;
char dictionary[12] = {'.', ',', '-', '~', ':', ';', '=', '!', '*', '#', '$', '@'};

const float R1 = 1.0f;//Радиус внутренней окружности тороида
const float R2 = 2.0f;//Расстояние от центра вращения до центра окружности
const float K2 = 10.0f;//Расстояние от наблюдателя до фигуры
const float K1 = window_width * K2 * 6 / (20*(R1 + R2));//Коэффициент масшабирования перспективной проекции

char** output;
float** z_buffer;
bool** theta_matrix;
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
float ooz;
int xp;
int yp;

void clearScreen() 
{
	std::cout << "\x1B[2J\x1B[H"; // ANSI escape-коды
}

void RotateAroundX(float* xyz, const float& cos_alpha, const float& sin_alpha)
{
	float old_y = xyz[1];
	float old_z = xyz[2];
	xyz[1] = old_y * cos_alpha + old_z * sin_alpha;
	xyz[2] = old_z * cos_alpha - old_y * sin_alpha;
}

void RotateAroundY(float* xyz, const float& cos_phi, const float& sin_phi)
{
	float old_x = xyz[0];
	float old_z = xyz[2];
	xyz[0] = old_x * cos_phi - old_z * sin_phi;
	xyz[2] = old_x * sin_phi + old_z * cos_phi;
}

void RotateAroundZ(float* xyz, const float& cos_beta, const float& sin_beta)
{
	float old_x = xyz[0];
	float old_y = xyz[1];
	xyz[0] = old_x * cos_beta + old_y * sin_beta;
	xyz[1] = old_y * cos_beta - old_x * sin_beta;
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
			arr[i][j] = (T)value;
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

void RenderFrame(float alpha, float beta)//Каждый фрейм будет повернут на alpha и beta соответственно по оси ОX и ОY.
{
	//Прерасчет sin cos поворота
	sin_alpha = sin(alpha);
	cos_alpha = cos(alpha);
	sin_beta = sin(beta);
	cos_beta = cos(beta);

	for (float theta = 0; theta < 2 * M_PI; theta += theta_step)//Окружность на XOY
	{
		cos_theta = cos(theta);
		sin_theta = sin(theta);
		for (float phi = 0; phi < 2 * M_PI; phi += phi_step) //Вращение по OY
		{
			cos_phi = cos(phi);
			sin_phi = sin(phi);

			float circlex = R2 + R1 * cos_theta;//Начальные координаты круга
			float circley = R1 * sin_theta;

			xyz[0] = R2 + (R1 * cos_theta);
			xyz[1] = R1 * sin_theta;
			xyz[2] = 0;

			RotateAroundY(xyz, cos_phi, sin_phi);
			RotateAroundX(xyz, cos_alpha, sin_alpha);
			RotateAroundZ(xyz, cos_beta, sin_beta);
			xyz[2] += K2;

			ooz = 1.0f / xyz[2]; //"one over z". Выгоднее один раз разделить, затем дважды умножить, чем дважды делить
			xp = static_cast<int>(window_width/2 + xyz[0] * ooz * K1);
			yp = static_cast<int>(window_high/2 - xyz[1] * ooz * K1);

			//Вектор нормали к поверхности
			Lxyz[0] = cos_theta;
			Lxyz[1] = sin_theta;
			Lxyz[2] = 0;
			RotateAroundY(Lxyz, cos_phi, sin_phi);
			RotateAroundX(Lxyz, cos_alpha, sin_alpha);
			RotateAroundZ(Lxyz, cos_beta, sin_beta);

			float light_intensity = Lxyz[1] - Lxyz[2];//Домножаем на вектор света (0,1,-1). Вектор света не нормализован, имеет длину sqrt(2) -> изменяется в диапазоне (-sqrt(2), sqrt(2))

			if (light_intensity > 0)//light_intensity - косинус угла между вектором света и нормали
			{
				if (ooz > z_buffer[xp][yp])
				{
					if(theta < M_PI) theta_matrix[xp][yp] = 1;
					else theta_matrix[xp][yp] = 0;
					z_buffer[xp][yp] = ooz;
					int luminance_index = static_cast<int>(light_intensity * 8);//Теперь в диапазоне (0, 11)
					output[xp][yp] = dictionary[luminance_index];
				}
			}
		}
	}
	bool upside_down = false;
	for(int i = 0; i < window_high; i++)
	{
		for (int j = 0; j < window_width; j++)
		{
			if(theta_matrix[i][j]) SetConsoleTextAttribute(handle, FOREGROUND_RED);
			else SetConsoleTextAttribute(handle, FOREGROUND_BLUE);
			putchar(output[i][j]);
		}
		putchar('\n');
	}
}

int main() 
{
	output = new char* [window_high];
	z_buffer = new float* [window_high];
	theta_matrix = new bool* [window_high];
	xyz = new float[3];
	Lxyz = new float[3];
	for (int i = 0; i < window_high; i++)
	{
		theta_matrix[i] = new bool[window_width];
		output[i] = new char[window_width];
		z_buffer[i] = new float[window_width];
	}

	int size = 3;
	int counter = 0;
	SetConsoleWindowSize(window_high, window_width); // Изменить размер окна

	float alpha = 0;
	float beta = 0;
	counter = 0;

	while (counter < 10000)
	{
		clearScreen();
		M_FillWithValue<char>(output, window_high, window_width, ' ');
		M_FillWithValue<float>(z_buffer, window_high, window_width, NULL);
		printf("\x1b[H"); //Переводим каретку в самое начало окна терминала
		RenderFrame(alpha, beta);
		//Sleep(10);
		alpha += 0.125f;//при бОльших занчениях получается бОльшая разница между кадрами, тк бОльший угол поворота фигуры
		beta += 0.03f;
		counter++;
	}
	for (int i = 0; i < window_high; i++)
	{
		delete[] output[i];
		delete[] z_buffer[i];
		delete[] theta_matrix[i];
	}
	delete[] output;
	delete[] z_buffer;
	delete[] theta_matrix;
	delete[] xyz;
	delete[] Lxyz;
	return 0;
}