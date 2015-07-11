#include <stdio.h>
#include <stdint.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

#define SHAPE_SCALE 100
#define SHAPE_POINTS 100
#define MALLOC_STEPS (1024UL*1024)
#define HEADER_SIZE (80+sizeof(g_triangles))

uint32_t g_triangles;

Vector3d g_vector_buffer[2][SHAPE_POINTS];
Vector3d *g_shape;
Vector3d *g_target;

uint8_t *g_buffer;
uint32_t g_buffer_pos, g_buffer_size;

static void* add_stl(uint32_t bytes)
{
	void* res;

	if((g_buffer_pos+bytes) > (g_buffer_size))
	{
		g_buffer = (uint8_t*) realloc(g_buffer, MALLOC_STEPS);
		if(!g_buffer)
			return NULL;

		g_buffer_size += MALLOC_STEPS;
	}

	res = g_buffer + g_buffer_pos;
	g_buffer_pos += bytes;

	return res;
}

static void create_shape(double radius)
{
	for(int i=0; i<SHAPE_POINTS; i++)
	{
		g_shape[i](0) = (sin(i*M_PI/(SHAPE_POINTS/2)) + (cos(i*M_PI/(SHAPE_POINTS/6))/7)) * radius;
		g_shape[i](1) = (cos(i*M_PI/(SHAPE_POINTS/2)) - (sin(i*M_PI/(SHAPE_POINTS/9))/13)) * radius;
		g_shape[i](2) = 0;
	}
}

static void translate_shape(const Vector3d &translate, double progress_percent)
{
	Matrix3d m;
	m = AngleAxisd(M_PI/(SHAPE_POINTS/2),Vector3d::UnitZ()) * Scaling(1+0.01*progress_percent);

	for(int i=0; i<SHAPE_POINTS; i++)
		g_target[i] = (m * g_shape[i]) + translate;
}

static void dump_vector(const Vector3d &v)
{
	float *out = static_cast<float*>(add_stl(sizeof(float)*3));

	for(int i=0; i<3; i++)
		*out++ = v(i);
}

static void emit_stl_vertex(const Vector3d &v1, const Vector3d &v2, const Vector3d &v3)
{
	Vector3d k1, k2, n;
	uint16_t *attribute_count;

	/* dump normale vector for inside */
	k1 = v2 - v1;
	k2 = v3 - v1;
	n = k1.cross(k2).normalized();
	dump_vector(n);

	/* dump vertexes */
	dump_vector(v1);
	dump_vector(v2);
	dump_vector(v3);

	/* output empty attribute field */
	attribute_count = static_cast<uint16_t*>(add_stl(sizeof(*attribute_count)));
	*attribute_count = 0;

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

static void emit_shaped_plane(void)
{
	/* determine center */
	Vector3d center(0, g_shape[0](2), 0);

	for(int i=0; i<SHAPE_POINTS; i++)
		emit_stl_vertex(g_shape[i], g_shape[(i+1)%SHAPE_POINTS], center);
}

int main (int argc, char *argv[])
{
	int i;
	Vector3d *tmp;
	void* buf;

	/* populate header */
	buf = add_stl(HEADER_SIZE);
	if(!buf)
		return 1;
	memset(buf, 0, HEADER_SIZE);

	/* initial; buffer setup */
	g_shape = g_vector_buffer[0];
	g_target = g_vector_buffer[1];

	/* create shape */
	create_shape(SHAPE_POINTS/3);

	/* emit bottom */
	emit_shaped_plane();

	/* emit sculpture */
	for(i=0; i<SHAPE_POINTS; i++)
	{
		/* calculate normalized translation vector */
		Vector3d translate(0,0,1);
		translate.normalize();

		/* translate all points */
		translate_shape(translate, ((double)i)/SHAPE_POINTS );
		/* emit resulting layer */
		emit_stl_layer();

		/* move to next layer */
		tmp = g_shape;
		g_shape = g_target;
		g_target = tmp;
	}

	/* copy triangle coutn  to header */
	memcpy(g_buffer + 80, &g_triangles, sizeof(g_triangles));

	/* dump whole file */
	fwrite(g_buffer, g_buffer_pos, 1, stdout);

	return 0;
}

