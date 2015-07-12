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

#define SHAPE_POINTS 23
#define SHAPE_BORDER_WIDTH 3

Vector3d g_vector_buffer[2][SHAPE_POINTS];
Vector3d *g_shape;
Vector3d *g_target;

static void create_shape(double radius, double diameter)
{
	/* create simple extrusion shape - a circle */
	for(int i=0; i<SHAPE_POINTS; i++)
	{
		g_shape[i](0) = radius + sin(i*M_PI/(SHAPE_POINTS/2)) * diameter;
		g_shape[i](1) = 0;
		g_shape[i](2) = cos(i*M_PI/(SHAPE_POINTS/2)) * diameter;
	}
}

static void translate_shape(const Vector3d &translate, int index)
{
}

static void emit_stl_layer(void)
{
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
	create_shape(100, 20);

	/* emit sculpture */
	for(i=0; i<SHAPE_POINTS; i++)
	{
		/* translate all points */
		translate_shape(Vector3d::UnitZ(), i);

		/* emit resulting layer */
		emit_stl_layer();

		/* move to next layer */
		tmp = g_shape;
		g_shape = g_target;
		g_target = tmp;

	}

	/* emit full stl file on stdout */
	dump_stl(stdout);

	return 0;
}

