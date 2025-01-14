#pragma once

#include <vector>

#include "shptypes.h"
#include "bspan.h"
#include "converters.h"


namespace waavs {
	//
	// A Shp Record is comprised of a number of segment structures
	// Each PathSegment begins with a SegmentKind, followed by a series
	// of numbers appropriate for that segment kind. 
	//
	struct ShpShape
	{
		ShpShapeType fShapeType{ ShpShapeType::NullShape };
		std::vector<double> fNumbers{};



		ShpShape() { ; }
		ShpShape(ShpShapeType aType) :fShapeType(aType) { ; }
		ShpShape(const ShpShape& other)
			:fShapeType(other.fShapeType)
			, fNumbers(other.fNumbers)
		{}

		ShpShape& operator=(const ShpShape& other)
		{
			fShapeType = other.fShapeType;
			fNumbers = other.fNumbers;
			return *this;
		}
		
		// Access to the numbers
		std::vector<double>& numbers() { return fNumbers; }
		const std::vector<double>& numbers() const { return fNumbers; }
		
		
		void addNumber(double aNumber) { fNumbers.push_back(aNumber); }
		void addPoint(double x, double y) { fNumbers.push_back(x); fNumbers.push_back(y); }

		// Read a double precision number from the stream and save it into
		// the numbers vector
		bool readNumber(ByteSpan& bs)
		{
			if (bs.size() < 8)
				return false;

			double value{ 0 };
			read_f64_le(bs, value);
			addNumber(value);

			return true;
		}

		// read two double precision numbers from the stream and save them into
		// the numbers vector
		bool readPoint(ByteSpan& bs)
		{
			if (bs.size() < 16)
				return false;

			double value{ 0 };
			read_f64_le(bs, value);
			addNumber(value);

			read_f64_le(bs, value);
			addNumber(value);

			return true;
		}
		
		virtual bool readSelfFromStream(ByteSpan& bs)
		{
			return true;
		}

		virtual bool readFromStream(ByteSpan& bs)
		{
			// Read the record type first
			read_i32_le(bs, (int32_t&)fShapeType);

			return readSelfFromStream(bs);
		}
	};

	struct ShpPoint : public ShpShape
	{	
		ShpPoint()
			:ShpShape(ShpShapeType::Point)
		{}


		bool readSelfFromStream(ByteSpan& bs) override
		{
			return readPoint(bs);
		}
	};

	struct ShpMultiPart : public ShpShape
	{
		double xMin{ 0 };
		double yMin{ 0 };
		double xMax{ 0 };
		double yMax{ 0 };
		
		std::vector<int> fParts{};
		
		ShpMultiPart(ShpShapeType aType)
			:ShpShape(aType)
		{}

		ShpMultiPart(const ShpMultiPart& other)
			:ShpShape(other)
			, xMin(other.xMin)
			, yMin(other.yMin)
			, xMax(other.xMax)
			, yMax(other.yMax)
			, fParts(other.fParts)
		{}

		ShpMultiPart& operator=(const ShpMultiPart& other)
		{
			ShpShape::operator=(other);
			xMin = other.xMin;
			yMin = other.yMin;
			xMax = other.xMax;
			yMax = other.yMax;
			fParts = other.fParts;
			return *this;
		}
		
		std::vector<int>& parts() { return fParts; }
		const std::vector<int>& parts() const { return fParts; }
		
		
		void addPart(int apart) {fParts.push_back(apart);}
		
		bool loadPart(ByteSpan& bs)
		{
			if (bs.size() < 4)
				return false;

			int32_t value{ 0 };
			read_i32_le(bs, value);
			addPart(value);

			return true;
		}
		
		bool parseBBox(ByteSpan& bs)
		{
			if (bs.size() < 32)
				return false;

			read_f64_le(bs, xMin);
			read_f64_le(bs, yMin);
			read_f64_le(bs, xMax);
			read_f64_le(bs, yMax);

			return true;
		}
		
		bool parsePartsAndPoints(ByteSpan& bs)
		{
			if (bs.size() < 8)
				return false;

			int32_t numParts{ 0 };
			int32_t numPoints{ 0 };
			read_i32_le(bs, numParts);
			read_i32_le(bs, numPoints);

			if (bs.size() < numParts * 4)
				return false;

			for (int i = 0; i < numParts; ++i)
			{
				if (!loadPart(bs))
					break;
			}

			for (int i = 0; i < numPoints; ++i)
			{
				if (!readPoint(bs))
					return false;
			}

			return true;
		}

		bool readFromStream(ByteSpan& bs) override
		{
			// First, do whatever is needed for all shape records
			ShpShape::readFromStream(bs);

			// Need at least enough to read the header information
			if (bs.size() < 44)
				return false;

			if (!parseBBox(bs))
				return false;

			if (!parsePartsAndPoints(bs))
				return false;

			return readSelfFromStream(bs);
		}

	};


	struct ShpPolyLine : public ShpMultiPart
	{
		ShpPolyLine()
			:ShpMultiPart(ShpShapeType::PolyLine)
		{}


	};

	struct ShpPolygon : public ShpMultiPart
	{
		ShpPolygon()
			:ShpMultiPart(ShpShapeType::Polygon)
		{}

	};

	struct ShpMultiPoint : public ShpMultiPart
	{
		ShpMultiPoint()
			:ShpMultiPart(ShpShapeType::MultiPoint)
		{}

		bool readFromStream(ByteSpan& bs)
		{
			// Need at least enough to read the header information
			if (bs.size() < 44)
				return false;

			parseBBox(bs);


			int32_t numPoints{ 0 };
			read_i32_le(bs, numPoints);

			for (int i = 0; i < numPoints; i++)
			{
				readPoint(bs);
				//load_f64_le(bs);	// x
				//load_f64_le(bs);	// y
			}

			return true;
		}
	};
}
