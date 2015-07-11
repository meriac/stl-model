#include <stdio.h>
#include <stdint.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

#define SHAPE_SCALE 100
#define SHAPE_POINTS 100

uint32_t g_triangles;
Vector3d g_shape[SHAPE_POINTS];
Vector3d g_target[SHAPE_POINTS];

static void create_shape(void)
{
	for(int i=0; i<SHAPE_POINTS; i++)
	{
		g_shape[i](0) = sin(i*M_PI/(SHAPE_POINTS/2));
		g_shape[i](1) = cos(i*M_PI/(SHAPE_POINTS/2));
		g_shape[i](2) = 0;
	}
}

static void translate_shape(const Vector3d &translate)
{
	for(int i=0; i<SHAPE_POINTS; i++)
		g_target[i] = g_shape[i] + translate;
}

static void dump_vector(const Vector3d &v)
{
	float out[3];

	for(int i=0; i<3; i++)
		out[i] = v(i);

	fwrite(&out, sizeof(out), 1, stdout);
}

static void emit_stl_vertex(const Vector3d &v1, const Vector3d &v2, const Vector3d &v3)
{
	uint16_t attribute_count;

	/* dump normale vector */
	dump_vector(v1.cross(v2));

	/* dump vertexes */
	dump_vector(v1);
	dump_vector(v2);
	dump_vector(v3);

	/* output empty attribute field */
	attribute_count = 0;
	fwrite(&attribute_count, sizeof(attribute_count), 1, stdout);

	/* increment triangle count */
	g_triangles++;
}

static void emit_stl_layer(void)
{
	for(int i=0; i<SHAPE_POINTS; i++)
	{
		emit_stl_vertex(g_shape[i], g_target[i], g_target[(i+1)%SHAPE_POINTS]);
		emit_stl_vertex(g_shape[i], g_target[(i+1)%SHAPE_POINTS], g_shape[(i+1)%SHAPE_POINTS]);
	}
}

int main (int argc, char *argv[])
{
	int i;

	create_shape();

	for(i=0; i<SHAPE_POINTS; i++)
	{
		Vector3d translate(sin(i*M_PI/SHAPE_POINTS),cos(i*M_PI/SHAPE_POINTS),1);
		translate_shape(translate);
		emit_stl_layer();
	}

	return 0;
}
