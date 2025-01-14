#pragma once


#include "maths.h"


namespace waavs {


	const double EARTH_RADIUS = 6378137.0; // in meters
	const double MAX_WEB_MERCATOR_COORD = 20037508.34; // Maximum x/y coordinate in Web Mercator

	// Convert latitude/longitude to transformed Web Mercator X/Y coordinates
	// We want (0,0) to be in the upper left
	// with +x to the right
	// and +y going down
	// This fits SVG and typical computer graphics
	void latLongToMercatorSVG(double lat, double lon, double& x, double& y) 
	{
		// Convert longitude to Web Mercator X
		x = lon * EARTH_RADIUS * waavs::pi / 180.0;

		// Convert latitude to Web Mercator Y
		y = EARTH_RADIUS * std::log(std::tan(waavs::pi / 4.0 + lat * (waavs::pi / 180.0) / 2.0));

		// Apply transformations
		x += MAX_WEB_MERCATOR_COORD;  // Shift X to make negative X values start at 0
		
		// Flip and shift Y to make north pole Y=0
		if (y > 0)
			y = map(y, 0, MAX_WEB_MERCATOR_COORD, MAX_WEB_MERCATOR_COORD, 0);
		else
			y = map(y, 0, -MAX_WEB_MERCATOR_COORD, MAX_WEB_MERCATOR_COORD, MAX_WEB_MERCATOR_COORD * 2);
		//y = MAX_WEB_MERCATOR_COORD - y; 
	}

}

/*
namespace waavs
{
	// projection of latitude, longitude to x, y
	// This is a Mercator projection
	// using NADS 83 datum
	static const double earthRadius_epsg3857 = 6378137.0;	// meters	
	static const double maxCoordinate_epsg3857 = 20037508.342789244;
	
	static inline double lon_to_x(double lon)
	{
		return waavs::radians(lon) * earthRadius_epsg3857;
	}
	
	// faster version of the canonical log(tan()) version
	static inline double lat_to_y_with_sin(double lat) {
		const double f = std::sin(maths::radians(lat));
		const double y = earthRadius_epsg3857 * 0.5 * std::log((1 + f) / (1 - f));
		return y;
	}
	
	// Convert latitude/longitude degrees into XY in Spherical Mercator EPSG:3857
	static inline maths::vec2d latlongToXY(double lat, double lon)
	{
		return { lon_to_x(lon), lat_to_y_with_sin(lat)};
	}

	// Setup a viewport based on latitude and longitude
	// Also setup a mapping, that converts to a pixel range
	// Then allow lat/long to be converted to pixel coordinates
	struct MercatorMapping
	{
		maths::bbox2f fLatLonBounds;
		maths::bbox2f fXYBounds;
		maths::bbox2f fPixelFrame;

		// The constructor must give the range 
		// of the frame in terms of latitude and longitude
		MercatorMapping(const maths::bbox2f& latlonbox, maths::bbox2f& fr)
			:fLatLonBounds(latlonbox)
			, fPixelFrame(fr)
		{
			// Convert the lat/long to x/y
			auto xyMin = latlongToXY(latlonbox.min.y, latlonbox.min.x);
			auto xyMax = latlongToXY(latlonbox.max.y, latlonbox.max.x);

			fXYBounds.min.x = xyMin.x;
			fXYBounds.max.x = xyMax.x;
			fXYBounds.min.y = xyMin.y;
			fXYBounds.max.y = xyMax.y;
		}

		maths::vec2d convertToPixel(const double lat, const double lon)
		{
			// Get coordinates on a world scale in absolute kilometers
			const maths::vec2d xy = latlongToXY(lat, lon);

			// If the point is not even within our constrained box, then
			// just return negatives
			if (!contains(fXYBounds, xy.x, xy.y))
				return { -1, -1 };

			// Now we know the xy is within the bounding area in XY space
			// so use maths::map to convert to the pixel space
			const double x = maths::map(xy.x, fXYBounds.min.x, fXYBounds.max.x, fPixelFrame.min.x, fPixelFrame.max.x);
			const double y = maths::map(xy.y, fXYBounds.min.y, fXYBounds.max.y, fPixelFrame.min.y, fPixelFrame.max.y);

			return { x, y };
		}
	};
}
*/