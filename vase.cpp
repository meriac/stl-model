/***************************************************************
 *
 * Create a paremetric STL binary 3D-printing model of a vase
 *
 * The Cup shape consists of multiple overlaid sine waves and is
 * twisted around the Z-axis.
 *
 * Copyright 2015 Milosch Meriac <milosch.meriac.com>
 *
 ***************************************************************/

/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published
 by the Free Software Foundation; version 3.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "stl-helper.h"

typedef enum {BorderTop, BorderBottom} TBorderType;

#define SHAPE_SCALE 100
#define SHAPE_POINTS 100
#define SHAPE_BORDER_WIDTH 3

Vector3d g_vector_buffer[2][SHAPE_POINTS];
Vector3d *g_shape;
Vector3d *g_target;

static void create_shape(double radius)
{
	/* create simple extrusion shape - a wobbly circle */
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
	/* rotate and scale shape depending on height along Z-axis */
	m =   AngleAxisd(M_PI/(SHAPE_POINTS/2),Vector3d::UnitZ())
		* Scaling(1+0.01*(((double)index)/SHAPE_POINTS));

	/* apply scaling matrix to all shape points */
	for(int i=0; i<SHAPE_POINTS; i++)
		g_target[i] = (m * g_shape[i]) + translate;
}

static void translate_inner_border(const Vector3d &vec, Vector3d &out)
{
	/* determine center */
	Vector3d center(0, 0, vec(2));

	/* calculate normale vector towards center */
	Vector3d n = (center - vec).normalized();

	/* move outer point to inner surface border */
	out = vec + (n*SHAPE_BORDER_WIDTH);
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

		/* emit outer surface */
		emit_stl_vertex(g_shape[i], g_target[i], target2);
		emit_stl_vertex(g_shape[i], target2, shape2);

		/* emit inner surface */
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

	/* emit STL header */
	dump_header();

	/* initial; buffer setup */
	g_shape = g_vector_buffer[0];
	g_target = g_vector_buffer[1];

	/* create shape */
	create_shape(SHAPE_POINTS/2.5);

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

	/* emit full stl file on stdout */
	dump_stl(stdout);

	return 0;
}
