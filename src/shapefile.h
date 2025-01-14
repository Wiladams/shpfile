#pragma once



#include <vector>
#include <map>
#include <string>

#include "bspan.h"
#include "shptypes.h"
#include "geometry.h"
#include "converters.h"



namespace waavs {
	// Shp file header
	// This is the same for content and index files
	// So, this can act as a base class for those structures

	struct ShapefileHeader
	{
		std::string fName{};
		
		// Header, the same for content and index
		int32_t fileCode{};
		int32_t fileLength{};
		int32_t version{};
		int32_t shapeType{};
		double xMin{};
		double yMin{};
		double xMax{};
		double yMax{};
		double zMin{};
		double zMax{};
		double mMin{};
		double mMax{};

		ShapefileHeader(std::string name)
			: fName(name)
		{}
		

		// treat the fName as a field
		// create setter and getter for fName field
		const std::string& name() const { return fName; }
		void name(const std::string& val) { fName = val; }
		
		
		// get file size
		size_t fileSize() const
		{
			return fileLength * 2;
		}
		

		
		waavs::bbox3f bbox() const
		{
			return { (float)xMin, (float)yMin, (float)zMin, (float)xMax, (float)yMax, (float)zMax };
		}
		
		waavs::bbox2f bbox2d() const
		{
			return { (float)xMin, (float)yMin, (float)xMax, (float)yMax };
		}
		
		// return shapeType as one of ShpShapeType
		ShpShapeType kind() const
		{
			return (ShpShapeType)shapeType;
		}


		// Read the file header using a BStream
		// Return true if successful
		virtual bool readSelfFromStream(waavs::ByteSpan& bs)
		{
			return true;
		}

		// This is the primary interface to be called
		// It will load the header from a file
		// then give a sub-class a chance to load itself
		// by calling loadSelfFromStream()
		virtual bool readFromStream(waavs::ByteSpan& bs)
		{
			// Check only once whether we have enough size
			// then we don't have to check for each read
			if (bs.size() < 100)
				return false;

			read_i32_be(bs, fileCode);	// always 9994
			bs.skip(5 * 4);				// skip past 5 unused ints

			double x1, y1;
			double x2, y2;

			read_i32_be(bs, fileLength);
			read_i32_le(bs, version);
			read_i32_le(bs, shapeType);
			read_f64_le(bs, x1);
			read_f64_le(bs, y1);
			read_f64_le(bs, x2);
			read_f64_le(bs, y2);
			read_f64_le(bs, zMin);
			read_f64_le(bs, zMax);
			read_f64_le(bs, mMin);
			read_f64_le(bs, mMax);

			xMin = x1;
			yMin = y1;
			xMax = x2;
			yMax = y2;

			// convert what we read in to SVG coordinates
			//latLongToMercatorSVG(y1, x1, xMin, yMin);
			//latLongToMercatorSVG(y2, x2, xMax, yMax);

			return readSelfFromStream(bs);
		}

	};
	
	// 
	// ShpFileRecord
	// Represents a record in a .shp file
	// This can be used by both the content iterator
	// as well as the index iterator
	//
	struct ShapefileRecord
	{
		size_t fRecordNumber{ 0 };

		ShapefileRecord() = default;
		ShapefileRecord(const ShapefileRecord& other) :fRecordNumber(other.fRecordNumber) {}
		
		ShapefileRecord& operator=(const ShapefileRecord& other)
		{
			fRecordNumber = other.fRecordNumber;

			return *this;
		}
		constexpr size_t recordNumber() const { return fRecordNumber; }
		constexpr void recordNumber(size_t num) { fRecordNumber = num; }
		
		
		virtual bool readFromStream(waavs::ByteSpan& bs)
		{
			return true;
		}
	};


	//========================================================
	// Shp File
	// This file contains the content of the 'shapefile'	
	//========================================================
	struct ShpRecord : public ShapefileRecord
	{
		waavs::ByteSpan fContentSpan{};	// The span of the record content, not including the header
		ShpShapeType fShapeType{ ShpShapeType::NullShape };
		
		ShpRecord() = default;
		ShpRecord(const ShpRecord& other) 
			:ShapefileRecord(other)
			, fContentSpan(other.fContentSpan)
			, fShapeType(other.fShapeType)
		{}
		
		ShpRecord& operator=(const ShpRecord& other)
		{
			ShapefileRecord::operator=(other);
			fContentSpan = other.fContentSpan;
			fShapeType = other.fShapeType;
			
			return *this;
		}

		// return the shapetype
		ShpShapeType shapeType() const { return fShapeType; }
		
		const waavs::ByteSpan& content() const { return fContentSpan; }
		size_t contentSize() const { return fContentSpan.size(); }
		
		// The recordSize() includes the bytes in the header of the record itself
		// which is 4 - WORDs (8 bytes), plus the contentSize()
		size_t recordSize() const {return contentSize() + 8;}
		
