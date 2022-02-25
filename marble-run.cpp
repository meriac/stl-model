/***************************************************************
 *
 * Create a paremetric STL binary
 * 3D-printing model of a marble-run
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

#define SHAPE_POINTS 42
#define MARBLE_RUN_WALL 2
#define MARBLE_RUN_RADIUS 60
#define MARBLE_RUN_RESOLUTION 1000
#define MARBLE_RUN_SEGMENT_HEIGHT (20)

Vector3d g_vector_buffer[2][SHAPE_POINTS];
Vector3d g_vector_inner[2][SHAPE_POINTS];
Vector3d *g_shape;
Vector3d *g_target;

static void create_shape(double diameter)
{
	/* create simple extrusion shape - a circle */
	for(int i=0; i<SHAPE_POINTS; i++)
	{
		g_shape[i](0) = MARBLE_RUN_RADIUS + sin(i*M_PI/(SHAPE_POINTS/2)) * diameter;
		g_shape[i](1) = 0;
		g_shape[i](2) = cos(i*M_PI/(SHAPE_POINTS/2)) * diameter;
	}
}

static void translate_shape(const Vector3d &translate, double modulation)
{
	Vector3d mod;

	/* rotate and scale shape depending on height along Z-axis */
	Matrix3d m;
	m = AngleAxisd(M_PI/(MARBLE_RUN_RESOLUTION/2),Vector3d::UnitZ());

	/* calculate modulation vector */
	mod = Vector3d(g_shape[0](0), g_shape[0](1), 0);
	mod = mod.normalized()*modulation + translate;

	/* apply scaling matrix to all shape points */
	for(int i=0; i<SHAPE_POINTS; i++)
		g_target[i] = (m * g_shape[i]) + mod;
}

static void emit_stl_layer(bool inner)
{
	Vector3d target2, shape2;

	for(int i=0; i<SHAPE_POINTS; i++)
	{
		target2 = g_target[(i+1)%SHAPE_POINTS];
		shape2 = g_shape[(i+1)%SHAPE_POINTS];

		if(inner)
		{
			/* emit inner surface */
			emit_stl_vertex(g_shape[i], g_target[i], target2);
			emit_stl_vertex(g_shape[i], target2, shape2);
		}
		else
		{
			/* emit outer surface */
			emit_stl_vertex(target2, g_target[i], g_shape[i]);
			emit_stl_vertex(shape2, target2, g_shape[i]);
		}
	}
}

static void emit_cap(Vector3d *inner, Vector3d *outer, bool start)
{
	Vector3d inner2, outer2;

	for(int i=0; i<SHAPE_POINTS; i++)
	{
		inner2 = inner[(i+1)%SHAPE_POINTS];
		outer2 = outer[(i+1)%SHAPE_POINTS];

		if(start)
		{
			emit_stl_vertex(outer2, outer[i], inner[i]);
			emit_stl_vertex(inner2, outer2, inner[i]);
		}
		else
		{
			emit_stl_vertex(inner[i], outer[i], outer2);
			emit_stl_vertex(inner[i], outer2, inner2);
		}
	}
}

static void emit_spiral(bool inner)
{
	double mod;
	Vector3d *tmp;
	Vector3d translate;
	int i;

	/* initial; buffer setup */
	g_shape = g_vector_buffer[0];
	g_target = g_vector_buffer[1];

	/* create shape */
	create_shape(inner ?
		MARBLE_RUN_SEGMENT_HEIGHT/2 :
		MARBLE_RUN_SEGMENT_HEIGHT/2+MARBLE_RUN_WALL
	);

	/* remember start */
	if(inner)
		memcpy(&g_vector_inner[0], g_shape, sizeof(g_vector_inner[0]));
	else
		emit_cap(g_vector_inner[0], g_shape, true);

	/* emit sculpture */
	translate = Vector3d::UnitZ()*(MARBLE_RUN_SEGMENT_HEIGHT+MARBLE_RUN_WALL)/MARBLE_RUN_RESOLUTION;
	for(i=0; i<(6*MARBLE_RUN_RESOLUTION); i++)
	{
		if(inner)
			mod = sin(i*M_PI/(((i / ((MARBLE_RUN_RESOLUTION)/10)) % 4)+1.5))/7;
		else
			mod = 0;

		/* translate all points */
		translate_shape(translate, mod);

		/* emit resulting layer */
		emit_stl_layer(inner);

		/* move to next layer */
		tmp = g_shape;
		g_shape = g_target;
		g_target = tmp;
	}

	/* remember end */
	if(inner)
		memcpy(&g_vector_inner[1], g_shape, sizeof(g_vector_inner[1]));
	else
		emit_cap(g_vector_inner[1], g_shape, false);
}

int main (int argc, char *argv[])
{
	/* emit STL header */
	dump_header();

	/* emit outer and inner spiral */
	emit_spiral(true);
	emit_spiral(false);

	/* emit full stl file on stdout */
	dump_stl(stdout);

	return 0;
}

