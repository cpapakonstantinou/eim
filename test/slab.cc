#include <strip.h>
#include <iostream>
#include <log.h>
#include <carray.h>

using namespace std;
using namespace eim;

int main(int argc, char const *argv[])
{
	double n1 = 1.44, n2 = 3.47, n3 = 1.44;
	double lam = 1.55;//um
	int mode = 0;
	double W = 0.5;//um
	int pts = 200;

	cvector<double> x(pts);
	vec::linspace<double>(x.begin(), x.end(), -1, 1);
	cvector<field_t> E (pts);
	cvector<field_t> H (pts);
	cvector<field_t> l (pts); // longitudinal field
	cvector<field_t> n (pts); // normal field

	try
	{
		Log log("slab.csv");

		auto neff = solve_slab(n1, n2, n3, lam, W, mode);

		printf("TE%d:%g\nTM%d:%g\n", mode, get<0>(neff), mode, get<1>(neff));
		
		mode_1D<Mode::TE>(x.begin(),x.end(), E.begin(), l.begin(), n.begin(), get<0>(neff), n1, n2, n3, lam, W, mode);

		mode_1D<Mode::TM>(x.begin(),x.end(), H.begin(), l.begin(), n.begin(), get<1>(neff), n1, n2, n3, lam, W, mode);
		
		for( size_t i = 0; i < pts; i++)
		{
			log << lam << W << mode << x[i] << E[i].real() << H[i].real();
			++log;
		}
	}
	catch(const exception& ex)
	{
		cerr << ex.what() << endl;
		return -1;
	}
	return 0;
}