#include<iostream>
using namespace std;

//1、值传递
void swap01(int num1, int num2)
{
	int temp = num1;
	num1 = num2;
	num2 = temp;
}

//2、地址传递
void swap02(int* num1, int* num2)
{
	int temp = *num1;
	*num1 = *num2;
	*num2 = temp;
}

//3、引用传递，可以改变实参 
void swap03(int& num1, int& num2)
{
	int temp = num1;
	num1 = num2;
	num2 = temp;
}

int main()
{
	int a = 10;
	int b = 20;

	swap01(a, b);
	cout << "swap01值传递：" << endl;
	cout << "a = " << a << endl;
	cout << "b = " << b << endl;

	swap02(&a, &b);
	cout << "swap02地址传递：" << endl;
	cout << "a = " << a << endl;
	cout << "b = " << b << endl;

	swap03(a, b);
	cout << "swap03引用传递：" << endl;
	cout << "a = " << a << endl;
	cout << "b = " << b << endl;

	system("pause");
	return 0;

}