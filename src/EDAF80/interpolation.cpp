#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{

	return (1.0f - x) * p0 + x * p1;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	glm::vec3 const m1 = t * (p2 -  p0);
	glm::vec3 const m2 = t * (p3 - p1);
	
	float const x2 =  x * x;
	float const x3 = x2 * x;
	
	return (2.0f * x3 - 3.0f* x2 + 1.0f) * p1
	+ (x3 - 2.0f * x3 + x ) * m1
	+ (-2.0f * x3 + 3.0f * x2) * p2
	+ (x3 - x2) * m2;
	

}
