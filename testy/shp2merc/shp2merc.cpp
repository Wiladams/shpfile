

#include <cstdio>
#include <vector>
#include <map>


#include "bspan.h"
#include "mappedfile.h"
#include "mercator.h"

#include "shapefile.h"
#include "shputil.h"




int gargc;
char** gargv;


using namespace waavs;



static void mercPrintPoint(ByteSpan& bs)
{
	waavs::ShpPoint pt{};
	if (!pt.readFromStream(bs))
	{
		printf("Failed to parse point\n");
		return;
	}

	double pixelX{ 0 };
	double pixelY{ 0 };
	
	latLongToMercatorSVG(pt.numbers()[1], pt.numbers()[0], pixelX, pixelY);

	
	printf("<path d='%3.4f, %3.4f'/>\n", pixelX, pixelY);
}

static void mercPrintMultiPoint(ByteSpan& bs)
{
	waavs::ShpMultiPoint mp{};
	if (!mp.readFromStream(bs))
	{
		printf("Failed to parse multi-point\n");
		return;
	}

	size_t numPoints = mp.numbers().size() / 2;

	//printf("MultiPoint: [%zd] points\n", numPoints);
	for (size_t i = 0; i < numPoints; i++)
	{
		double pixelX{ 0 };
		double pixelY{ 0 };

		latLongToMercatorSVG(mp.numbers()[(i * 2) + 1], mp.numbers()[(i * 2)], pixelX, pixelY);
		
		printf("M %3.4f, %3.4f\n", pixelX, pixelY);
	}
}

static void mercPrintPolyLine(ByteSpan& bs, bool closeIt = false)
{
	waavs::ShpPolyLine pl{};
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
			double pixelX{ 0 };
			double pixelY{ 0 };

			latLongToMercatorSVG(pl.numbers()[(j * 2) + 1], pl.numbers()[(j * 2)], pixelX, pixelY);
			
			printf(" %3.4f, %3.4f", pixelX, pixelY);
		}
		
		if (closeIt) {
			printf(" Z");
		}
	}
	printf("\"/>\n");
}

void printShpFile(ShpFile& shp)
{
	double minX{0};
	double minY{0};
	latLongToMercatorSVG(shp.yMax, shp.xMin, minX, minY);
	
	double maxX{ 0 };
	double maxY{ 0 };
	latLongToMercatorSVG(shp.yMin, shp.xMax, maxX, maxY);

	double lenX = maxX - minX;
	double lenY = maxY - minY;
	
	printf("<svg \n");
	printf("  xmlns='http://www.w3.org/2000/svg'\n");
	printf("  xmlns:waavs='https:william-a-adams.com/namespaces/waavs'\n");
	printf("  xmlns:waavsgeo='https:william-a-adams.com/namespaces/waavs'\n");
	printf("  width='%3.4f' height='%3.4f'\n", lenX, lenY);
	printf("  viewBox ='%3.4f %3.4f %3.4f %3.4f' \n", minX, minY, lenX, lenY);
	printf(">\n");

	printf("<style>\n");
	printf("  svg {stroke-width:0.5;stroke:black;vector-effect:non-scaling-stroke;fill:black;}\n");
	printf("  path {paint-order:fill,stroke;stroke-width:0.5;stroke:black;vector-effect:non-scaling-stroke;fill:beige;}\n");
	printf("</style>\n");



	for (const ShpRecord& rec : shp.records())
	{
		//printf("============================================\n");
		//printf("Record Number: %d\n", rec.fRecordNumber);
		//printf("Record Size: %zd\n", rec.recordSize());
		//printf("Content Size: %zd\n", rec.contentSize());
		//printf("Content Span : %zd\n", rec.content().size());
		//printf("Shape Type: %d\n", rec.shapeType());


		// create a stream on the record content
		ByteSpan rs(rec.content());

		switch (rec.shapeType())
		{
		case ShpShapeType::NullShape:
			printf("Null Shape\n");
			break;
		case ShpShapeType::Point:
			//printf("== Point ==\n");
			mercPrintPoint(rs);
			break;
		case ShpShapeType::PolyLine:
			//printf("PolyLine\n");
			mercPrintPolyLine(rs);
			break;
		case ShpShapeType::Polygon:
			//printf("== Polygon ==\n");
			mercPrintPolyLine(rs, true);
			break;
		case ShpShapeType::MultiPoint:
			//printf("== MultiPoint ==\n");
			mercPrintMultiPoint(rs);
			break;
		case ShpShapeType::PointZ:
			printf("PointZ\n");
			break;
		case ShpShapeType::PolyLineZ:
			printf("PolyLineZ\n");
			break;
		case ShpShapeType::PolygonZ:
			printf("<!-- PolygonZ -->\n");
			mercPrintPolyLine(rs, true);
			break;
		case ShpShapeType::MultiPointZ:
			printf("MultiPointZ\n");

			break;
		case ShpShapeType::PointM:
			printf("PointM\n");
			break;
		case ShpShapeType::PolyLineM:
			printf("PolyLineM\n");
			break;
		case ShpShapeType::PolygonM:
			printf("PolygonM\n");
			break;
		case ShpShapeType::MultiPointM:
			printf("MultiPointM\n");
			break;
		case ShpShapeType::MultiPatch:
			printf("MultiPatch\n");
			break;
		default:
			printf("Unknown Shape Type\n");
			break;
		}
	}

	printf("</svg>\n");

}

static void convertShpFile(const char *filename)
{
	std::string shpFilename = filename;
	auto shpFile = MappedFile::create_shared(shpFilename);

	if (!shpFile)
	{
		printf("Failed to open shp file: %s\n", filename);
		return;
	}

	ByteSpan shpChunk(shpFile->data(), shpFile->size());

	//BStream shpStream(shpChunk);

	ShpFile shp(shpFilename);
	if (!shp.readFromStream(shpChunk))
	{
		printf("Failed to parse shp file\n");
		return;
	}

	printShpFile(shp);
}

int main(int argc, char** argv)
{
	gargc = argc;
	gargv = argv;

	//printf("argc: %d\n", argc);

	if (argc < 2)
	{
		printf("Usage: shp2merc <filename>\n");
		return 0;
	}

	const char * filename = gargv[1];
	
	convertShpFile(filename);

	return 1;
}
