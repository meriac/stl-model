/***************************************************************
 *
 * STL binary buffer helpers
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

#ifndef __STL_HELPER_H__
#define __STL_HELPER_H__

#include <stdio.h>
#include <stdint.h>

#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

#define MALLOC_STEPS (1024UL*1024)
#define HEADER_SIZE (80+sizeof(g_triangles))

uint32_t g_triangles;
uint8_t *g_buffer;
uint32_t g_buffer_pos, g_buffer_size;

/* create buffer for binary STL */
static void* add_stl(uint32_t bytes)
{
	void* res;

	/* allocate more memory in output if needed */
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

	/* return allocated buffer */
	res = g_buffer + g_buffer_pos;
	/* increment position in buffer for next allocation */
	g_buffer_pos += bytes;

	return res;
}

static void dump_vector(const Vector3d &v)
{
	/* get space for 3 gloatsi n output */
	float *out = static_cast<float*>(add_stl(sizeof(*out)*3));

	/* dump three floats in binary format */
	for(int i=0; i<3; i++)
		*out++ = v(i);
}

static void dump_header(void)
{
	/* populate header */
	void *buf = add_stl(HEADER_SIZE);
	memset(buf, 0, HEADER_SIZE);
}

static void dump_stl(FILE *stream)
{
	/* copy triangle count to header */
	memcpy(g_buffer + 80, &g_triangles, sizeof(g_triangles));

	/* dump whole file */
	fwrite(g_buffer, g_buffer_pos, 1, stream);
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

#endif/*__STL_HELPER_H__*/