		// Read the record header using a BStream
		// Return true if successful
		bool readFromStream(waavs::ByteSpan& bs) override
		{
			if (bs.size() < 8)
				return false;

			int32_t contentLength{ 0 };
			
			int32_t recNum{ 0 };
			read_i32_be(bs, recNum);
			// assign recNum to fRecordNumber using static cast
			fRecordNumber = static_cast<size_t>(recNum);
			
			read_i32_be(bs, contentLength);
			
			// Create the contentSpan by using the current position
			// and size of the content
			size_t cSize = contentLength * 2;
			fContentSpan = bs.subSpan(0,cSize);
			if (fContentSpan.size() != cSize)
				return false;
			
			// Advance the stream we were fed
			bs.skip(cSize);
			
			// What should follow is the shape type of the record
			// so at least read the shape type so we can use that later
			ByteSpan recordStream(fContentSpan);
			read_i32_le(recordStream, (int32_t&)fShapeType);

			return true;
		}
	};

	
	//
	// ShpContentFile
	// Represents the '.shp' file, and contains the actual content
	//
	struct ShpFile : public ShapefileHeader
	{
		std::vector<ShpRecord> fRecords{};

		
		ShpFile(const std::string& name) :ShapefileHeader(name) {}
		ShpFile(const ShpFile& other) :ShapefileHeader(other), fRecords(other.fRecords) {}
		
		ShpFile& operator=(const ShpFile& other)
		{
			ShapefileHeader::operator=(other);
			fRecords = other.fRecords;

			return *this;
		}

		
		const std::vector<ShpRecord>& records() const
		{
			return fRecords;
		}
		
		// read records
		// We don't parse the record content here, just read the record header
		// including the size and shape type
		bool readSelfFromStream(waavs::ByteSpan& bs) override
		{
			// read records
			auto sentinel = bs.data();
			while (bs.size() > 0) {
				ShpRecord rec;
				if (!rec.readFromStream(bs))
					return false;
				fRecords.push_back(rec);

				// Skip to the next record, based on the size the record
				// we just read said it was.  We did not read the full record
				// content, only enough to know to capture the record
				// for later processing
				bs.fStart = (sentinel + rec.recordSize());
				sentinel = bs.data();
			}

			//printf("Record Count: %d\n", fRecords.size());

			return true;
		}



	};


	//===========================================================
	// Shx File
	// 
	// The Shapefile 'index' file
	//===========================================================
	// Shx file record
	// Each one is 8 bytes in the actual file
	//
	struct ShxRecord : public ShapefileRecord
	{
	private:
		size_t fRecordOffset{ 0 };
		size_t fContentSize{ 0 };
		
	public:
		ShxRecord() = default;
		ShxRecord(const ShxRecord& other)
			:ShapefileRecord(other)
			, fRecordOffset(other.fRecordOffset)
			, fContentSize(other.fContentSize)
		{}
		
		ShxRecord(int32_t recordNum) 
		{ 
			recordNumber(recordNum); 
		}

		ShxRecord& operator=(const ShxRecord& other)
		{
			ShapefileRecord::operator=(other);
			fRecordOffset = other.fRecordOffset;
			fContentSize = other.fContentSize;

			return *this;
		}
		
		//
		// The recordOffset gives you the offset within the .shp file for the record
		// This is the beginning of a shpRecord, which includes the header
		// The contentOffset gives you the offset within the .shp file for the content
		// The contentSize() tells you how big the content itself is
		// 
		// If you are using these interfaces, and you don't need the record header
		// then you can just position on the contentOffset and read the contentSize()
		//
		const size_t recordOffset() const { return fRecordOffset; }
		const size_t contentOffset() const { return fRecordOffset + 8; }
		const size_t contentSize() const { return fContentSize; }
		
		
		// Read the record header using a BStream
		bool readFromStream(waavs::ByteSpan& bs) override
		{
			if (bs.size() < 8)
				return false;

			int32_t offset{ 0 };
			int32_t cLength{ 0 };

			read_i32_be(bs, offset);
			read_i32_be(bs, cLength);

			fRecordOffset = offset * 2;
			fContentSize = cLength * 2;

			return true;
		}
	};

	
	//
	// ShpIndexFile
	// Represents the '.shx' file, and contains the index of the content
	// It's not strictly necessary, as you can traverse the content on its own
	// but it's typically part of the set.
	// 
	//
	struct ShxFile : public ShapefileHeader
	{
		std::map<int32_t, ShxRecord> fRecordMap;
		int32_t fRecordCount{ 0 };
		
		ShxFile() = default;

		const std::map<int32_t, ShxRecord>& recordMap() const
		{
			return fRecordMap;
		}
		
		const size_t recordCount() const
		{
			return fRecordCount;
		}
		
		const ShxRecord& getRecord(int32_t recordNum)
		{
			return fRecordMap[recordNum];
		}
		
		bool readSelfFromStream(waavs::ByteSpan &bs) override
		{
			while (bs.size() > 0)
			{
				ShxRecord rec(fRecordCount+1);
				if (!rec.readFromStream(bs))
					return false;
				
				fRecordCount++;
				fRecordMap.insert(std::make_pair(fRecordCount, rec));
			}

			return true;
		}


	};

}