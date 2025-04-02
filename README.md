# Effective Index Method

## Introduction
The **Effective Index Method** provides a framework for the analysis of two-dimensional (2D) optical waveguide structures by simplifying the problem into a series of repeating, one-dimensional (1D), slab optical waveguide analyses. For which the analytical solution of a slab waveguide is known.

## Licensing
 eim is released under the MIT license. This library is distributed WITHOUT ANY WARRANTY. For details, see the file named 'LICENSE', and license statements in the source files.

## Usage Examples

1. Specify refractive indices of the three layer slab, the mode orders and widths to solve.
```bash

./eim -n 1.44,3.47,1.44  -j 0,1 -w 0.1,0.2,0.3,0.4,0.5

t_slab,t_rib,width,mode,neff
0,0.22,0.1,TE0,1.47821
0,0.22,0.1,TE1,1.44
0,0.22,0.2,TE0,1.65145
0,0.22,0.2,TE1,1.44
0,0.22,0.3,TE0,2.01259
0,0.22,0.3,TE1,1.44
0,0.22,0.4,TE0,2.31116
0,0.22,0.4,TE1,1.46452
0,0.22,0.5,TE0,2.48433
0,0.22,0.5,TE1,1.58088

```
If the mode is unsupported the refractive index will be the min of the box and cladding layers.

2. Plot the output of a sweep by redirecting the stdout to a file.

```bash
./eim -j 0,1 -w 0.1,0.2,0.3,0.4,0.5 > eim.csv

make plot_eim
```

3. Calculate the TEO 2D field amplitude in the transverse plane and plot it. 
```bash

./eim -n 1.44,3.47,1.44 -m TE -j 0 -w 0.5 -O 

make plot_mode TARGET_MODE_MODE=TE0 TARGET_MODE_WIDTH=0.5
```

## Effective Index Method Concept
Consider the scalar wave equation:

$$ \frac{\partial^2 \phi(x, y)}{\partial x^2} + \frac{\partial^2 \phi(x, y)}{\partial y^2} + k_0^2 \left(n_{e}^{2}(x, y) - n_{eff}^2 \right) \phi(x, y) = 0 $$

where n<sub>eff</sub> is the effective index to be determined.

### Separation of Variables
The wave function is separated into two functions:

$$ \phi(x, y) = f(x) \cdot g(y) $$

Substituting this into the wave equation and dividing by phi(x, y), one obtains:

$$ \frac{1}{f(x)} \frac{d^2 f(x)}{dx^2} + \frac{1}{g(y)} \frac{d^2 g(y)}{dy^2} + k_0^2 \left[n_{e}^{2}(x, y) - n_{\text{eff}}^2 \right] = 0. $$

### Independent Equations
By separating variables, derive two independent equations:

$$ \frac{d^2 g(y)}{dy^2} + k_0^2[n_{e}^{2}(x, y) - N^2(x)] g(y) = 0 $$

$$ \frac{d^2 f(x)}{dx^2} - k_0^2 \left( N^2(x) - n_{eff}^{2} \right) f(x) = 0 $$

### Calculation Procedure
The effective index calculation follows these steps:

1. Replace the 2D optical waveguide with a combination of 1D optical waveguides.
2. For each 1D optical waveguide, calculate the effective index along the y-axis.
3. Model an optical slab waveguide by placing the effective indices from step (2) along the x-axis.
4. Obtain the overall effective index by solving the modal equation along the x-axis.

**Note:** For TE modes in 2D optical waveguides, we first solve for TE-mode analysis and then for TM-mode analysis.

```c++
auto n1 = solve_slab(n_box, (t_slab ? n_core : n_clad), n_clad, wavelength, t_slab, 0);
auto n2 = solve_slab(n_box, n_core, n_clad, wavelength, t_rib, 0);
const auto& n3 = n1;

// The TE mode of the waveguide is the TM mode of the analysis
// For TM mode analysis, it is the opposite order
if (mode ==  TE)
{
	auto neff = solve_slab(get<0>(n1), get<0>(n2), get<0>(n3), wavelength, w_rib, mode_order);
	return get<1>(neff);
}
else //(mode == TM)
{
	auto neff = solve_slab(get<1>(n1), get<1>(n2), get<1>(n3), wavelength, w_rib, mode_order);
	return get<0>(neff);
}

```


### 1D TE Mode Equation Derivation
The Transverse Electric (TE) mode is purely transverse for a slab waveguide. To begin the proof, expand Maxwell’s equations in the absence of free charges or currents. This will later be related to the TE mode condition.

