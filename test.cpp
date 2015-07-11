#include <stdio.h>
#include <stdint.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

typedef enum {BorderTop, BorderBottom} TBorderType;

#define SHAPE_SCALE 100
#define SHAPE_POINTS 100
#define MALLOC_STEPS (1024UL*1024)
#define HEADER_SIZE (80+sizeof(g_triangles))
#define SHAPE_BORDER_WIDTH 3

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
		g_buffer = (uint8_t*) realloc(g_buffer, g_buffer_size+MALLOC_STEPS);
		if(!g_buffer)
		{
			perror("Error: Failed to reallocate memory\n");
			exit(1);
		}

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
		g_shape[i](0) = (sin(i*M_PI/(SHAPE_POINTS/2)) + (cos(i*M_PI/(SHAPE_POINTS/14))/17)) * radius;
		g_shape[i](1) = (cos(i*M_PI/(SHAPE_POINTS/2)) - (sin(i*M_PI/(SHAPE_POINTS/9))/13)) * radius;
		g_shape[i](2) = 0;
	}
}

static void translate_shape(const Vector3d &translate, int index)
{
	Matrix3d m;
	m = AngleAxisd(M_PI/(SHAPE_POINTS/2),Vector3d::UnitZ()) * Scaling(1+0.01*(((double)index)/SHAPE_POINTS));

	for(int i=0; i<SHAPE_POINTS; i++)
		g_target[i] = (m * g_shape[i]) + translate;
}

static void dump_vector(const Vector3d &v)
{
	float *out = static_cast<float*>(add_stl(sizeof(*out)*3));

	for(int i=0; i<3; i++)
		*out++ = v(i);
}

static void translate_inner_border(const Vector3d &vec, Vector3d &out)
{
	/* determine center */
	Vector3d center(0, 0, vec(2));

	Vector3d n = (center - vec).normalized();

	out = vec + (n*SHAPE_BORDER_WIDTH);
}

static void emit_stl_vertex(const Vector3d &v1, const Vector3d &v2, const Vector3d &v3)
{
	Vector3d k1, k2;
	uint16_t *attribute_count;

	/* dump normale vector for outside */
	k1 = v2 - v1;
	k2 = v3 - v1;
	dump_vector(k1.cross(k2).normalized());

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

void emit_stl_border(const Vector3d &v1, const Vector3d &v2, const Vector3d &v3)
{
	Vector3d k1, k2, border[3];
	uint16_t *attribute_count;

	/* dump normale vector for outside */
	k1 = v2 - v1;
	k2 = v3 - v1;
	dump_vector(k1.cross(k2).normalized());

	/* translate outer points to inner points */
	translate_inner_border(v1, border[0]);
	translate_inner_border(v2, border[1]);
	translate_inner_border(v3, border[2]);

	/* dump vertexes */
	dump_vector(border[0]);
	dump_vector(border[1]);
	dump_vector(border[2]);

	/* output empty attribute field */
	attribute_count = static_cast<uint16_t*>(add_stl(sizeof(*attribute_count)));
	*attribute_count = 0;

	/* increment triangle count */
	g_triangles++;
}

static void emit_stl_layer(bool border)
{
	Vector3d target2, shape2;

	for(int i=0; i<SHAPE_POINTS; i++)
	{
		target2 = g_target[(i+1)%SHAPE_POINTS];
		shape2 = g_shape[(i+1)%SHAPE_POINTS];

		emit_stl_vertex(g_shape[i], g_target[i], target2);
		emit_stl_vertex(g_shape[i], target2, shape2);

		if(border)
		{
			emit_stl_border(target2,  g_target[i], g_shape[i]);
			emit_stl_border(shape2, target2, g_shape[i]);
		}
	}
}

static void emit_shaped_plane(void)
{
	/* determine center */
	Vector3d center(0, g_shape[0](2), 0);

	for(int i=0; i<SHAPE_POINTS; i++)
		emit_stl_vertex(g_shape[i], g_shape[(i+1)%SHAPE_POINTS], center);
}

static void emit_shaped_border(double border, TBorderType type)
{
	Vector3d p[4];

	/* determine center */
	Vector3d center(0, 0, g_shape[0](2));

	for(int i=0; i<SHAPE_POINTS; i++)
	{
		p[0] = g_shape[i];
		p[1] = g_shape[(i+1)%SHAPE_POINTS];

		/* calculate inner border */
		translate_inner_border(p[0], p[2]);
		translate_inner_border(p[1], p[3]);

		switch(type)
		{
			case BorderTop:
				emit_stl_vertex(p[1],p[0],p[2]);
				emit_stl_vertex(p[1],p[2],p[3]);
				break;

			case BorderBottom:
				emit_stl_vertex(p[3],p[2],center);
				break;
		}
	}
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
		/* translate all points */
		translate_shape(Vector3d::UnitZ(), i);

		/* emit resulting layer */
		emit_stl_layer(i>=3);

		/* emit inner bottom */
		if(i==3)
			emit_shaped_border(SHAPE_BORDER_WIDTH, BorderBottom);

		/* move to next layer */
		tmp = g_shape;
		g_shape = g_target;
		g_target = tmp;

	}

	/* emit top border */
	emit_shaped_border(SHAPE_BORDER_WIDTH, BorderTop);

	/* copy triangle coun  to header */
	memcpy(g_buffer + 80, &g_triangles, sizeof(g_triangles));

	/* dump whole file */
	fwrite(g_buffer, g_buffer_pos, 1, stdout);

	return 0;
}


