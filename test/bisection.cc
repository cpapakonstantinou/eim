#include <libopt.h>
#include <iostream>

using namespace std;
using namespace opt;

double f(double x)
{
	return 2*x - 5;
}

int main(int argc, char const *argv[])
{

	double root = bisection(f, -6, 6);

	cout << "root: " << root << endl;
	
	return 0;
}