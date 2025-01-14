#pragma once

#include "shptypes.h"
#include "shpgeometry.h"
#include "shapefile.h"

#include <string>
#include <sstream>

namespace shputil {
	static std::ostringstream& outPoint(std::ostringstream& out, double x, double y)
	{
		out << x << "," << y;

		return out;
	}

	static std::ostringstream& outSinglePoint(std::ostringstream& out, const shp::ShpRecord& rec)
	{
		shp::ShpPoint shape{};
		waavs::BStream bs(rec.content());
		if (!shape.loadFromStream(bs))
		{
			printf("Failed to parse point\n");
			return out;
		}

		if (shape.numbers().size() < 2)
			return out;

		out << "M ";
		outPoint(out, shape.numbers()[0], shape.numbers()[1]);

		return out;
	}

	static std::ostringstream& outMultiPoint(std::ostringstream& out, const shp::ShpRecord& rec)
	{
		shp::ShpMultiPoint shape{};
		waavs::BStream bs(rec.content());
		if (!shape.loadFromStream(bs))
		{
			printf("Failed to parse multi-point\n");
			return out;
		}

		const auto& numbers = shape.numbers();

		size_t n = numbers.size();
		if (n < 2)
			return out;

		out << "M ";
		outPoint(out, numbers[0], numbers[1]);
		for (size_t i = 2; i < n; i += 2)
		{
			out << ",";
			outPoint(out, numbers[i], numbers[i + 1]);
		}

		return out;
	}

	static std::ostringstream& outPolygon(std::ostringstream& out, const shp::ShpRecord& rec)
	{
		shp::ShpPolygon poly{};
		waavs::BStream bs(rec.content());
		if (!poly.loadFromStream(bs))
		{
			printf("Failed to parse polygon\n");
			return out;
		}

		size_t numParts = poly.parts().size();
		size_t numPoints = poly.numbers().size() / 2;


		for (size_t i = 0; i < numParts; i++)
		{
			// each part begins with 'M', and ends with 'Z'
			size_t partStart = poly.parts()[i];
			size_t partEnd = (i + 1 < numParts) ? poly.parts()[i + 1] : numPoints;

			out << "M ";

			for (size_t j = partStart; j < partEnd; j++)
			{
				outPoint(out, poly.numbers()[(j * 2)], poly.numbers()[(j * 2) + 1]);
			}
			out << "Z";
		}

	}

	static std::ostringstream& outPolyline(std::ostringstream& out, const shp::ShpRecord& rec)
	{
		shp::ShpPolyLine poly{};
		waavs::BStream bs(rec.content());
		if (!poly.loadFromStream(bs))
		{
			printf("Failed to parse polygon\n");
			return out;
		}

		size_t numParts = poly.parts().size();
		size_t numPoints = poly.numbers().size() / 2;

		for (size_t i = 0; i < numParts; i++)
		{
			// each part begins with 'M', and ends with 'Z'
			size_t partStart = poly.parts()[i];
			size_t partEnd = (i + 1 < numParts) ? poly.parts()[i + 1] : numPoints;

			out << "M ";

			for (size_t j = partStart; j < partEnd; j++)
			{
				outPoint(out, poly.numbers()[(j * 2)], poly.numbers()[(j * 2) + 1]);
			}
			//out << "Z";
		}
	}

	// Generate a SVG <path> 'd' attribute from a shape
	static std::ostringstream& shpRecordToStream(std::ostringstream& out, const shp::ShpRecord& rec)
	{
		// go through the record and build a string using ostringstream

		switch (rec.shapeType())
		{
		case shp::ShpShapeType::Point:
			outSinglePoint(out, rec);
			break;

		case shp::ShpShapeType::MultiPoint:
			outMultiPoint(out, rec);
			break;

		case shp::ShpShapeType::Polygon:
			outPolygon(out, rec);
			break;

		case shp::ShpShapeType::PolyLine:
			outPolyline(out, rec);
			break;

		}

		return out;
	}
}
