#pragma once

#include "shptypes.h"
#include "shpgeometry.h"
#include "shapefile.h"

#include <string>
#include <sstream>



namespace waavs
{



	//=======================================================
	// Printing SVG appropriate to stdout
	//=======================================================
	static void printPoint(waavs::ByteSpan& bs)
	{
		ShpPoint pt{};
		if (!pt.readFromStream(bs))
		{
			printf("Failed to parse point\n");
			return;
		}
		printf("<rect class='loc' x='%f' y='%f'  />\n", pt.numbers()[0], pt.numbers()[1]);
	}
	
	static void printMultiPoint(waavs::ByteSpan& bs)
	{
		ShpMultiPoint mp{};
		if (!mp.readFromStream(bs))
		{
			printf("Failed to parse multi-point\n");
			return;
		}
		
		size_t numPoints = mp.numbers().size() / 2;
		
		//printf("MultiPoint: [%zd] points\n", numPoints);
		for (size_t i=0;i<numPoints;i++)
		{
			printf("M %f, %f\n", mp.numbers()[(i*2)], mp.numbers()[(i*2)+1]);
		}
	}

	// Print a polygon as a svg path
	static void printPolygon(waavs::ByteSpan& bs)
	{
		ShpPolygon poly{};
		if (!poly.readFromStream(bs))
		{
			printf("Failed to parse polygon\n");
			return;
		}
		
		size_t numParts = poly.parts().size();
		size_t numPoints = poly.numbers().size() / 2;


		printf("<path d=\"");
		for (size_t i = 0; i < numParts; i++)
		{
			// each part begins with 'M', and ends with 'Z'
			size_t partStart = poly.parts()[i];
			size_t partEnd = (i + 1 < numParts) ? poly.parts()[i + 1] : numPoints;

			printf("M ");
			//printf("Part [%zd]: [%zd] points\n", i, partEnd - partStart);
			for (size_t j = partStart; j < partEnd; j++)
			{
				printf(" %f, %f", poly.numbers()[(j * 2)], poly.numbers()[(j * 2) + 1]);
			}
			printf("Z\n");
		}
		printf("\"/>\n");
	}

	static void printPolyLine(waavs::ByteSpan& bs)
	{
		ShpPolyLine pl{};
		if (!pl.readFromStream(bs))
		{
			printf("Failed to parse polyline\n");
			return;
		}

		size_t numParts = pl.parts().size();
		size_t numPoints = pl.numbers().size() / 2;

		//printf("<path fill='none' stroke='black' stroke-width=\"0.0001\" d=\"");
		printf("<path  d=\"");
		for (size_t i = 0; i < numParts; i++)
		{
			// each part begins with 'M', and ends with 'Z'
			size_t partStart = pl.parts()[i];
			size_t partEnd = (i + 1 < numParts) ? pl.parts()[i + 1] : numPoints;

			printf("M ");
			//printf("Part [%zd]: [%zd] points\n", i, partEnd - partStart);
			for (size_t j = partStart; j < partEnd; j++)
			{
				printf(" %f, %f", pl.numbers()[(j * 2)], pl.numbers()[(j * 2) + 1]);
			}

		}
		printf("\"/>\n");
	}
}