$$
\nabla \times \mathbf{E} = -j\omega\mu_0 \mathbf{H}
$$

$$
\nabla \times \mathbf{H} = j\omega\varepsilon_r \mathbf{E}
$$

### Component Form
Expanding these in Cartesian coordinates:

#### **Faraday's Law:**

$$
\frac{\partial E_z}{\partial y} - \frac{\partial E_y}{\partial z} = -j\omega\mu_0 H_x,
$$

$$
\frac{\partial E_x}{\partial z} - \frac{\partial E_z}{\partial x} = -j\omega\mu_0 H_y,
$$

$$
\frac{\partial E_y}{\partial x} - \frac{\partial E_x}{\partial y} = -j\omega\mu_0 H_z.
$$

#### **Ampère's Law:**

$$
\frac{\partial H_z}{\partial y} - \frac{\partial H_y}{\partial z} = j\omega\varepsilon_r E_x,
$$

$$
\frac{\partial H_x}{\partial z} - \frac{\partial H_z}{\partial x} = j\omega\varepsilon_r E_y,
$$

$$
\frac{\partial H_y}{\partial x} - \frac{\partial H_x}{\partial y} = j\omega\varepsilon_r E_z.
$$

### TE Mode Condition

The TE mode condition is that the normal component of the electic field does not exist. Meaning that the field is purely transverse.

$$
E_z = 0
$$

From **Ampère’s Law**, the third equation:


$$
\frac{\partial H_y}{\partial x} - \frac{\partial H_x}{\partial y} = j\omega\varepsilon_r E_z
$$

Since \( E_z = 0 \), this equation reduces to:


$$
\frac{\partial H_y}{\partial x} - \frac{\partial H_x}{\partial y} = 0.
$$

Thus, the magnetic field, H, can have a normal component, Hz, but the electric field remains purely in the transverse plane.
 
### Derivation of the Characteristic Equation

By considering the transverse nature of the fields and applying boundary conditions, the TE mode wave equation can be expressed as:

$$ \frac{\partial^2 E_y}{\partial x^2} + k_0^2 (n^2 - n_{eff}^2) E_y = 0 $$

The general solutions for the electric field in each region are:

**Buried Oxide Region**:

$$ E_y(x) = C_1 exp(\gamma_1 x ) \quad for \quad  (x < 0) $$

**Core Region**: 

$$ E_y(x) = C_2 cos(\gamma_2 x + \alpha) \quad for \quad (x \geq 0, x \leq W) $$

**Cladding Region**: 

$$ E_y(x) = C_3 exp(-\gamma_3 (x - W)) \quad for \quad (x > W) $$

Applying boundary conditions at the interfaces, we equate the tangential field components:

1. Continuity of Electric Field:

$$ E_y \quad at \quad x = 0 \quad and \quad x = W $$

3. Continuity of Magnetic Field:

$$ \frac{dE_y}{dx} \quad at \quad x = 0 \quad and \quad x = W $$

Which leads to the characteristic equation:

$$ 
	\gamma_2 W = -tan^{-1}\left(\frac{\gamma_2}{\gamma_1}\right) -tan^{-1}\left(\frac{\gamma_2}{\gamma_3}\right) + (j+1)\pi \quad (j=0,1,2,...)
$$

This equation determines the discrete set of allowed effective indices n<sub>eff</sub> for the waveguide modes. A similar derivation applies for the transverse magnetic, **TM Mode**.

```c++
template<Mode mode>
double slab_equation(double n1, double n2, double n3, double lambda, double W, int j, double neff)
{
	double k0 = 2*pi*(1 / lambda); 
	double gamma1 = k0*sqrt( pow(neff, 2) - pow(n1, 2) );
	double gamma2 = k0*sqrt( pow(n2, 2) - pow(neff, 2) );
	double gamma3 = k0*sqrt( pow(neff, 2) - pow(n3, 2) );

	double lhs = gamma2 * W;
	if constexpr (mode == TE)
	{
		double rhs = -atan2(gamma2, gamma1) - atan2(gamma2, gamma3) + (j+1)*pi;
		return rhs - lhs;
	}
	if constexpr (mode == TM)
	{
		double rhs = -atan2(pow(n1, 2) * gamma2, pow(n2, 2) * gamma1) - 
			atan2( pow(n3, 2) * gamma2, pow(n2, 2) * gamma3 ) + (j+1)*pi;
		return rhs - lhs;
	}
}
```

## References

[1] K. Kawano and T. Kitoh, Introduction to optical waveguide analysis: solving Maxwell’s equations and the Schrödinger equation. New York: Wiley, 2004.
